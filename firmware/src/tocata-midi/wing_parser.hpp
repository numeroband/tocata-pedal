#pragma once

#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <functional>
#include <array>

namespace tocata::midi {

class WingParser
{
public:
  using Hash = uint32_t;
  using NoContent = std::byte;
  using Content = std::variant<NoContent, Hash, int32_t, float, const char*>;
  using Callback = std::function<void(Hash hash, Content content)>;

  WingParser(Callback callback) : _callback(callback) {}

  void parse(uint8_t db)
  {
    if (db == NRP_ESCAPE_CODE && !_escf) {
      _escf = true;
    } else {
      if (_escf) {
        if (db != NRP_ESCAPE_CODE) {
          _escf = false;
          if (db == NRP_ESCAPE_CODE - 1) {
            db = NRP_ESCAPE_CODE;
          } else if (db >= NRP_CHANNEL_ID_BASE &&
                  db < NRP_CHANNEL_ID_BASE + NRP_NUM_CHANNELS) {
            _ch_id_rx = db - NRP_CHANNEL_ID_BASE;
            return;
          } else if (_ch_id_rx == 1) {
            parseFiltered(NRP_ESCAPE_CODE);
          }
        }
      }
      if (_ch_id_rx == 1) {
        parseFiltered(db);
      } 
    }
  }

private:
  static constexpr uint8_t NRP_ESCAPE_CODE = 0xdf;
  static constexpr uint8_t NRP_CHANNEL_ID_BASE = 0xd0;
  static constexpr uint8_t NRP_NUM_CHANNELS = 14;

  struct Range {
    uint8_t start;
    uint8_t size = 1;
    constexpr bool in(uint8_t value) const { return value >= start && value < (start + size); }
  };

  struct Partial {
    size_t size;
    size_t offset;
    Content content;
    
    const char* type() {
      if (std::holds_alternative<NoContent>(content)) {
        return "NoContent";
      }
      if (std::holds_alternative<Hash>(content)) {
        return "Hash";
      }
      if (std::holds_alternative<int32_t>(content)) {
        return "int";
      }
      if (std::holds_alternative<float>(content)) {
        return "float";
      }
      if (std::holds_alternative<const char*>(content)) {
        return "string";
      }
      return "Unknown";
    }

    Partial() : size{0}, offset{0}, content{NoContent(0)} {}
    Partial(void* buffer, size_t size) : size{size}, offset{0}, content{static_cast<const char*>(buffer)} {
      static_cast<char*>(buffer)[size] = '\0';
    }
    Partial(size_t size, size_t offset, Content content) : size{size}, offset{offset}, content{content} {}

    template<typename T>
    Partial(T value) : size{sizeof(T)}, offset{sizeof(T)}, content{value} {}

    operator bool() { return std::holds_alternative<NoContent>(content); }
    operator bool() const { return std::holds_alternative<NoContent>(content); }

    bool pending() { return size != offset; }

    void add(uint8_t value) {
      uint8_t* ptr = nullptr;
      size_t idx = 0;
      if (std::holds_alternative<const char*>(content)) {
        ptr = reinterpret_cast<uint8_t*>(const_cast<char*>(std::get<const char*>(content)));
        idx = offset;
      } else if (std::holds_alternative<Hash>(content)) {
        ptr = reinterpret_cast<uint8_t*>(&std::get<Hash>(content));
        idx = size - 1 - offset;
      } else if (std::holds_alternative<int32_t>(content)) {
        ptr = reinterpret_cast<uint8_t*>(&std::get<int32_t>(content));
        idx = size - 1 - offset;
      } else if (std::holds_alternative<float>(content)) {
        ptr = reinterpret_cast<uint8_t*>(&std::get<float>(content));
        idx = size - 1 - offset;
      }
      ptr[idx] = value;
      ++offset;
    }
  };
  
  template<typename T>
  static Partial make_partial() {
    return {sizeof(T), 0, static_cast<T>(0)};
  }

  template<typename T>
  static Partial make_partial(T value) {
    return {sizeof(T), sizeof(T), value};
  }

  using Next = std::function<Partial(Partial& partial)>;
  using PartialNext = std::pair<Partial, Next>;
  using ParseFunc = std::function<PartialNext(uint8_t, void*)>;
  using Msg = std::pair<Range, ParseFunc>;

  PartialNext parseMessage(uint8_t value) {
    static const std::array<Msg, 20> messages = {
      // int 0..63
      Msg{{0x00, 64}, [](uint8_t value, void* buffer) -> PartialNext {
        return {make_partial(int32_t(value)), nullptr};
      }},
      // node index 1..64
      Msg{{0x40, 64}, [](uint8_t value, void* buffer) -> PartialNext {
        return {make_partial(int32_t(value - 0x40 + 1)), nullptr};
      }},
      // string[1..64]
      Msg{{0x80, 64}, [](uint8_t value, void* buffer) -> PartialNext {
        return {{buffer, size_t(value - 0x80 + 1)}, nullptr};
      }},
      // node name[1..16]
      Msg{{0xc0, 16}, [](uint8_t value, void* buffer) -> PartialNext {
        return {{buffer, size_t(value - 0xc0 + 1)}, nullptr};
      }},
      // empty string
      Msg{{0xd0}, [](uint8_t value, void* buffer) -> PartialNext {
        return {{buffer, 0}, nullptr};
      }},
      // string[1..256]
      Msg{{0xd1}, [](uint8_t value, void* buffer) -> PartialNext {
        return {{1, 0, int32_t(0)}, [buffer](Partial& partial) {
          return Partial{buffer, size_t(1 + std::get<int32_t>(partial.content))};
        }};
      }},
      // node index 1..65536
      Msg{{0xd2}, [](uint8_t value, void* buffer) -> PartialNext {
        return {{2, 0, int32_t(0)}, [](Partial& partial) {
          partial.content = int32_t(1 + std::get<int32_t>(partial.content));
          return Partial{};
        }};
      }},
      // int16
      Msg{{0xd3}, [](uint8_t value, void* buffer) -> PartialNext {
        return {{2, 0, int32_t(0)}, [](Partial& partial) {
          auto ptr = reinterpret_cast<int16_t*>(&std::get<int32_t>(partial.content));
          partial.content = int32_t(*ptr);
          return Partial{};
        }};
      }},
      // int32
      Msg{{0xd4}, [](uint8_t value, void* buffer) -> PartialNext {
        return {make_partial<int32_t>(), nullptr};
      }},
      // float
      Msg{{0xd5}, [](uint8_t value, void* buffer) -> PartialNext {
        return {make_partial<int32_t>(), nullptr};
      }},
      // raw float32 (0.0..1.0)
      Msg{{0xd6}, [](uint8_t value, void* buffer) -> PartialNext {
        return {make_partial<int32_t>(), nullptr};
      }},
      // node hash
      Msg{{0xd7}, [](uint8_t value, void* buffer) -> PartialNext {
        return {{}, [](Partial& partial) {
          return make_partial<Hash>();
        }};
      }},
      // click
      Msg{{0xd8}, nullptr},
      // step(inc/dec)
      Msg{{0xd9}, [](uint8_t value, void* buffer) -> PartialNext {
        return {{1, 0, int32_t(0)}, [](Partial& partial) {
          auto ptr = reinterpret_cast<int8_t*>(&std::get<int32_t>(partial.content));
          partial.content = int32_t(*ptr);
          return Partial{};
        }};
      }},
      // node tree: goto root node
      Msg{{0xda}, nullptr},
      // node tree: 1 level up
      Msg{{0xdb}, nullptr},
      // data request
      Msg{{0xdc}, nullptr},
      // request node definition (current node)
      Msg{{0xdd}, nullptr},
      // end of data/def request
      Msg{{0xde}, nullptr},
      // node definition response (word: data length in bytes)
      Msg{{0xdf}, [](uint8_t value, void* buffer) -> PartialNext {
        return {{2, 0, int32_t(0)}, [](Partial& partial) {
          return Partial{size_t(std::get<int32_t>(partial.content)), 0, NoContent(0)};
        }};
      }},
    };


    for (auto& [range, parse] : messages) {
      if (range.in(value)) {
        return parse ? parse(value, _buffer.data()) : PartialNext{{}, nullptr};
      }
    }

    return {};
  }

  void parseFiltered(uint8_t db) {
    // printf("Parse %02X\n", db);
    if (_partial.pending()) {
      _partial.add(db);
    } else  {
      auto[partial, next] = parseMessage(db);
      _partial = partial;
      _next = next;
      // printf("partial %s with %zu/%zu\n", _partial.type(), _partial.offset, _partial.size);
    }

    if (_partial.pending()) {
      return;
    }

    Partial partial = _partial;
    if (_next) {
      _partial = _next(partial);
      _next = nullptr;
      // printf("next partial %s with %zu/%zu\n", _partial.type(), _partial.offset, _partial.size);
    }

    if (std::holds_alternative<NoContent>(partial.content)) {
      // printf("no partial type %s\n", previous.type());
      return;
    }

    if (_partial.pending()) {
      return;
    }

    if (std::holds_alternative<Hash>(partial.content)) {
      _hash = std::get<Hash>(partial.content);
      _partial = {};
      // printf("found hash %08X\n", _hash);
      return;
    }

    _callback(_hash, partial.content);
    _partial = {};
    _next = nullptr;
  }

  Callback _callback;
  bool _escf = false;
  uint8_t _ch_id_rx = 0;
  Partial _partial{};
  Hash _hash = 0;
  Next _next{};
  std::array<uint8_t, 1024> _buffer;
};

}