#include "fsmidi.h"

#include <BLEMIDI_Transport.h>
#include <hardware/BLEMIDI_ESP32_NimBLE.h>

namespace tocata {

BLEMIDI_CREATE_DEFAULT_INSTANCE();

struct FsMidi::Impl
{
	BLEMIDI_NAMESPACE::BLEMIDI_Transport<BLEMIDI_NAMESPACE::BLEMIDI_ESP32_NimBLE> ble_midi;
	MIDI_NAMESPACE::MidiInterface<BLEMIDI_NAMESPACE::BLEMIDI_Transport<BLEMIDI_NAMESPACE::BLEMIDI_ESP32_NimBLE>, BLEMIDI_NAMESPACE::MySettings> ble_midi_iface;
	Impl(const char* name) : ble_midi(name), ble_midi_iface(ble_midi) {}
};

FsMidi* FsMidi::_singleton;

FsMidi::FsMidi(const char* name) : _impl(new Impl(name))
{
}

void FsMidi::begin()
{
	assert(!_singleton);
	_singleton = this;

    _impl->ble_midi.setHandleConnected([]{
		_singleton->_connected = true;
		_singleton->_on_connect();
	});

    _impl->ble_midi.setHandleDisconnected([]{
		_singleton->_connected = false;
		_singleton->_on_disconnect();
	});

    // ble_midi_iface_.setHandleNoteOn(OnNoteOn);
    // ble_midi_iface_.setHandleNoteOff(OnNoteOff);

    _impl->ble_midi_iface.begin();
}

void FsMidi::loop()
{
	_impl->ble_midi_iface.read();
}

void FsMidi::sendProgram(uint8_t program)
{
	_impl->ble_midi_iface.sendProgramChange(program, 1);
}

void FsMidi::sendControl(uint8_t control, uint8_t value)
{
	_impl->ble_midi_iface.sendControlChange(control, value, 1);
}

} // namespace tocata