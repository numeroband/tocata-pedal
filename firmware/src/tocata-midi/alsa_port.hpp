#pragma once
#include "midi_port.hpp"
#include <alsa/asoundlib.h>
#include <poll.h>
#include <unistd.h>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// Linux MIDI backend, on the ALSA sequencer. Reached through midi_port.hpp's
// platform dispatch; see coremidi_port.hpp for the macOS counterpart.
//
// The sequencer has no callback thread of its own, so instead of spawning one
// this registers snd_seq's poll descriptor with the shared asio::io_context.
// Everything -- MIDI in, MIDI out, multicast, WING -- then runs on the single
// thread that calls io_context.run().

namespace tocata::midi {

namespace detail {

// A port belonging to some other sequencer client.
struct AlsaEndpoint {
    int client;
    int port;
    std::string name;
};

// Ports we can read from == CoreMIDI "sources"; ports we can write to ==
// CoreMIDI "destinations". A bidirectional port qualifies as both.
inline constexpr unsigned int kSourceCaps = SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ;
inline constexpr unsigned int kDestCaps = SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE;

// Every port on every client except our own and the system client, filtered to
// those carrying all of required_caps.
//
// Names are formatted "<client name>:<port name>", which is what `aconnect -l`
// prints and what users type into `aconnect`. A substring match against that
// hits on a bare client name, a bare port name, or the pair -- so the
// match-by-substring contract is the same as on CoreMIDI display names.
inline std::vector<AlsaEndpoint> enumerateWith(snd_seq_t* seq, unsigned int required_caps) {
    std::vector<AlsaEndpoint> out;
    const int self = snd_seq_client_id(seq);

    snd_seq_client_info_t* cinfo = nullptr;
    snd_seq_port_info_t* pinfo = nullptr;
    snd_seq_client_info_malloc(&cinfo);
    snd_seq_port_info_malloc(&pinfo);

    snd_seq_client_info_set_client(cinfo, -1);  // -1 == start before client 0
    while (snd_seq_query_next_client(seq, cinfo) >= 0) {
        const int client = snd_seq_client_info_get_client(cinfo);
        if (client == self || client == SND_SEQ_CLIENT_SYSTEM) continue;
        const std::string client_name = snd_seq_client_info_get_name(cinfo);

        snd_seq_port_info_set_client(pinfo, client);
        snd_seq_port_info_set_port(pinfo, -1);
        while (snd_seq_query_next_port(seq, pinfo) >= 0) {
            const unsigned int caps = snd_seq_port_info_get_capability(pinfo);
            if ((caps & required_caps) != required_caps) continue;
            if (caps & SND_SEQ_PORT_CAP_NO_EXPORT) continue;
            out.push_back({client, snd_seq_port_info_get_port(pinfo),
                           client_name + ":" + snd_seq_port_info_get_name(pinfo)});
        }
    }

    snd_seq_port_info_free(pinfo);
    snd_seq_client_info_free(cinfo);
    return out;
}

// enumerateWith() on a throwaway client, for the static listing helpers that
// have no port of their own.
inline std::vector<AlsaEndpoint> enumerate(unsigned int required_caps) {
    snd_seq_t* seq = nullptr;
    if (snd_seq_open(&seq, "default", SND_SEQ_OPEN_DUPLEX, 0) < 0) return {};
    auto out = enumerateWith(seq, required_caps);
    snd_seq_close(seq);
    return out;
}

inline std::vector<std::string> names(const std::vector<AlsaEndpoint>& endpoints) {
    std::vector<std::string> out;
    out.reserve(endpoints.size());
    for (const auto& e : endpoints) out.push_back(e.name);
    return out;
}

inline std::optional<AlsaEndpoint> findFirst(const std::vector<AlsaEndpoint>& endpoints,
                                             const std::string& name_substr) {
    for (const auto& e : endpoints) {
        if (e.name.find(name_substr) != std::string::npos) return e;
    }
    return std::nullopt;
}

}

// Shared machinery for both ALSA-backed ports: one duplex sequencer port, a
// MIDI byte-stream codec, and the asio hookup. Subclasses differ only in how
// they name themselves and whether they auto-connect to another client.
class AlsaPort : public MidiPort {
public:
    // Neither copyable nor movable: an armed async_wait handler captures `this`,
    // and the sequencer handle and duplicated fd are raw resources. main.cpp
    // stores these behind unique_ptr, so neither operation is needed.
    AlsaPort(const AlsaPort&) = delete;
    AlsaPort& operator=(const AlsaPort&) = delete;
    AlsaPort(AlsaPort&&) = delete;
    AlsaPort& operator=(AlsaPort&&) = delete;

    ~AlsaPort() override {
        // Order matters: drop the asio descriptor first so it cancels any
        // pending async_wait and closes our duplicate, then let snd_seq_close
        // deal with the sequencer's own copy of that fd.
        //
        // Cancelling posts the handler with operation_aborted, and that handler
        // holds a now-dangling `this`. It never dereferences it on the error
        // path, and in practice main.cpp only destroys ports after
        // io_context.run() has returned, so the handler is discarded unrun.
        if (_fd.is_open()) {
            asio::error_code ec;
            _fd.close(ec);
        }
        if (_encoder) snd_midi_event_free(_encoder);
        if (_decoder) snd_midi_event_free(_decoder);
        if (_seq) {
            if (_port >= 0) snd_seq_delete_simple_port(_seq, _port);
            snd_seq_close(_seq);
        }
    }

    void setCallback(Callback callback) override { _callback = std::move(callback); }

    void send(std::span<const uint8_t> data) override {
        if (data.empty()) return;
        snd_midi_event_reset_encode(_encoder);

        const uint8_t* bytes = data.data();
        long remaining = static_cast<long>(data.size());
        while (remaining > 0) {
            snd_seq_event_t ev;
            snd_seq_ev_clear(&ev);
            const long consumed = snd_midi_event_encode(_encoder, bytes, remaining, &ev);
            if (consumed <= 0) break;  // malformed input; drop the rest
            bytes += consumed;
            remaining -= consumed;
            if (ev.type == SND_SEQ_EVENT_NONE) continue;  // message not complete yet

            snd_seq_ev_set_source(&ev, _port);  // stamp which of our ports it leaves from
            snd_seq_ev_set_subs(&ev);           // fan out to everyone subscribed to it
            snd_seq_ev_set_direct(&ev);         // bypass the timestamped queue

            // Writes straight through, so no snd_seq_drain_output() is needed.
            // snd_seq_event_output() + a single drain after the loop would
            // amortise the syscall over a burst, but this bridge handles one
            // datagram at a time and latency matters more.
            const int err = snd_seq_event_output_direct(_seq, &ev);
            if (err < 0) {
                std::fprintf(stderr, "[alsa] output: %s\n", snd_strerror(err));
            }
        }
    }

protected:
    AlsaPort(asio::io_context& io_context, const std::string& client_name,
             const std::string& port_name)
        : _fd{io_context} {
        int err = snd_seq_open(&_seq, "default", SND_SEQ_OPEN_DUPLEX, 0);
        if (err < 0) {
            _seq = nullptr;
            throw std::runtime_error(std::string{"AlsaPort: snd_seq_open failed: "} +
                                     snd_strerror(err));
        }
        snd_seq_set_client_name(_seq, client_name.c_str());
        // Required: snd_seq_event_input() must never block the io_context.
        snd_seq_nonblock(_seq, 1);

        // ALSA models as one duplex port what CoreMIDI splits across a
        // MIDISourceCreate (readable by others == our output) and a
        // MIDIDestinationCreate (writable by others == our input). The port
        // therefore carries all four caps and shows up under both `aconnect -i`
        // and `aconnect -o`.
        _port = snd_seq_create_simple_port(
            _seq, port_name.c_str(),
            SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ | SND_SEQ_PORT_CAP_WRITE |
                SND_SEQ_PORT_CAP_SUBS_WRITE,
            SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION);
        if (_port < 0) {
            err = _port;
            _port = -1;
            throw std::runtime_error(std::string{"AlsaPort: create_simple_port failed: "} +
                                     snd_strerror(err));
        }

        snd_midi_event_new(kEventBufSize, &_encoder);
        snd_midi_event_new(kEventBufSize, &_decoder);
        // Always emit a full status byte. The pedal's midiCallback reads
        // packet[0] as the status, so running-status elision would corrupt it.
        snd_midi_event_no_status(_decoder, 1);
        snd_midi_event_reset_encode(_encoder);
        snd_midi_event_reset_decode(_decoder);

        attachPollFd();
    }

    // Begins listening. Called at the end of each subclass constructor rather
    // than from this one, so a subclass that throws during its own setup never
    // leaves a handler armed against a half-built object.
    void start() { arm(); }

    snd_seq_t* seq() const { return _seq; }
    int port() const { return _port; }

private:
    // Worst-case SysEx on this protocol is ~593 bytes (midi_sysex.h), and the
    // multicast transport caps payloads at 512 (mc_midi.hpp), so 2048 leaves
    // room for both the codec scratch and SysEx reassembly.
    static constexpr size_t kEventBufSize = 2048;
    static constexpr size_t kMaxSysExSize = 2048;

    void attachPollFd() {
        const int count = snd_seq_poll_descriptors_count(_seq, POLLIN);
        if (count != 1) {
            throw std::runtime_error("AlsaPort: expected exactly 1 POLLIN descriptor, got " +
                                     std::to_string(count));
        }
        struct pollfd pfd {};
        snd_seq_poll_descriptors(_seq, &pfd, 1, POLLIN);
        // Duplicate: asio::posix::stream_descriptor closes whatever fd it owns,
        // but this one belongs to snd_seq_t and is closed by snd_seq_close().
        // Handing over the original would be a double close.
        const int fd = ::dup(pfd.fd);
        if (fd < 0) throw std::runtime_error("AlsaPort: dup(sequencer pollfd) failed");
        _fd.assign(fd);
    }

    void arm() {
        _fd.async_wait(asio::posix::descriptor_base::wait_read,
                       [this](const asio::error_code& ec) {
                           if (ec) {
                               if (ec != asio::error::operation_aborted) {
                                   std::fprintf(stderr, "[alsa] wait: %s\n", ec.message().c_str());
                               }
                               return;
                           }
                           drainInput();
                           arm();
                       });
    }

    void drainInput() {
        snd_seq_event_t* ev = nullptr;
        int err;
        // -EAGAIN terminates the loop once the queue is empty (we set
        // nonblock above). snd_seq_event_input_pending() would gate this
        // equivalently at the cost of an extra syscall per iteration.
        while ((err = snd_seq_event_input(_seq, &ev)) >= 0) {
            handleEvent(ev);
        }
        if (err == -ENOSPC) {
            // The kernel input pool overflowed and dropped events, so any
            // partial SysEx and the decoder's running-status state are garbage.
            snd_midi_event_reset_decode(_decoder);
            _sysex.clear();
            std::fprintf(stderr, "[alsa] input overrun, events lost\n");
        } else if (err != -EAGAIN) {
            std::fprintf(stderr, "[alsa] event_input: %s\n", snd_strerror(err));
        }
    }

    void handleEvent(const snd_seq_event_t* ev) {
        if (!_callback) return;

        if (ev->type == SND_SEQ_EVENT_SYSEX) {
            handleSysEx(ev);
            return;
        }

        uint8_t buf[kEventBufSize];
        const long length = snd_midi_event_decode(_decoder, buf, sizeof(buf), ev);
        // A negative return is routine -- it is how non-MIDI events (client
        // announcements, port subscriptions, clock) report that they have no
        // byte-stream form. Ignore them.
        if (length > 0) _callback({buf, size_t(length)});
    }

    // ALSA splits a long SysEx across several SND_SEQ_EVENT_SYSEX events: only
    // the first starts with F0 and only the last ends with F7. Reassemble
    // before handing it on, since each fragment would otherwise become its own
    // multicast datagram and the pedal's processSysEx would reject it.
    void handleSysEx(const snd_seq_event_t* ev) {
        const auto* fragment = static_cast<const uint8_t*>(ev->data.ext.ptr);
        const size_t length = ev->data.ext.len;
        if (length == 0) return;

        if (fragment[0] == 0xF0) _sysex.clear();
        if (_sysex.size() + length > kMaxSysExSize) {
            std::fprintf(stderr, "[alsa] sysex longer than %zu bytes, dropped\n", kMaxSysExSize);
            _sysex.clear();
            return;
        }
        _sysex.insert(_sysex.end(), fragment, fragment + length);

        if (_sysex.back() == 0xF7) {
            _callback({_sysex.data(), _sysex.size()});
            _sysex.clear();
        }
    }

    snd_seq_t* _seq{nullptr};
    int _port{-1};
    snd_midi_event_t* _encoder{nullptr};
    snd_midi_event_t* _decoder{nullptr};
    asio::posix::stream_descriptor _fd;
    std::vector<uint8_t> _sysex;
    Callback _callback;
};

// Creates a new sequencer port that other applications connect to. Like
// CoreMIDI's MIDISourceCreate it just exists and waits to be subscribed; it
// does not wire itself to anything.
class VirtualMidi : public AlsaPort {
public:
    VirtualMidi(asio::io_context& io_context, const std::string& port_name)
        // Name the client and the port alike, so `aconnect -l` reads
        //   client 129: 'TocataMIDI 1' [type=user]
        //       0 'TocataMIDI 1'
        : AlsaPort{io_context, port_name, port_name} {
        start();
    }
};

// Attaches to an existing sequencer port (matched by substring against its
// "<client>:<port>" name) instead of creating a standalone virtual port, so
// TocataMidi can bridge multicast MIDI directly into a device other software
// already expects to talk to.
class SystemMidi : public AlsaPort {
public:
    // Throws std::runtime_error (message includes currently-available
    // source/destination names) if name_substr matches neither a readable
    // nor a writable port currently present on the system.
    SystemMidi(asio::io_context& io_context, const std::string& name_substr)
        : AlsaPort{io_context, "TocataMIDI->" + name_substr, "bridge"} {
        // Enumerate through our own client so that it stays out of both the
        // match and the "available devices" listing below.
        const auto sources = detail::enumerateWith(seq(), detail::kSourceCaps);
        const auto destinations = detail::enumerateWith(seq(), detail::kDestCaps);
        auto src = detail::findFirst(sources, name_substr);
        auto dst = detail::findFirst(destinations, name_substr);

        if (!src && !dst) {
            // The base is fully constructed by now, so its destructor runs as
            // this unwinds and releases the sequencer. Nothing is armed yet --
            // start() is deliberately the last thing this constructor does.
            std::string msg = "SystemMidi: no device matching \"" + name_substr + "\" found.\n";
            msg += "Available sources:\n";
            for (auto& n : detail::names(sources)) msg += "  " + n + "\n";
            msg += "Available destinations:\n";
            for (auto& n : detail::names(destinations)) msg += "  " + n + "\n";
            throw std::runtime_error(msg);
        }

        // Their output feeds our input, and our output feeds their input.
        if (src) {
            const int err = snd_seq_connect_from(seq(), port(), src->client, src->port);
            if (err < 0) {
                std::fprintf(stderr, "[alsa] connect_from(%s): %s\n", src->name.c_str(),
                             snd_strerror(err));
            }
        }
        if (dst) {
            const int err = snd_seq_connect_to(seq(), port(), dst->client, dst->port);
            if (err < 0) {
                std::fprintf(stderr, "[alsa] connect_to(%s): %s\n", dst->name.c_str(),
                             snd_strerror(err));
            }
        }

        start();
    }

    // Names of all sequencer ports we could read from / write to, in
    // client-then-port enumeration order.
    static std::vector<std::string> listSources() {
        return detail::names(detail::enumerate(detail::kSourceCaps));
    }

    static std::vector<std::string> listDestinations() {
        return detail::names(detail::enumerate(detail::kDestCaps));
    }

    // Prints a labeled listing of listSources()/listDestinations(). Backs
    // --list-devices. Numbering is cosmetic only -- device selection is
    // always by substring match, not by these indices.
    static void printAvailableDevices() {
        printf("Sources (input to TocataMidi):\n");
        auto sources = listSources();
        for (size_t i = 0; i < sources.size(); ++i) {
            printf("  %zu: %s\n", i, sources[i].c_str());
        }
        printf("Destinations (output from TocataMidi):\n");
        auto destinations = listDestinations();
        for (size_t i = 0; i < destinations.size(); ++i) {
            printf("  %zu: %s\n", i, destinations[i].c_str());
        }
    }
};

}
