#pragma once

#include <stdint.h>
#include <cassert>
#include "hal.h"
#include <array>
#include <vector>

namespace tocata {

class DisplaySim
{
public:
    bool processTransfer(const uint8_t* transfer, uint32_t len)
    {
        _modified = true;

        switch (_transferType)
        {
            case kCommand:
                return processCommand(transfer, len);
            case kData:
                return processData(transfer, len);
            default:
                return false;
        }
    }

    void setControlData(bool isData) {
        _transferType = isData ? kData : kCommand;
    }

    void refresh(uint32_t* screen, size_t hw_cols, uint32_t* colors)
    {
        if (!_modified)
        {
            return;
        }

        _modified = false;
        for (size_t row = 0; row < kRows; ++row) {
            for (size_t col = 0; col < kCols; ++col) {
                auto data = _ram[row][col];
                screen[row * hw_cols + col] = colors[(data & 1)];
            }
        }
    }
    
    size_t numRows() const { return kRows; }
    size_t numColumns() const { return kCols; }

private:
    static constexpr size_t kCols = 256;
    static constexpr size_t kRows = 64;

    enum TransferType
    {
        kCommand = 0,
        kData = 1,
    };

    struct Command
    {
        using CommandCb = void (DisplaySim::*)();
        uint8_t cmd;
        CommandCb function;
        uint8_t numBytes;
    };

    void ignore() {
//        printf("[%02X] ", _pendingCmd->cmd);
//        for (auto b : _data) {
//            printf("%2X ", b);
//        }
//        printf("\n");
    }
    
    void setColumn() {
        _colsRange = {_data[0], _data[1]};
        _col = _colsRange.start;
//       printf("\ncol %zu-%zu\n", _colsRange.start, _colsRange.end);
    }
    void setRow() {
        _rowsRange = {_data[0], _data[1]};
        _row = _rowsRange.start;
//       printf("\nrow %zu-%zu\n", _rowsRange.start, _rowsRange.end);
    }
    void writeData() {
//       printf("%02X %02X ", _data[0], _data[1]);
        size_t col = (_col - 28) * 4;
        _ram[_row][col + 0] = _data[0] >> 4;
        _ram[_row][col + 1] = _data[0] & 0x0F;
        _ram[_row][col + 2] = _data[1] >> 4;
        _ram[_row][col + 3] = _data[1] & 0x0F;
        increase();
    }
    void increase() {
        ++_col;
        if (_col > _colsRange.end) {
            _col = _colsRange.start;
            ++_row;
        }
        if (_row > _rowsRange.end) {
            _row = _rowsRange.start;
        }
    }

    static constexpr std::array<Command, 23> kCommands = {
        Command{0x15, &DisplaySim::setColumn, 2},
        Command{0x5C, &DisplaySim::writeData, 2},
        Command{0x75, &DisplaySim::setRow, 2},
        Command{0xA0, &DisplaySim::ignore, 2},
        Command{0xA1, &DisplaySim::ignore, 1},
        Command{0xA2, &DisplaySim::ignore, 1},
        Command{0xA6, &DisplaySim::ignore, 0},
        Command{0xA9, &DisplaySim::ignore, 0},
        Command{0xAB, &DisplaySim::ignore, 1},
        Command{0xAE, &DisplaySim::ignore, 0},
        Command{0xAF, &DisplaySim::ignore, 0},
        Command{0xB1, &DisplaySim::ignore, 1},
        Command{0xB3, &DisplaySim::ignore, 1},
        Command{0xB4, &DisplaySim::ignore, 2},
        Command{0xB6, &DisplaySim::ignore, 1},
        Command{0xB9, &DisplaySim::ignore, 0},
        Command{0xBB, &DisplaySim::ignore, 1},
        Command{0xBE, &DisplaySim::ignore, 1},
        Command{0xC1, &DisplaySim::ignore, 1},
        Command{0xC7, &DisplaySim::ignore, 1},
        Command{0xCA, &DisplaySim::ignore, 1},
        Command{0xD1, &DisplaySim::ignore, 2},
        Command{0xFD, &DisplaySim::ignore, 1},
    };

    bool processCommand(const uint8_t* buf, uint32_t len)
    {
        if (len != 1)
        {
            return false;
        }
        
        assert(_data.size() == 0);

        uint8_t cmd = buf[0];
        _pendingCmd = nullptr;
        for (auto& command : kCommands) {
            if (command.cmd == cmd) {
                _pendingCmd = &command;
            }
        }
        assert(_pendingCmd);
        
        if (_pendingCmd->numBytes == 0) {
            (this->*_pendingCmd->function)();
        }

        return true;
    }

    bool processData(const uint8_t* buf, uint32_t len)
    {
        assert(_pendingCmd);
        for (uint32_t i = 0; i < len; ++i) {
            _data.push_back(buf[i]);
            if (_data.size() == _pendingCmd->numBytes) {
                (this->*_pendingCmd->function)();
                _data.clear();
            }
        }
        
        return true;
    }

    struct Range {
        size_t start;
        size_t end;
    };

    TransferType _transferType = kCommand;
    std::array<std::array<uint8_t, kCols>, kRows> _ram{};
    Range _rowsRange{};
    Range _colsRange{};
    size_t _row = 0;
    size_t _col = 0;
    const Command* _pendingCmd = nullptr;
    bool _modified = false;
    std::vector<uint8_t> _data;
};

}
