#pragma once

#include <cstdint>
#include <functional>
#include <span>
#include "hal.h"

namespace tocata {

class PollTimer {
public:
	void restart(uint32_t interval) {
		uint32_t start = _deadline ?: millis();
		_deadline = start + interval;
	}

	void start(uint32_t interval) {
		_deadline = millis() + interval;
	}

	bool expired() {
		return millis() >= _deadline;
	}

private:
	uint32_t _deadline = 0;
};

}
