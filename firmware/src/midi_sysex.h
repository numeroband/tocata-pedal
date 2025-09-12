#include <span>
#include <array>
#include <cstdint>
#include <algorithm>

namespace tocata {

constexpr std::array<uint8_t, 4> kSysExPrefix = {0xF0, 0x00, 0x2F, 0x7F};
constexpr std::array<uint8_t, 1> kSysExSuffix = {0xF7};
constexpr auto kSysExMinSize = kSysExPrefix.size() + kSysExSuffix.size();

class MidiSysExWriter {
public:
    static constexpr size_t bytesRequired(size_t content) {
        auto total_bits = content * 8;
        return kSysExMinSize + (total_bits + 6) / 7;
    }

    bool init(std::span<uint8_t> buffer) {
        if (buffer.size() < kSysExMinSize) {
            printf("syses_writer buffer too small\n");
            return false;
        }

        _buffer = buffer;
        std::copy(kSysExPrefix.begin(), kSysExPrefix.end(), _buffer.begin());
        _offset = kSysExPrefix.size();
        _buffer[_offset] = 0;
        _bits = 0;
        
        return true;
    }

    void finish() {
        if (_bits > 0) {
            ++_offset;
        }
        std::copy(kSysExSuffix.begin(), kSysExSuffix.end(), _buffer.begin() + _offset);
        _offset += kSysExSuffix.size();
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

    std::span<const uint8_t> buffer() { return {_buffer.data(), _offset}; }
    
    size_t available() {
        auto in_buffer = _buffer.size() - _offset;
        if (in_buffer < kSysExSuffix.size()) {
            return 0;
        }
        
        in_buffer -= kSysExSuffix.size();
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
        if (_offset + kSysExSuffix.size() >= _buffer.size()) {
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
        if (buffer.size() < kSysExMinSize || 
            !std::equal(kSysExPrefix.begin(), kSysExPrefix.end(), buffer.begin()) ||
            !std::equal(kSysExSuffix.begin(), kSysExSuffix.end(), buffer.end() - kSysExSuffix.size())
        ) {
            return false;
        }

        _buffer = {buffer.data() + kSysExPrefix.size(), buffer.size() - kSysExMinSize};
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

}
