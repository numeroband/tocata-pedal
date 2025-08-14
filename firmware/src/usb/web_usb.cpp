#include "web_usb.h"
#include "hal.h"

#include <cassert>

namespace tocata
{

WebUsb* WebUsb::_singleton;

void WebUsb::init()
{
  assert(!_singleton);
  _singleton = this;
}

void WebUsb::sendData()
{
  if (!_out_pending || !usb_vendor_write_available())
  {
    return;
  }

  uint32_t count = usb_vendor_write(_out_buf, _out_pending);
  _out_buf += count;
  _out_pending -= count;
  if (_out_pending == 0) {
    usb_vendor_write_flush();
  }
}

void WebUsb::receiveData()
{  
  while (_out_pending == 0 && usb_vendor_available())
  {
    const Message& msg = reinterpret_cast<const Message&>(_in_out_buf);
    uint32_t count = usb_vendor_read(_in_out_buf + _in_length, sizeof(_in_out_buf) - _in_length);
    if (count == 0)
    {
      return;
    }

    printf("USB Read %u bytes\n", count);
    _in_length += count;

    if (_in_length >= sizeof(Message) && _in_length >= (sizeof(Message) + msg.length))
    {
      _in_length = 0;
      processRequest();
    }
  }
}

void WebUsb::run(void)
{
  if (!_connected)
  {
    return;
  }

  receiveData();
  sendData();
}

void WebUsb::reset()
{
  _in_length = 0;
  _out_pending = 0;
}

void WebUsb::processRequest()
{
  const Message& msg = reinterpret_cast<const Message&>(_in_out_buf);
  printf("process request %u status %u length %u\n", msg.command, msg.status, msg.length);

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
      memmove(_in_out_buf + sizeof(msg), _in_out_buf, sizeof(_in_out_buf) - sizeof(msg));
      sendResponse(sizeof(_in_out_buf) - sizeof(msg), kInvalidCommand);
      break;
  }
}

void WebUsb::restart()
{
  board_reset();
  sendStatus(kOk);
}

void WebUsb::bootRom()
{
  board_program();
  sendStatus(kOk);
}

void WebUsb::getConfig()
{
  Message& msg = reinterpret_cast<Message&>(_in_out_buf);

  GetConfigRes& res = reinterpret_cast<GetConfigRes&>(msg.payload);
  res.config.load();
  sendResponse(sizeof(res));
}

void WebUsb::setConfig()
{
  Message& msg = reinterpret_cast<Message&>(_in_out_buf);
  const SetConfigReq& req = reinterpret_cast<const SetConfigReq&>(msg.payload);

  req.config.save();
  sendStatus(kOk);
  _delegate.configChanged();
}

void WebUsb::deleteConfig()
{
  Message& msg = reinterpret_cast<Message&>(_in_out_buf);

  Config::remove();
  sendStatus(kOk);
  _delegate.configChanged();
}

void WebUsb::getNames()
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

void WebUsb::getProgram()
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

void WebUsb::setProgram()
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

void WebUsb::deleteProgram()
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

void WebUsb::memRead()
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

void WebUsb::memWrite()
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

void WebUsb::flashErase()
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

void WebUsb::sendResponse(uint16_t length, Status status)
{
  Message& msg = reinterpret_cast<Message&>(_in_out_buf);
  msg.length = length;
  msg.status = status;

  _out_buf = _in_out_buf;
  _out_pending = sizeof(Message) + msg.length;
}

}