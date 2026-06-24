#include "hal.h"

#ifdef HAL_PICO

#include "usb_device.h"

extern "C" {

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

} // extern "C"

namespace tocata {

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

uint HALDisplay::dma;
uint8_t HALDisplay::cs_pin;
uint8_t HALDisplay::reset_pin;
uint8_t HALDisplay::dc_pin;
spi_inst_t* HALDisplay::spi;

} // namespace tocata

#endif // HAL_PICO