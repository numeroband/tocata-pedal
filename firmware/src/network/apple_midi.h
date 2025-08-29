#pragma once

#define APPLEMIDI_INITIATOR
#include <Ethernet.h>
#include <AppleMIDI.h>

#include <cstdint>
#include <midi_sender.h>


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

private:
    using MidiSession = APPLEMIDI_NAMESPACE::AppleMIDISession<EthernetUDP>;
    using MidiSettings = APPLEMIDI_NAMESPACE::AppleMIDISettings;
    using MidiInterface = MIDI_NAMESPACE::MidiInterface<MidiSession, MidiSettings>;
    MidiSession _midi_session{"TocataPedal", DEFAULT_CONTROL_PORT};
    MidiInterface _midi{_midi_session};
    size_t _connected{0};
};

}