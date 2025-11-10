#pragma once

#include "display_sim.h"

namespace tocata {

class DisplaySimSH1106 : public DisplaySim
{
public:
    bool processTransfer(const uint8_t* transfer, uint32_t len) override
    {
        if (len < 2)
        {
            return false;
        }

        _modified = true;

        switch (transfer[0])
        {
            case kCommand:
                return processCommand(transfer + 1, len - 1);
            case kData:
                return processData(transfer + 1, len - 1);
            default:
                return false;
        }
    }

    void refresh(uint32_t* screen, size_t hw_cols, uint32_t* colors) override
    {
        if (!_modified)
        {
            return;
        }

        _modified = false;
        for (uint8_t page = 0; page < kPages; ++page)
        {
            for (uint8_t col = 0; col < hw_cols; ++col)
            {
                uint8_t data = _ram[page][col];
                for (uint32_t row = 0; row < 8; ++row)
                {
                    screen[(page * 8 + row) * hw_cols + col] = colors[(data & 1)];
                    data = data >> 1;
                }
            }
        }
    }

    void setControlData(bool isData) override {}
    size_t numRows() const override { return kRows; }
    size_t numColumns() const override { return kCols; }

private:
    static constexpr uint8_t kCols = 132;
    static constexpr uint8_t kRows = 64;
    static constexpr uint8_t kPages = kRows / 8;

    enum TransferType
    {
        kCommand = 0x00,
        kData = 0x40,
    };

    struct Command
    {
        using CommandCb = void (DisplaySimSH1106::*)(uint8_t);
        uint8_t start;
        uint8_t end;
        CommandCb function;
        bool doubleCmd;
    };

    void ignore(uint8_t cmd) {}
    void lowerColAddr(uint8_t cmd) { _colAddr = (_colAddr & 0xF0) | (cmd & 0x0F); }
    void higherColAddr(uint8_t cmd) { _colAddr = (_colAddr & 0x0F) | (cmd & 0xF0) << 4; }
    void startLine(uint8_t cmd) { _startLine = cmd - 0x40; }
    void pageAddr(uint8_t cmd) { _page = cmd - 0xB0; }

    static constexpr Command kCommands[] = {
        {0x00, 0x0F, &DisplaySimSH1106::lowerColAddr},
        {0x10, 0x1F, &DisplaySimSH1106::higherColAddr},
        {0x20, 0x20, &DisplaySimSH1106::ignore, true},
        {0x2e, 0x2e, &DisplaySimSH1106::ignore},
        {0x40, 0x7F, &DisplaySimSH1106::startLine},
        {0x8d, 0x8d, &DisplaySimSH1106::ignore, true},
        {0x81, 0x81, &DisplaySimSH1106::ignore, true},
        {0xA0, 0xA1, &DisplaySimSH1106::ignore},
        {0xA4, 0xA5, &DisplaySimSH1106::ignore},
        {0xA6, 0xA7, &DisplaySimSH1106::ignore},
        {0xA8, 0xA8, &DisplaySimSH1106::ignore, true},
     // {0xAD, 0xAD, &DisplaySimSH1106::ignore, true},
        {0xAE, 0xAF, &DisplaySimSH1106::ignore},
        {0xB0, 0xB7, &DisplaySimSH1106::pageAddr},
        {0xC0, 0xC8, &DisplaySimSH1106::ignore},
        {0xD3, 0xD3, &DisplaySimSH1106::ignore, true},
        {0xD5, 0xD5, &DisplaySimSH1106::ignore, true},
        {0xD9, 0xD9, &DisplaySimSH1106::ignore, true},
        {0xDA, 0xDA, &DisplaySimSH1106::ignore, true},
        {0xDB, 0xDB, &DisplaySimSH1106::ignore, true},
     // {0xE3, 0xE3, &DisplaySimSH1106::ignore},
    };

    bool processCommand(const uint8_t* buf, uint32_t len)
    {
        if (len != 1)
        {
            return false;
        }

        uint8_t cmd = buf[0];

        if (_pendingCmd)
        {
            (this->*_pendingCmd->function)(cmd);
            _pendingCmd = nullptr;
            return true;
        }

        for (int i = 0; i < sizeof(kCommands) / sizeof(kCommands[0]); ++i)
        {
            const Command& cmdDef = kCommands[i];
            if (cmd >= cmdDef.start && cmd <= cmdDef.end)
            {
                (this->*cmdDef.function)(cmd);
                if (cmdDef.doubleCmd)
                {
                    _pendingCmd = &cmdDef;
                }
                return true;
            }
        }

        return false;
    }

    bool processData(const uint8_t* buf, uint32_t len)
    {
        for (uint32_t i = 0; i < len; ++i)
        {
            _ram[_page][_colAddr++] = buf[i];
            assert(_colAddr <= kCols);
        }
        return true;
    }

    uint8_t _ram[kPages][kCols] = {};
    uint8_t _colAddr = 0;
    uint8_t _startLine = 0;
    uint8_t _page = 0;
    uint8_t _lastCmd;
    const Command* _pendingCmd = nullptr;
    bool _modified = false;
};

}
