#include "uniqueding_kb.h"

#if  0
#ifdef PS2_MOUSE_ENABLE
#include "ps2_mouse.h"
#include "ps2.h"
#endif


#ifdef PS2_MOUSE_ENABLE
  void ps2_mouse_init_user() {
      uint8_t rcv;

      // set TrackPoint sensitivity
      PS2_MOUSE_SEND(0xE2, "tpsens: 0xE2");
      PS2_MOUSE_SEND(0x81, "tpsens: 0x81");
      PS2_MOUSE_SEND(0x4A, "tpsens: 0x4A");
      PS2_MOUSE_SEND(0x49, "tpsens: 0x59");

      // set TrackPoint Negative Inertia factor
      PS2_MOUSE_SEND(0xE2, "tpnegin: 0xE2");
      PS2_MOUSE_SEND(0x81, "tpnegin: 0x81");
      PS2_MOUSE_SEND(0x4D, "tpnegin: 0x4D");
      PS2_MOUSE_SEND(0x06, "tpnegin: 0x06");

      // set TrackPoint speed
      // (transfer function upper plateau speed)
      PS2_MOUSE_SEND(0xE2, "tpsp: 0xE2");
      PS2_MOUSE_SEND(0x81, "tpsp: 0x81");
      PS2_MOUSE_SEND(0x60, "tpsp: 0x60");
      PS2_MOUSE_SEND(0x61, "tpsp: 0x61");

      // inquire pts status
      rcv = ps2_host_send(0xE2);
      rcv = ps2_host_send(0x2C);
      rcv = ps2_host_recv_response();
      if ((rcv & 1) == 1) {
        // if on, disable pts
        rcv = ps2_host_send(0xE2);
        rcv = ps2_host_send(0x47);
        rcv = ps2_host_send(0x2C);
        rcv = ps2_host_send(0x01);
      }
  }
#endif
#endif
