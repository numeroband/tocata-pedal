#pragma once

#include <config.h>

#include <cstdio>
#include <cstdint>

namespace tocata
{
class WebUsb
{
public:
  static WebUsb& singleton() { return *_singleton; }

  class Delegate
  {
    public:
      virtual void configChanged() = 0;
      virtual void programChanged(uint8_t id) = 0;
  };

  WebUsb(Delegate& delegate) : _delegate(delegate) {}
  void init();
  void run();
  void connected(bool connected) 
  { 
    _connected = connected; 
    reset();
  }
  
private:
  static constexpr size_t kBuffSize = 512;

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

  static constexpr size_t kMaxNamesPerResponse = 
    (kBuffSize - sizeof(Message) - sizeof(GetNamesRes)) / Program::kMaxNameLength;

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

  Delegate& _delegate;
  uint8_t* _out_buf;
  uint32_t _out_pending = 0;
  uint32_t _in_length = 0;
  uint8_t _in_out_buf[kBuffSize];
  bool _connected = false;
};

}