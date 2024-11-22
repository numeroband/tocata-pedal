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


//--------------------------------------------------------------------+
// WebUSB use vendor class
//--------------------------------------------------------------------+

// Invoked when a control transfer occurred on an interface of this class
// Driver response accordingly to the request and the transfer stage (setup/data/ack)
// return false to stall control endpoint (e.g unsupported request)
bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request)
{
  // nothing to with DATA & ACK stage
  if (stage != CONTROL_STAGE_SETUP) return true;

  switch (request->bmRequestType_bit.type)
  {
    case TUSB_REQ_TYPE_VENDOR:
      switch (request->bRequest)
      {
        case VENDOR_REQUEST_WEBUSB:
          // match vendor request in BOS descriptor
          // Get landing page url
          return tud_control_xfer(rhport, request, (void*)(uintptr_t) &desc_url, desc_url.bLength);

        case VENDOR_REQUEST_MICROSOFT:
          if ( request->wIndex == 7 )
          {
            // Get Microsoft OS 2.0 compatible descriptor
            uint16_t total_len;
            memcpy(&total_len, desc_ms_os_20+8, 2);

            return tud_control_xfer(rhport, request, (void*)(uintptr_t) desc_ms_os_20, total_len);
          }else
          {
            return false;
          }

        default: break;
      }
    break;

    case TUSB_REQ_TYPE_CLASS:
      if (request->bRequest == CDC_REQUEST_SET_CONTROL_LINE_STATE)
      {
        // Webserial simulate the CDC_REQUEST_SET_CONTROL_LINE_STATE (0x22) to connect and disconnect.
        tocata::WebUsb::singleton().connected(request->wValue != 0);

        // response with status OK
        return tud_control_status(rhport, request);
      }
    break;

    default: break;
  }

  // stall unknown request
  return false;
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
      if (!tud_cdc_connected()) {
        return;
      }

      int sent = 0;
      bool blocked = false;
      while (sent < len)
      {
        while (tud_cdc_connected() && !tud_cdc_write_available())
        {
          tud_task();
        }

        if (!tud_cdc_connected()) {
          return;
        }

        int written = tud_cdc_write(buf + sent, len - sent);
        sent += written;
      }
    },
    .out_flush = []() {
      if (tud_cdc_connected()) {
        tud_cdc_write_flush();
      }
    },
    .in_chars = [](char *buf, int len) { 
      if (!tud_cdc_connected()) {
        return 0;
      }
      return (int)tud_cdc_read(buf, (uint32_t)len); 
    },
    .crlf_enabled = PICO_STDIO_DEFAULT_CRLF,
  };

  flash_init();

  pico_unique_board_id_t board_id;
  pico_get_unique_board_id(&board_id);
  bytes_to_hex(usb_serial_number, board_id.id, sizeof(board_id.id));

  board_init();

  // init device stack on configured roothub port
  tud_init(BOARD_TUD_RHPORT);

  if (board_init_after_tusb) {
    board_init_after_tusb();
  }

  stdio_set_driver_enabled(&usb_stdio, true);
}

} // namespace tocata

#endif // HAL_PICO