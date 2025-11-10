#include "apple_midi.h"
#include <EthernetBonjour.h>

EthernetClass Ethernet;
_serial Serial;

namespace tocata {

static AppleMidi* _apple_midi;

void AppleMidi::init()
{
  // Hack for the callbacks
  _apple_midi = this;
  EthernetBonjour.begin("TocataPedal");

  EthernetBonjour.addServiceRecord("TocataPedal._apple-midi",
                                   5004,
                                  MDNSServiceUDP);

  _midi.begin(MIDI_CHANNEL_OMNI);

  _midi_session.setHandleConnected([](const APPLEMIDI_NAMESPACE::ssrc_t & ssrc, const char* name) {
    AppleMidi::sharedInstance()._connected++;
    printf("[AM%u] Connected to session %s\n", ssrc, name);
  });
  _midi_session.setHandleDisconnected([](const APPLEMIDI_NAMESPACE::ssrc_t & ssrc) {
    AppleMidi::sharedInstance()._connected--;
    printf("[AM%u] Disconnected\n"), ssrc;
  });

  _midi.setHandleControlChange([](Channel channel, byte v1, byte v2) {
    printf("ControlChange %u %u %u\n", channel, v1, v2);
  });
  _midi.setHandleProgramChange([](Channel channel, byte v1) {
    printf("ProgramChange %u %u\n", channel, v1);
  });
  _midi.setHandlePitchBend([](Channel channel, int v1) {
    printf("PitchBend %u %u\n", channel, v1);
  });
  _midi.setHandleNoteOn([](byte channel, byte note, byte velocity) {
    printf("NoteOn %u %u %u\n", channel, note, velocity);
  });
  _midi.setHandleNoteOff([](byte channel, byte note, byte velocity) {
    printf("NoteOff %u %u %u\n", channel, note, velocity);
  });
  _midi.setHandleSystemExclusive([](byte* array, unsigned size) {
    if (!_apple_midi->_callback) { return; }
    std::span<uint8_t> buffer{array, kMidiSysExMaxSize};
    _apple_midi->_callback({array, size}, buffer, *_apple_midi);
  });

  _midi_session.sendInvite({192, 168, 2, 20}, DEFAULT_CONTROL_PORT); // port is 5004 by default
  _initialized = true;
}

void AppleMidi::run() {
    _midi.read();
    EthernetBonjour.run();
}

void AppleMidi::sendProgram(uint8_t channel, uint8_t program) {
    if (!_initialized) { return; }
    printf("sendprogram %u\n", program);
    _midi.sendProgramChange(program, channel + 1);
}

void AppleMidi::sendControl(uint8_t channel, uint8_t control, uint8_t value) {
    if (!_initialized) { return; }
    printf("sendcontrol %u\n", control);
    _midi.sendControlChange(control, value, channel + 1);
}

void AppleMidi::sendSysEx(std::span<const uint8_t> sysex) {
  if (sysex.size() == 0) { return; }
  _midi.sendSysEx(sysex.size(), sysex.data(), true);
}

} // namespace tocata
