#pragma once

#define APPLEMIDI_INITIATOR
#include <Ethernet.h>
#include <AppleMIDI.h>

#include <cstdint>
#include <midi_sender.h>
#include <midi_sysex.h>

namespace tocata {

class AppleMidi : public MidiSender {
public:
    static AppleMidi& sharedInstance() {
        static AppleMidi shared;
        return shared;
    }
    void init();
    void run();
	void sendProgram(uint8_t channel, uint8_t program) override;
	void sendControl(uint8_t channel, uint8_t control, uint8_t value) override;
	void sendSysEx(std::span<const uint8_t> sysex) override;
    void setCallback(Callback callback) override {
        _callback = callback;
    }

private:
    void sendInvite();

    struct MidiSettings : public APPLEMIDI_NAMESPACE::AppleMIDISettings
    {
        static const unsigned SysExMaxSize = kMidiSysExMaxSize;
    };
    struct AppleMidiSettings : public APPLEMIDI_NAMESPACE::DefaultSettings
    {
        static const size_t MaxBufferSize = kMidiSysExMaxSize;        
    };
    using MidiSession = APPLEMIDI_NAMESPACE::AppleMIDISession<EthernetUDP, AppleMidiSettings>;
    using MidiInterface = MIDI_NAMESPACE::MidiInterface<MidiSession, MidiSettings>;
    MidiSession _midi_session{"TocataPedal", DEFAULT_CONTROL_PORT};
    MidiInterface _midi{_midi_session};
    size_t _connected{0};
    bool _initialized = false;
    Callback _callback{};
};

}