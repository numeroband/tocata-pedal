#pragma once

#include <cstdint>
#include <functional>
#include <span>
#include "hal.h"

namespace tocata {

class PollTimer {
public:
	// A default-constructed timer reads as expired (kUnset) so the first poll
	// fires immediately; start()/restart() then arm a real deadline.
	PollTimer() = default;

	// Re-arm keeping a drift-free cadence: when already running, anchor the next
	// deadline to the previous one; otherwise (unset/disabled) anchor to now.
	void restart(uint32_t interval) {
		uint32_t start = (_state == kRunning) ? _deadline : micros();
		_deadline = start + interval;
		_state = kRunning;
	}

	void start(uint32_t interval) {
		_deadline = micros() + interval;
		_state = kRunning;
	}

	bool expired() {
		if (_state != kRunning) {
			// kUnset counts as expired (fire immediately); kDisabled never does.
			return _state == kUnset;
		}
		// Signed difference is wrap-around safe across the ~71.6 min micros()
		// rollover, and _deadline can never collide with a "disabled" sentinel.
		return static_cast<int32_t>(micros() - _deadline) >= 0;
	}

	void wait() {
		while (!expired()) {
		}
	}

	void disable() {
		_state = kDisabled;
	}

private:
	enum State : uint8_t { kUnset, kRunning, kDisabled };
	uint32_t _deadline{0};
	State _state{kUnset};
};

}
