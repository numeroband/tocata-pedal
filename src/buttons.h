#pragma once

#include <Arduino.h>

#include <array>
#include <bitset>
#include <functional>

namespace tocata {

template<std::size_t INS, std::size_t OUTS>
class Buttons
{
public:
    using ButtonsMask = std::bitset<INS * OUTS>;
    using ButtonsCallback = std::function<void(ButtonsMask status, ButtonsMask modified)>;
    using InArray = std::array<uint8_t, INS>;
    using OutArray = std::array<uint8_t, OUTS>;

    Buttons(const InArray& in_gpios, const OutArray& out_gpios) : _in_gpios(in_gpios), _out_gpios(out_gpios) {}
    void begin()
    {
        for (int i = 0; i < _in_gpios.size(); ++i)
        {
            Serial.print("Configuring input ");
            Serial.println(_in_gpios[i]);
            pinMode(_in_gpios[i], INPUT | PULLUP);
        }

        for (int i = 0; i < _out_gpios.size(); ++i)
        {
            Serial.print("Configuring output ");
            Serial.println(_out_gpios[i]);
            pinMode(_out_gpios[i], INPUT);
        }
    }

    void setCallback(ButtonsCallback callback) { _callback = callback; }

    void loop() 
    {
        if (!_callback)
        {
            Serial.println("No buttons callback");
            return;
        }

        ButtonsMask gpios = readGpios();
        if (gpios == _last_state)
        {
            return;    
        }

        ButtonsMask debounce;        
        do {
            debounce = gpios;
            delay(kDebounceMs);
            gpios = readGpios();    
        } while (gpios != debounce);

        if (gpios == _last_state)
        {
            return;    
        }

        _callback(gpios, _last_state ^ gpios);
        _last_state = gpios;
    }    

private:
    static constexpr int kDebounceMs = 50;

    ButtonsMask readGpios()
    {
        ButtonsMask gpios;
        for (int out = 0; out < _out_gpios.size(); ++out)
        {
            pinMode(_out_gpios[out], OUTPUT);
            digitalWrite(_out_gpios[out], LOW);
            delay(1);

            for (int in = 0; in < _in_gpios.size(); ++in)
            {
                int index = out * _in_gpios.size() + in;
                gpios[index] = !digitalRead(_in_gpios[in]);
            }
            pinMode(_out_gpios[out], INPUT);
        }

        return gpios;
    }

    InArray _in_gpios;
    OutArray _out_gpios;
    ButtonsMask _last_state;
    ButtonsCallback _callback;
};

}