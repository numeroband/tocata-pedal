#include "config_protocol.h"
#include "hal.h"
#include "midi_sysex.h"

namespace tocata
{

void ConfigProtocol::processRequest()
{
  const Message& msg = reinterpret_cast<const Message&>(_in_out_buf);
  // printf("process request %u status %u length %u\n", msg.command, msg.status, msg.length);

  switch (msg.command)
  {
    case kRestart:
      restart();
      break;
    case kBootRom:
      bootRom();
      break;
    case kGetConfig:
      getConfig();
      break;
    case kSetConfig:
      setConfig();
      break;
    case kDeleteConfig:
      deleteConfig();
      break;
    case kGetNames:
      getNames();
      break;
    case kGetProgram:
      getProgram();
      break;
    case kSetProgram:
      setProgram();
      break;
    case kDeleteProgram:
      deleteProgram();
      break;
    case kMemRead:
      memRead();
      break;
    case kMemWrite:
      memWrite();
      break;
    case kFlashErase:
      flashErase();
      break;
    default:
      memmove(_in_out_buf.data() + sizeof(msg), _in_out_buf.data(), _in_out_buf.size() - sizeof(msg));
      sendResponse(_in_out_buf.size() - sizeof(msg), kInvalidCommand);
      break;
  }
}

void ConfigProtocol::restart()
{
  board_reset();
  sendStatus(kOk);
}

void ConfigProtocol::bootRom()
{
  board_program();
  sendStatus(kOk);
}

void ConfigProtocol::getConfig()
{
  Message& msg = reinterpret_cast<Message&>(_in_out_buf);

  GetConfigRes& res = reinterpret_cast<GetConfigRes&>(msg.payload);
  res.config.load();
  sendResponse(sizeof(res));
}

void ConfigProtocol::setConfig()
{
  Message& msg = reinterpret_cast<Message&>(_in_out_buf);
  const SetConfigReq& req = reinterpret_cast<const SetConfigReq&>(msg.payload);

  req.config.save();
  sendStatus(kOk);
  _delegate.configChanged();
}

void ConfigProtocol::deleteConfig()
{
  Message& msg = reinterpret_cast<Message&>(_in_out_buf);

  Config::remove();
  sendStatus(kOk);
  _delegate.configChanged();
}

void ConfigProtocol::getNames()
{
  Message& msg = reinterpret_cast<Message&>(_in_out_buf);
  const GetNamesReq& req = reinterpret_cast<const GetNamesReq&>(msg.payload);
  if (msg.length < sizeof(req))
  {
    sendStatus(kInvalidLength);
    return;
  }

  if (req.id >= Program::kMaxPrograms)
  {
    sendStatus(kInvalidProgramId);
    return;
  }

  GetNamesRes& res = reinterpret_cast<GetNamesRes&>(msg.payload);
  res.from_id = req.id;
  const uint8_t remaining = Program::kMaxPrograms - req.id;
  res.num_names = (kMaxNamesPerResponse < remaining) ? kMaxNamesPerResponse : remaining;
  for (uint8_t i = 0; i < res.num_names; ++i)
  {
    Program::copyName(req.id + i, res.names[i]);
  }

  sendResponse(sizeof(res) + kMaxNamesPerResponse * sizeof(res.names[0]));
}

void ConfigProtocol::getProgram()
{
  Message& msg = reinterpret_cast<Message&>(_in_out_buf);
  const GetProgramReq& req = reinterpret_cast<const GetProgramReq&>(msg.payload);
  if (msg.length < sizeof(req))
  {
    sendStatus(kInvalidLength);
    return;
  }

  if (req.id >= Program::kMaxPrograms)
  {
    sendStatus(kInvalidProgramId);
    return;
  }

  GetProgramRes& res = reinterpret_cast<GetProgramRes&>(msg.payload);
  res.id = req.id;
  res.program.load(req.id);

  sendResponse(sizeof(res));
}

void ConfigProtocol::setProgram()
{
  Message& msg = reinterpret_cast<Message&>(_in_out_buf);
  const SetProgramReq& req = reinterpret_cast<const SetProgramReq&>(msg.payload);
  if (msg.length < sizeof(req))
  {
    sendStatus(kInvalidLength);
    return;
  }

  if (req.id >= Program::kMaxPrograms)
  {
    sendStatus(kInvalidProgramId);
    return;
  }

  req.program.save(req.id);
  sendStatus(kOk);
  _delegate.programChanged(req.id);
}

void ConfigProtocol::deleteProgram()
{
  Message& msg = reinterpret_cast<Message&>(_in_out_buf);
  const DeleteProgramReq& req = reinterpret_cast<const DeleteProgramReq&>(msg.payload);
  if (msg.length < sizeof(req))
  {
    sendStatus(kInvalidLength);
    return;
  }

  if (req.id >= Program::kMaxPrograms)
  {
    sendStatus(kInvalidProgramId);
    return;
  }

  Program::remove(req.id);
  sendStatus(kOk);
  _delegate.programChanged(req.id);
}

void ConfigProtocol::memRead()
{
  Message& msg = reinterpret_cast<Message&>(_in_out_buf);
  const MemReadReq& req = reinterpret_cast<const MemReadReq&>(msg.payload);
  if (msg.length < sizeof(req))
  {
    sendStatus(kInvalidLength);
    return;
  }

  MemReadRes& res = reinterpret_cast<MemReadRes&>(msg.payload);
  if (req.length + sizeof(msg) + sizeof(res) > sizeof(_in_out_buf))
  {
    sendStatus(kInvalidPayloadLength);
    return;
  }

  uint32_t length = req.length;
  void* src = reinterpret_cast<void*>(req.address);
  res.address = req.address;
  res.length = length;
  if (res.address >= kFlashAddress && res.address < (kFlashAddress + kFlashSize))
  {
    flash_read(res.address - kFlashAddress, res.payload, length);
  }
  else
  {
    memcpy(res.payload, src, req.length);
  }

  sendResponse(sizeof(res) + length);
}

void ConfigProtocol::memWrite()
{
  Message& msg = reinterpret_cast<Message&>(_in_out_buf);
  const MemWriteReq& req = reinterpret_cast<const MemWriteReq&>(msg.payload);
  if (msg.length < sizeof(req))
  {
    sendStatus(kInvalidLength);
    return;
  }

  if (req.length + sizeof(msg) + sizeof(req) > sizeof(_in_out_buf))
  {
    sendStatus(kInvalidPayloadLength);
    return;
  }

  void* dst = reinterpret_cast<void*>(req.address);
  if (req.address >= kFlashAddress && req.address < (kFlashAddress + kFlashSize))
  {
    if (req.address + req.length >= (kFlashAddress + kFlashSize)) {
      sendStatus(kInvalidPayloadLength);
      return;
    }
    flash_write(req.address - kFlashAddress, req.payload, req.length);
  }
  else
  {
    memcpy(dst, req.payload, req.length);
  }

  sendStatus(kOk);
}

void ConfigProtocol::flashErase()
{
  Message& msg = reinterpret_cast<Message&>(_in_out_buf);
  const FlashEraseReq& req = reinterpret_cast<const FlashEraseReq&>(msg.payload);
  if (msg.length < sizeof(req))
  {
    sendStatus(kInvalidLength);
    return;
  }

  if (req.address < kFlashAddress || req.address >= (kFlashAddress + kFlashSize))
  {
    sendStatus(kInvalidAddress);
    return;
  }

  if (req.address + req.length >= (kFlashAddress + kFlashSize)) {
    sendStatus(kInvalidPayloadLength);
    return;
  }

  if (req.length > kFlashSize)
  {
    sendStatus(kInvalidPayloadLength);
    return;
  }

  uint32_t length = (req.length + kFlashSectorSize - 1) & ~(kFlashSectorSize - 1);
  flash_erase(req.address - kFlashAddress, length);
  sendStatus(kOk);
}

void ConfigProtocol::sendResponse(uint16_t length, Status status)
{
  Message& msg = *reinterpret_cast<Message*>(_in_out_buf.data());
  msg.length = length;
  msg.status = status;

  _out_buf = _in_out_buf.data();
  _out_pending = sizeof(Message) + msg.length;
}


std::span<const uint8_t> ConfigProtocol::processSysEx(std::span<const uint8_t> sysex, std::span<uint8_t> buffer, uint8_t channel) {
  MidiSysExParser parser;
  if (!parser.init(sysex, channel)) {
    return {};
  }

  _in_length = uint32_t(parser.read(_in_out_buf));
  processRequest();
  if (_out_pending == 0) {
    return {};
  }
  MidiSysExWriter writer;
  writer.init(buffer, channel);
  writer.write({_out_buf, _out_pending});
  writer.finish();
  _out_pending = 0;
  return writer.buffer();
}

}
