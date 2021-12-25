#include "hal.h"

#ifdef HAL_PICO

extern "C" {

#include "usb_device.h"

// Include C implementation
#include "usb_descriptors.c"

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
  tocata::UsbDevice::singleton().mount();
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
  tocata::UsbDevice::singleton().umount();
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void) remote_wakeup_en;
  tocata::UsbDevice::singleton().suspend();
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
  tocata::UsbDevice::singleton().resume();
}

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
      tocata::WebUsb::singleton().connected(request->wValue != 0);

      // response with status OK
      return tud_control_status(rhport, request);

    default:
      // stall unknown request
      return false;
  }
}

// Invoked when DATA Stage of VENDOR's request is complete
bool tud_vendor_control_complete_cb(uint8_t rhport, tusb_control_request_t const * request)
{
  (void) rhport;
  (void) request;

  // nothing to do
  return true;
}

} // extern "C"

namespace tocata {

bool leds_fix_mapping()
{
  static pico_unique_board_id_t swapped_board_id = {.id = {0xE6, 0X60, 0X38, 0XB7, 0X13, 0X90, 0X7E, 0X33}};
  pico_unique_board_id_t board_id{};
  pico_get_unique_board_id(&board_id);
  return (memcmp(&board_id, &swapped_board_id, sizeof(board_id)) != 0);
}

static char nibble_to_hex(uint8_t nibble)
{
  if (nibble < 10)
  {
    return '0' + nibble;
  }
  else
  {
    return 'A' + (nibble - 10);
  }
}

static void bytes_to_hex(char* dst, uint8_t* bytes, size_t length)
{
  for (size_t i = 0; i < length; ++i)
  {
    dst[2 * i] = nibble_to_hex(bytes[i] >> 4);
    dst[2 * i + 1] = nibble_to_hex(bytes[i] & 0xF);
  }
  dst[2 * length] = '\0';
}

#if MEMFLASH
uint8_t MemFlash[kFlashSize];
#endif

void usb_init()
{
  static struct stdio_driver usb_stdio = {
    .out_chars = [](const char *buf, int len) {
      int sent = 0;
      bool blocked = false;
      while (sent < len)
      {
        while (!tud_cdc_write_available())
        {
          tud_task();
        }
        int written = tud_cdc_write(buf + sent, len - sent);
        sent += written;
      }
    },
    .out_flush = []() {
      tud_cdc_write_flush();
    },
    .in_chars = [](char *buf, int len) { 
      return (int)tud_cdc_read(buf, (uint32_t)len); 
    },
    .crlf_enabled = PICO_STDIO_DEFAULT_CRLF,
  };

  flash_init();

  pico_unique_board_id_t board_id;
  pico_get_unique_board_id(&board_id);
  bytes_to_hex(usb_serial_number, board_id.id, sizeof(board_id.id));
  tusb_init();
  stdio_set_driver_enabled(&usb_stdio, true);
}

} // namespace tocata

#endif // HAL_PICO