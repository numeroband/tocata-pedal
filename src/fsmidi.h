#pragma once

#include <functional>

namespace tocata {

class FsMidi
{
public:
	FsMidi(const char* name);
	void begin();
	void loop();
	void setOnConnect(std::function<void()> on_connect) { _on_connect = on_connect; };
	void setOnDisconnect(std::function<void()> on_disconnect) { _on_disconnect = on_disconnect; };
	bool connected() { return _connected; }
	void sendProgram(uint8_t program);
	void sendControl(uint8_t control, uint8_t value);

private:
	static FsMidi* _singleton;
	struct Impl;
	Impl* _impl;
	bool _connected;
	std::function<void()> _on_connect;
	std::function<void()> _on_disconnect;
};

}