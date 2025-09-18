#pragma once

#include <span>
#include <array>
#include <cstdint>
#include <algorithm>

namespace tocata {

constexpr std::array<uint8_t, 4> kMidiSysExPrefix = {0xF0, 0x00, 0x2F, 0x7F};
constexpr std::array<uint8_t, 1> kMidiSysExSuffix = {0xF7};
constexpr auto kMidiSysExMinSize = kMidiSysExPrefix.size() + kMidiSysExSuffix.size();

class MidiSysExWriter {
public:
    static constexpr size_t bytesRequired(size_t content) {
        auto total_bits = content * 8;
        return kMidiSysExMinSize + (total_bits + 6) / 7;
    }

    bool init(std::span<uint8_t> buffer) {
        if (buffer.size() < kMidiSysExMinSize) {
            return false;
        }

        _buffer = buffer;
        std::copy(kMidiSysExPrefix.begin(), kMidiSysExPrefix.end(), _buffer.begin());
        _offset = kMidiSysExPrefix.size();
        _buffer[_offset] = 0;
        _bits = 0;
        
        return true;
    }

    void finish() {
        if (_bits > 0) {
            ++_offset;
        }
        std::copy(kMidiSysExSuffix.begin(), kMidiSysExSuffix.end(), _buffer.begin() + _offset);
        _offset += kMidiSysExSuffix.size();
    }

    size_t write(std::span<const uint8_t> input) {
        size_t written = 0;
        for (auto b : input) {
            if (!write(b)) {
                break;
            }
            ++written;
        }
        return written;
    }

    size_t size() const { return _offset; }
    std::span<const uint8_t> buffer() { return {_buffer.data(), size()}; }
    
    size_t available() {
        auto in_buffer = _buffer.size() - _offset;
        if (in_buffer < kMidiSysExSuffix.size()) {
            return 0;
        }
        
        in_buffer -= kMidiSysExSuffix.size();
        if (in_buffer == 0) {
            return 0;
        }

        auto total_bits = (7 - _bits) + ((in_buffer - 1) * 7);
        return total_bits / 8;
    }
    
    void reset() {
        _buffer = {};
        _offset = 0;
        _bits = 0;
    }
    
    operator bool() {
        return _buffer.size() != 0;
    }

private:
    bool write(uint8_t input) {
        if (_offset + kMidiSysExSuffix.size() >= _buffer.size()) {
            return false;
        }
        
        _buffer[_offset++] |= input >> ++_bits;
        _buffer[_offset] = ~0x80 & (input << (7 - _bits));
        if (_bits == 7) {
            _bits = 0;
            _buffer[++_offset] = 0;
        }
    
        return true;
    }
    
    std::span<uint8_t> _buffer{};
    size_t _bits{};
    size_t _offset{};
};

class MidiSysExParser {
public:
    bool init(std::span<const uint8_t> buffer) {
        if (buffer.size() < kMidiSysExMinSize || 
            !std::equal(kMidiSysExPrefix.begin(), kMidiSysExPrefix.end(), buffer.begin()) ||
            !std::equal(kMidiSysExSuffix.begin(), kMidiSysExSuffix.end(), buffer.end() - kMidiSysExSuffix.size())
        ) {
            return false;
        }

        _buffer = {buffer.data() + kMidiSysExPrefix.size(), buffer.size() - kMidiSysExMinSize};
        _offset = 0;
        _bits = 0;

        return true;
    }

    size_t read(std::span<uint8_t> output) {
        size_t bytes_read = 0;
        for (auto& b : output) {
            if (!read(b)) {
                break;
            }
            ++bytes_read;
        }
        return bytes_read;
    }
    
    size_t available() {
        auto in_buffer = _buffer.size() - _offset;
        if (in_buffer == 0) {
            return 0;
        }
        auto total_bits = (7 - _bits) + ((in_buffer - 1) * 7);
        return total_bits / 8;
    }

    void reset() {
        _buffer = {};
        _offset = 0;
        _bits = 0;
    }

    operator bool() {
        return _buffer.size() != 0;
    }

private:
    bool read(uint8_t& b) {
        if (_offset + 1 >= _buffer.size()) {
            return false;
        }
        
        b = _buffer[_offset++] << ++_bits;
        b |= _buffer[_offset] >> (7 - _bits);
        if (_bits == 7) {
            _bits = 0;
            ++_offset;
        }
        return true;
    }

    std::span<const uint8_t> _buffer;
    size_t _bits{};
    size_t _offset{};
};

constexpr auto kMidiSysExMaxSize = MidiSysExWriter::bytesRequired(512);

}
