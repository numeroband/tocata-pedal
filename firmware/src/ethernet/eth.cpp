#include "eth.h"
#include <cstdint>
#include <cstdio>
#include "hal.h"

extern "C" {
#include "wizchip_spi.h"
#include "dhcp.h"
#include "timer.h"
}

// APPLEMIDI

// #define SerialMon Serial
#define ONE_PARTICIPANT
#define USE_EXT_CALLBACKS
#include <Ethernet.h>
#include <AppleMIDI.h>



unsigned long t1 = millis();
int8_t isConnected = 0;

APPLEMIDI_CREATE_INSTANCE(EthernetUDP, MIDI, "AppleMIDI-Arduino", DEFAULT_CONTROL_PORT);

void OnAppleMidiException(const APPLEMIDI_NAMESPACE::ssrc_t&, const APPLEMIDI_NAMESPACE::Exception&, const int32_t);

//////////////

/* ----------------------------------------------------------------------------------------------------
 * Macros
 * ----------------------------------------------------------------------------------------------------
 */
/* Clock */
#define PLL_SYS_KHZ (133 * 1000)

/* Buffer */
#define ETHERNET_BUF_MAX_SIZE (1024 * 2)

/* Socket */
#define SOCKET_DHCP 0

/* Retry count */
#define DHCP_RETRY_COUNT 5

/**
 * ----------------------------------------------------------------------------------------------------
 * Variables
 * ----------------------------------------------------------------------------------------------------
 */
/* Network */
static wiz_NetInfo g_net_info =
    {
        .mac = {0x00, 0x08, 0xDC, 0x12, 0x34, 0x56}, // MAC address
        .ip = {192, 168, 1, 151},                     // IP address
        .sn = {255, 255, 255, 0},                    // Subnet Mask
        .gw = {192, 168, 1, 1},                     // Gateway
        .dns = {8, 8, 8, 8},                         // DNS server
        .dhcp = NETINFO_DHCP        
};
static uint8_t g_ethernet_buf[ETHERNET_BUF_MAX_SIZE] = {
    0,
}; // common buffer

/* DHCP */
static uint8_t g_dhcp_get_ip_flag = 0;

/* Timer */
static volatile uint16_t g_msec_cnt = 0;

/**
 * ----------------------------------------------------------------------------------------------------
 * Functions
 * ----------------------------------------------------------------------------------------------------
 */
/* Clock */
static void set_clock_khz(void);

/* DHCP */
static void wizchip_dhcp_init(void);
static void wizchip_dhcp_assign(void);
static void wizchip_dhcp_conflict(void);

/* Timer */
static void repeating_timer_callback(void);

///

static void AM_setup();
static void AM_loop();

namespace tocata {

void Ethernet::init() {
    uint32_t now = millis();
    if (_init_millis == 0) {
        _init_millis = now;
    }

    if ((now - _init_millis) < 2000) {
        return;
    }

    _init = true;
    printf("Initializing ethernet...\n");

    set_clock_khz();
    printf("Clock initialized\n");
    wizchip_spi_initialize();
    wizchip_cris_initialize();    
    wizchip_reset();
    wizchip_initialize();
    wizchip_check();

    // printf("sleeping 2s...\n");
    // wizchip_delay_ms(2000);
    // printf("wakeup\n");
   
    wizchip_1ms_timer_initialize(repeating_timer_callback);

    if (g_net_info.dhcp == NETINFO_DHCP) // DHCP
    {
        wizchip_dhcp_init();
        _dhcp_init_ms = millis();
    }
    else // static
    {
        network_initialize(g_net_info);

        /* Get network information */
        print_network_information(g_net_info);
    }
    printf("eth init finished\n");
}

void Ethernet::run() {
    if (!_init) {
        init();
        return;
    }

    if (g_dhcp_get_ip_flag) {
        AM_loop();
    } else {
        if (_dhcp_init_ms) {
            if (millis() - _dhcp_init_ms < 500) {
                return;
            } else {
                _dhcp_init_ms = 0;
            }
        }
        runDHCP();
        if (g_dhcp_get_ip_flag) {
            AM_setup();
        }
    }
}

void Ethernet::runDHCP() {

    /* Initialize */
    static uint8_t retval = 0;
    static uint8_t dhcp_retry = 0;

    /* Assigned IP through DHCP */
    if (g_dhcp_get_ip_flag || g_net_info.dhcp != NETINFO_DHCP)
    {
        return;
    }

    retval = DHCP_run();

    if (retval == DHCP_IP_LEASED)
    {
        if (g_dhcp_get_ip_flag == 0)
        {
            printf(" DHCP success\n");

            g_dhcp_get_ip_flag = 1;
            DHCP_stop();
        }
    }
    else if (retval == DHCP_FAILED)
    {
        g_dhcp_get_ip_flag = 0;
        dhcp_retry++;

        if (dhcp_retry <= DHCP_RETRY_COUNT)
        {
            printf(" DHCP timeout occurred and retry %d\n", dhcp_retry);
        }
    }

    if (dhcp_retry > DHCP_RETRY_COUNT)
    {
        printf(" DHCP failed\n");

        DHCP_stop();
    }
}

void Ethernet::sendProgram(uint8_t channel, uint8_t program) {
    printf("sendprogram %u\n", program);
    MIDI.sendProgramChange(program, channel + 1);
}

void Ethernet::sendControl(uint8_t channel, uint8_t control, uint8_t value) {
    printf("sendcontrol %u\n", control);
    MIDI.sendControlChange(control, value, channel + 1);
}

} // namespace tocata

/**
 * ----------------------------------------------------------------------------------------------------
 * Functions
 * ----------------------------------------------------------------------------------------------------
 */
/* Clock */
static void set_clock_khz(void)
{
    // set a system clock frequency in khz
    set_sys_clock_khz(PLL_SYS_KHZ, true);

    // configure the specified clock
    clock_configure(
        clk_peri,
        0,                                                // No glitchless mux
        CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, // System PLL on AUX mux
        PLL_SYS_KHZ * 1000,                               // Input frequency
        PLL_SYS_KHZ * 1000                                // Output (must be same as no divider)
    );
}

/* DHCP */
static void wizchip_dhcp_init(void)
{
    printf(" DHCP client running\n");

    DHCP_init(SOCKET_DHCP, g_ethernet_buf);

    reg_dhcp_cbfunc(wizchip_dhcp_assign, wizchip_dhcp_assign, wizchip_dhcp_conflict);
}

static void wizchip_dhcp_assign(void)
{
    getIPfromDHCP(g_net_info.ip);
    getGWfromDHCP(g_net_info.gw);
    getSNfromDHCP(g_net_info.sn);
    getDNSfromDHCP(g_net_info.dns);

    g_net_info.dhcp = NETINFO_DHCP;

    /* Network initialize */
    network_initialize(g_net_info); // apply from DHCP

    print_network_information(g_net_info);
    printf(" DHCP leased time : %ld seconds\n", getDHCPLeasetime());
}

static void wizchip_dhcp_conflict(void)
{
    printf(" Conflict IP from DHCP\n");
}

/* Timer */
static void repeating_timer_callback(void)
{
    g_msec_cnt += 1;

    if (g_msec_cnt >= 1000 - 1)
    {
        g_msec_cnt = 0;

        DHCP_time_handler();
    }
}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
static void AM_setup()
{
  AM_DBG_SETUP(115200);
  AM_DBG(F("Das Booting"));

//   ETH_startup();

  AM_DBG(F("OK, now make sure you an rtpMIDI session that is Enabled"));
  AM_DBG(F("Add device named Arduino with Host"), "Port", AppleMIDI.getPort(), "(Name", AppleMIDI.getName(), ")");
  AM_DBG(F("Select and then press the Connect button"));
  AM_DBG(F("Then open a MIDI listener and monitor incoming notes"));

  MIDI.begin(MIDI_CHANNEL_OMNI);

  // Normal callbacks - always available
  // Stay informed on connection status
  AppleMIDI.setHandleConnected([](const APPLEMIDI_NAMESPACE::ssrc_t & ssrc, const char* name) {
    isConnected++;
    AM_DBG(F("Connected to session"), ssrc, name);
  });
  AppleMIDI.setHandleDisconnected([](const APPLEMIDI_NAMESPACE::ssrc_t & ssrc) {
    isConnected--;
    AM_DBG(F("Disconnected"), ssrc);
  });

  // Extended callback, only available when defining USE_EXT_CALLBACKS
  AppleMIDI.setHandleSentRtp([](const APPLEMIDI_NAMESPACE::Rtp_t & rtp) {
    //  AM_DBG(F("an rtpMessage has been sent with sequenceNr"), rtp.sequenceNr);
  });
  AppleMIDI.setHandleSentRtpMidi([](const APPLEMIDI_NAMESPACE::RtpMIDI_t& rtpMidi) {
    AM_DBG(F("an rtpMidiMessage has been sent"), rtpMidi.flags);
  });
  AppleMIDI.setHandleReceivedRtp([](const APPLEMIDI_NAMESPACE::ssrc_t & ssrc, const APPLEMIDI_NAMESPACE::Rtp_t & rtp, const int32_t& latency) {
    //  AM_DBG(F("setHandleReceivedRtp"), ssrc, rtp.sequenceNr , "with", latency, "ms latency");
  });
  AppleMIDI.setHandleStartReceivedMidi([](const APPLEMIDI_NAMESPACE::ssrc_t& ssrc) {
    //  AM_DBG(F("setHandleStartReceivedMidi from SSRC"), ssrc);
  });
  AppleMIDI.setHandleReceivedMidi([](const APPLEMIDI_NAMESPACE::ssrc_t& ssrc, byte value) {
    //    AM_DBG(F("setHandleReceivedMidi from SSRC"), ssrc, ", value:", value);
  });
  AppleMIDI.setHandleEndReceivedMidi([](const APPLEMIDI_NAMESPACE::ssrc_t& ssrc) {
    //  AM_DBG(F("setHandleEndReceivedMidi from SSRC"), ssrc);
  });
  AppleMIDI.setHandleException(OnAppleMidiException);

  MIDI.setHandleControlChange([](Channel channel, byte v1, byte v2) {
    AM_DBG(F("ControlChange"), channel, v1, v2);
    printf("ControlChange %u %u %u\n", channel, v1, v2);
  });
  MIDI.setHandleProgramChange([](Channel channel, byte v1) {
    AM_DBG(F("ProgramChange"), channel, v1);
    printf("ProgramChange %u %u\n", channel, v1);
  });
  MIDI.setHandlePitchBend([](Channel channel, int v1) {
    AM_DBG(F("PitchBend"), channel, v1);
  });
  MIDI.setHandleNoteOn([](byte channel, byte note, byte velocity) {
    AM_DBG(F("NoteOn"), channel, note, velocity);
  });
  MIDI.setHandleNoteOff([](byte channel, byte note, byte velocity) {
    AM_DBG(F("NoteOff"), channel, note, velocity);
  });

  AM_DBG(F("Sending MIDI messages every second"));
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
static void AM_loop()
{
  // Listen to incoming notes
  MIDI.read();

// #ifndef ETHERNET3
//   EthernetBonjour.run();
// #endif
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void OnAppleMidiException(const APPLEMIDI_NAMESPACE::ssrc_t& ssrc, const APPLEMIDI_NAMESPACE::Exception& e, const int32_t value ) {
  switch (e)
  {
    case APPLEMIDI_NAMESPACE::Exception::BufferFullException:
      AM_DBG(F("*** BufferFullException"));
      break;
    case APPLEMIDI_NAMESPACE::Exception::ParseException:
      AM_DBG(F("*** ParseException"));
      break;
    case APPLEMIDI_NAMESPACE::Exception::TooManyParticipantsException:
      AM_DBG(F("*** TooManyParticipantsException"));
      break;
    case APPLEMIDI_NAMESPACE::Exception::UnexpectedInviteException:
      AM_DBG(F("*** UnexpectedInviteException"));
      break;
    case APPLEMIDI_NAMESPACE::Exception::ParticipantNotFoundException:
      AM_DBG(F("*** ParticipantNotFoundException"), value);
      break;
    case APPLEMIDI_NAMESPACE::Exception::ComputerNotInDirectory:
      AM_DBG(F("*** ComputerNotInDirectory"), value);
      break;
    case APPLEMIDI_NAMESPACE::Exception::NotAcceptingAnyone:
      AM_DBG(F("*** NotAcceptingAnyone"), value);
      break;
    case APPLEMIDI_NAMESPACE::Exception::ListenerTimeOutException:
      AM_DBG(F("*** ListenerTimeOutException"));
      break;
    case APPLEMIDI_NAMESPACE::Exception::MaxAttemptsException:
      AM_DBG(F("*** MaxAttemptsException"));
      break;
    case APPLEMIDI_NAMESPACE::Exception::NoResponseFromConnectionRequestException:
      AM_DBG(F("***:yyy did't respond to the connection request. Check the address and port, and any firewall or router settings. (time)"));
      break;
    case APPLEMIDI_NAMESPACE::Exception::SendPacketsDropped:
      AM_DBG(F("*** SendPacketsDropped"), value);
      break;
    case APPLEMIDI_NAMESPACE::Exception::ReceivedPacketsDropped:
      AM_DBG(F("******************************************** ReceivedPacketsDropped"), value);
      break;
    case APPLEMIDI_NAMESPACE::Exception::UdpBeginPacketFailed:
      AM_DBG(F("*** UdpBeginPacketFailed"), value);
      break;
  }
}
