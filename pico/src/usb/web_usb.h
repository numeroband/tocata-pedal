#pragma once

#include <cstdio>
#include <cstdint>

extern "C" {
#include <tusb.h>
}

namespace tocata
{
class WebUsb
{
public:
  static WebUsb& singleton() { return *_singleton; }

  void init();
  void run();

  bool controlRequestCb(uint8_t rhport, tusb_control_request_t const * request);

private:
  static constexpr size_t kBuffSize = 512;
  static constexpr size_t kNumPrograms = 99;
  static constexpr size_t kProgramNameSize = 30;

  static WebUsb* _singleton;

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
    kFirmwareUpgrade = 2,
    kGetConfig = 3,
    kSetConfig = 4,
    kDeleteConfig = 5,
    kGetNames = 6,
    kGetProgram = 7,
    kSetProgram = 8,
    kDeleteProgram = 9,
  };

  enum Status
  {
    kOk,
    kInvalidCommand,
    kInvalidLength,
    kInvalidProgramId,
  };

  struct Config
  {
    char _reserved[128];
  };

  struct ConfigReqRes
  {
    Config config;
  } __attribute__((packed));

  using ProgramName = char[kProgramNameSize + 1];

  struct Program
  {
    ProgramName name;
    char _reserved[269];
  };

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

  static constexpr size_t kMaxNamesPerResponse = 
    (kBuffSize - sizeof(Message) - sizeof(GetNamesRes)) / kProgramNameSize;

  using GetConfigRes = ConfigReqRes;
  using SetConfigReq = ConfigReqRes;
  using GetNamesReq = Id;
  using GetProgramReq = Id;
  using GetProgramRes = IdAndProgram;
  using SetProgramReq = IdAndProgram;
  using DeleteProgramReq = Id;

  void sendData();
  void receiveData();
  void reset();
  void processRequest();
  void sendResponse(uint16_t length, Status status = kOk);
  void sendStatus(Status status) { sendResponse(0, status); };
  void restart();
  void firmwareUpgrade();
  void getConfig();
  void setConfig();
  void deleteConfig();
  void getNames();
  void getProgram();
  void setProgram();
  void deleteProgram();

  uint8_t* _out_buf;
  uint32_t _out_pending = 0;
  uint32_t _in_length = 0;
  uint8_t _in_out_buf[kBuffSize];
  bool _connected = false;

  Program _programs[kNumPrograms] = {};
  Config _config = {};
};

}