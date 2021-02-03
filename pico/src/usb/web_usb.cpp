#include "web_usb.h"

extern "C" {
#include "usb_descriptors.h"
#include <pico/bootrom.h>
#include <hardware/watchdog.h>

#define URL  "numeroband.github.io/tocata-pedal"

const tusb_desc_webusb_url_t desc_url =
{
  3 + sizeof(URL) - 1, // bLength
  3, // WEBUSB URL type
  1, // 0: http, 1: https
  URL,
};

// Invoked when received VENDOR control request
bool tud_vendor_control_request_cb(uint8_t rhport, tusb_control_request_t const * request)
{
  return tocata::WebUsb::singleton().controlRequestCb(rhport, request);
}

// Invoked when DATA Stage of VENDOR's request is complete
bool tud_vendor_control_complete_cb(uint8_t rhport, tusb_control_request_t const * request)
{
  (void) rhport;
  (void) request;

  // nothing to do
  return true;
}

}

namespace tocata
{

WebUsb* WebUsb::_singleton;

void WebUsb::init()
{
  assert(!_singleton);
  _singleton = this;
}

bool WebUsb::controlRequestCb(uint8_t rhport, tusb_control_request_t const * request)
{
  switch (request->bRequest)
  {
    case VENDOR_REQUEST_WEBUSB:
      // match vendor request in BOS descriptor
      // Get landing page url
      return tud_control_xfer(rhport, request, (void*)&desc_url, desc_url.bLength);

    case VENDOR_REQUEST_MICROSOFT:
      if (request->wIndex != 7)
      {
        return false;
      }
       
      // Get Microsoft OS 2.0 compatible descriptor
      uint16_t total_len;
      memcpy(&total_len, desc_ms_os_20 + 8, 2);

      return tud_control_xfer(rhport, request, (void*)desc_ms_os_20, total_len);

    case CDC_REQUEST_SET_CONTROL_LINE_STATE:
      _connected = (request->wValue != 0);
      reset();

      // response with status OK
      return tud_control_status(rhport, request);

    default:
      // stall unknown request
      return false;
  }
}

void WebUsb::sendData()
{
  if (!_out_pending || !tud_vendor_write_available())
  {
    return;
  }

  uint32_t count = tud_vendor_write(_out_buf, _out_pending);
  _out_buf += count;
  _out_pending -= count;
}

void WebUsb::receiveData()
{  
  while (_out_pending == 0 && tud_vendor_available())
  {
    const Message& msg = reinterpret_cast<const Message&>(_in_out_buf);
    uint32_t count = tud_vendor_read(_in_out_buf + _in_length, sizeof(_in_out_buf) - _in_length);
    if (count == 0)
    {
      return;
    }

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
  switch (msg.command)
  {
    case kRestart:
      restart();      
      break;
    case kFirmwareUpgrade:
      firmwareUpgrade();
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
    default:
      memmove(_in_out_buf + sizeof(msg), _in_out_buf, sizeof(_in_out_buf) - sizeof(msg));
      sendResponse(sizeof(_in_out_buf) - sizeof(msg), kInvalidCommand);
      break;
  }
}

void WebUsb::restart()
{
  watchdog_enable(50, 0);
  sendStatus(kOk);
}

void WebUsb::firmwareUpgrade()
{
  add_alarm_in_ms(50, [](alarm_id_t, void *) {
    reset_usb_boot(0, 1);
    return 0LL;
  }, NULL, false);
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
}

void WebUsb::deleteConfig()
{
  Message& msg = reinterpret_cast<Message&>(_in_out_buf);

  Config::remove();

  sendStatus(kOk);
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