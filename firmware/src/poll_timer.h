#pragma once

#include <cstdint>
#include <functional>
#include <span>
#include "hal.h"

namespace tocata {

class PollTimer {
public:
	PollTimer(uint32_t deadline = 0) : _deadline{deadline} {}

	void restart(uint32_t interval) {
		uint32_t start = _deadline ?: micros();
		_deadline = start + interval;
	}

	void start(uint32_t interval) {
		_deadline = micros() + interval;
	}

	bool expired() {
		return _deadline != kInvalid && micros() >= _deadline;
	}

	void disable() {
		_deadline = kInvalid;
	}

private:
	static constexpr uint32_t kInvalid = UINT32_MAX;
	uint32_t _deadline{kInvalid};
};

}
