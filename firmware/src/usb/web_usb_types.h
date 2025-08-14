#pragma once

#include "config.h"

#include <cstdio>
#include <cstdint>

namespace tocata::usb {

static constexpr size_t kUsbBuffSize = 512;

struct Message
{
  uint16_t length;
  uint8_t command;
  uint8_t status;
  uint8_t payload[];
};

enum Command
{
  kNone = 0,
  kRestart = 1,
  kBootRom = 2,
  kGetConfig = 3,
  kSetConfig = 4,
  kDeleteConfig = 5,
  kGetNames = 6,
  kGetProgram = 7,
  kSetProgram = 8,
  kDeleteProgram = 9,
  kMemRead = 0x10,
  kMemWrite = 0x11,
  kFlashErase = 0x12,
};

enum Status
{
  kOk = 0,
  kInvalidCommand = 1,
  kInvalidLength = 2,
  kInvalidProgramId = 3,
  kInvalidAddress = 4,
  kInvalidPayloadLength = 5,
};

struct ConfigReqRes
{
  Config config;
} __attribute__((packed));

using ProgramName = char[Program::kMaxNameLength + 1];

struct IdAndProgram
{
  uint8_t id;
  Program program;
} __attribute__((packed));

struct Id 
{
  uint8_t id;
} __attribute__((packed));

struct GetNamesRes
{
  uint8_t from_id;
  uint8_t num_names;
  ProgramName names[];
} __attribute__((packed));

struct AddressAndLength
{
  uint32_t address;
  uint32_t length;
} __attribute__((packed));

struct AddressAndPayload
{
  uint32_t address;
  uint32_t length;
  uint8_t payload[];
} __attribute__((packed));

static constexpr size_t kMaxNamesPerResponse = 
  (kUsbBuffSize - sizeof(Message) - sizeof(GetNamesRes)) / Program::kMaxNameLength;

using GetConfigRes = ConfigReqRes;
using SetConfigReq = ConfigReqRes;
using GetNamesReq = Id;
using GetProgramReq = Id;
using GetProgramRes = IdAndProgram;
using SetProgramReq = IdAndProgram;
using DeleteProgramReq = Id;
using MemReadReq = AddressAndLength;
using MemReadRes = AddressAndPayload;
using MemWriteReq = AddressAndPayload;
using FlashEraseReq = AddressAndLength;

}