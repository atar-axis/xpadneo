<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# HID Descriptors for GameSir G7 SE


## HID-Mode ID 3537:1082 Length 148

```
# xxd /sys/bus/hid/devices/0003:3537:1082.002F/report_descriptor
00000000: 05 01 09 05 a1 01 85 05 15 00 25 01 35 00 45 01 75 01 95 0f  ..........%.5.E.u...
00000014: 05 09 19 01 29 0f 81 02 95 01 81 01 05 01 25 07 46 3b 01 75  ....).........%.F;.u
00000028: 04 95 01 65 14 09 39 81 42 65 00 95 01 81 01 26 ff 00 46 ff  ...e..9.Be.....&..F.
0000003c: 00 09 30 09 31 09 32 09 35 75 08 95 04 81 02 05 02 15 00 26  ..0.1.2.5u.........&
00000050: ff 00 09 c5 09 c4 95 02 75 08 81 02 05 08 09 43 15 00 26 ff  ........u......C..&.
00000064: 00 35 00 46 ff 00 75 08 95 01 91 82 09 44 91 82 09 45 91 82  .5.F..u......D...E..
00000078: 09 46 91 82 c0 05 0c 09 01 a1 01 85 02 15 00 25 01 75 01 95  .F.............%.u..
0000008c: 08 09 ea 09 e9 81 02 c0                                      ........
1594328436 148 /sys/bus/hid/devices/0003:3537:1082.002F/report_descriptor

0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
0x09, 0x05,        // Usage (Game Pad)
0xA1, 0x01,        // Collection (Application)
0x85, 0x05,        //   Report ID (5)
0x15, 0x00,        //   Logical Minimum (0)
0x25, 0x01,        //   Logical Maximum (1)
0x35, 0x00,        //   Physical Minimum (0)
0x45, 0x01,        //   Physical Maximum (1)
0x75, 0x01,        //   Report Size (1)
0x95, 0x0F,        //   Report Count (15)
0x05, 0x09,        //   Usage Page (Button)
0x19, 0x01,        //   Usage Minimum (0x01)
0x29, 0x0F,        //   Usage Maximum (0x0F)
0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0x95, 0x01,        //   Report Count (1)
0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
0x25, 0x07,        //   Logical Maximum (7)
0x46, 0x3B, 0x01,  //   Physical Maximum (315)
0x75, 0x04,        //   Report Size (4)
0x95, 0x01,        //   Report Count (1)
0x65, 0x14,        //   Unit (System: English Rotation, Length: Centimeter)
0x09, 0x39,        //   Usage (Hat switch)
0x81, 0x42,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,Null State)
0x65, 0x00,        //   Unit (None)
0x95, 0x01,        //   Report Count (1)
0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
0x26, 0xFF, 0x00,  //   Logical Maximum (255)
0x46, 0xFF, 0x00,  //   Physical Maximum (255)
0x09, 0x30,        //   Usage (X)
0x09, 0x31,        //   Usage (Y)
0x09, 0x32,        //   Usage (Z)
0x09, 0x35,        //   Usage (Rz)
0x75, 0x08,        //   Report Size (8)
0x95, 0x04,        //   Report Count (4)
0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0x05, 0x02,        //   Usage Page (Sim Ctrls)
0x15, 0x00,        //   Logical Minimum (0)
0x26, 0xFF, 0x00,  //   Logical Maximum (255)
0x09, 0xC5,        //   Usage (Brake)
0x09, 0xC4,        //   Usage (Accelerator)
0x95, 0x02,        //   Report Count (2)
0x75, 0x08,        //   Report Size (8)
0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0x05, 0x08,        //   Usage Page (LEDs)
0x09, 0x43,        //   Usage (Slow Blink On Time)
0x15, 0x00,        //   Logical Minimum (0)
0x26, 0xFF, 0x00,  //   Logical Maximum (255)
0x35, 0x00,        //   Physical Minimum (0)
0x46, 0xFF, 0x00,  //   Physical Maximum (255)
0x75, 0x08,        //   Report Size (8)
0x95, 0x01,        //   Report Count (1)
0x91, 0x82,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Volatile)
0x09, 0x44,        //   Usage (Slow Blink Off Time)
0x91, 0x82,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Volatile)
0x09, 0x45,        //   Usage (Fast Blink On Time)
0x91, 0x82,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Volatile)
0x09, 0x46,        //   Usage (Fast Blink Off Time)
0x91, 0x82,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Volatile)
0xC0,              // End Collection
0x05, 0x0C,        // Usage Page (Consumer)
0x09, 0x01,        // Usage (Consumer Control)
0xA1, 0x01,        // Collection (Application)
0x85, 0x02,        //   Report ID (2)
0x15, 0x00,        //   Logical Minimum (0)
0x25, 0x01,        //   Logical Maximum (1)
0x75, 0x01,        //   Report Size (1)
0x95, 0x08,        //   Report Count (8)
0x09, 0xEA,        //   Usage (Volume Decrement)
0x09, 0xE9,        //   Usage (Volume Increment)
0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0xC0,              // End Collection

// 148 bytes
```


## HID-Mode ID 3537:1082 Length 214

```
# xxd /sys/bus/hid/devices/0003:3537:1082.0030/report_descriptor
00000000: 05 01 09 06 a1 01 85 03 05 07 19 e0 29 e7 15 00 25 01 75 01  ............)...%.u.
00000014: 95 08 81 02 75 08 95 01 81 01 05 07 19 00 2a ff 00 15 00 26  ....u.........*....&
00000028: ff 00 75 08 95 06 81 00 05 08 19 01 29 03 25 01 75 01 95 03  ..u.........).%.u...
0000003c: 91 02 95 05 91 01 c0 06 0c 00 09 01 a1 01 85 02 19 00 2b ff  ..................+.
00000050: ff 00 00 15 00 27 ff ff 00 00 75 10 95 02 81 00 c0 05 01 09  .....'....u.........
00000064: 02 a1 01 85 09 09 01 a1 00 05 09 19 01 29 05 15 00 25 01 95  .............)...%..
00000078: 05 75 01 81 02 95 01 75 03 81 01 05 01 09 30 09 31 16 01 80  .u.....u......0.1...
0000008c: 26 ff 7f 75 10 95 02 81 06 09 38 15 81 25 7f 75 08 95 01 81  &..u......8..%.u....
000000a0: 06 95 01 81 03 c0 c0 06 f0 ff 09 40 a1 01 06 00 ff 09 00 85  ...........@........
000000b4: 10 15 00 26 ff 00 35 00 46 ff 00 75 08 95 3f 81 02 85 12 09  ...&..5.F..u..?.....
000000c8: 21 95 3f 81 02 85 0f 09 22 95 3f 91 02 c0                    !.?.....".?...
1398098508 214 /sys/bus/hid/devices/0003:3537:1082.0030/report_descriptor

0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
0x09, 0x06,        // Usage (Keyboard)
0xA1, 0x01,        // Collection (Application)
0x85, 0x03,        //   Report ID (3)
0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
0x19, 0xE0,        //   Usage Minimum (0xE0)
0x29, 0xE7,        //   Usage Maximum (0xE7)
0x15, 0x00,        //   Logical Minimum (0)
0x25, 0x01,        //   Logical Maximum (1)
0x75, 0x01,        //   Report Size (1)
0x95, 0x08,        //   Report Count (8)
0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0x75, 0x08,        //   Report Size (8)
0x95, 0x01,        //   Report Count (1)
0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
0x19, 0x00,        //   Usage Minimum (0x00)
0x2A, 0xFF, 0x00,  //   Usage Maximum (0xFF)
0x15, 0x00,        //   Logical Minimum (0)
0x26, 0xFF, 0x00,  //   Logical Maximum (255)
0x75, 0x08,        //   Report Size (8)
0x95, 0x06,        //   Report Count (6)
0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
0x05, 0x08,        //   Usage Page (LEDs)
0x19, 0x01,        //   Usage Minimum (Num Lock)
0x29, 0x03,        //   Usage Maximum (Scroll Lock)
0x25, 0x01,        //   Logical Maximum (1)
0x75, 0x01,        //   Report Size (1)
0x95, 0x03,        //   Report Count (3)
0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
0x95, 0x05,        //   Report Count (5)
0x91, 0x01,        //   Output (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
0xC0,              // End Collection
0x06, 0x0C, 0x00,  // Usage Page (Consumer)
0x09, 0x01,        // Usage (Consumer Control)
0xA1, 0x01,        // Collection (Application)
0x85, 0x02,        //   Report ID (2)
0x19, 0x00,        //   Usage Minimum (Unassigned)
0x2B, 0xFF, 0xFF, 0x00, 0x00,  //   Usage Maximum (0xFFFF)
0x15, 0x00,        //   Logical Minimum (0)
0x27, 0xFF, 0xFF, 0x00, 0x00,  //   Logical Maximum (65534)
0x75, 0x10,        //   Report Size (16)
0x95, 0x02,        //   Report Count (2)
0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
0xC0,              // End Collection
0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
0x09, 0x02,        // Usage (Mouse)
0xA1, 0x01,        // Collection (Application)
0x85, 0x09,        //   Report ID (9)
0x09, 0x01,        //   Usage (Pointer)
0xA1, 0x00,        //   Collection (Physical)
0x05, 0x09,        //     Usage Page (Button)
0x19, 0x01,        //     Usage Minimum (0x01)
0x29, 0x05,        //     Usage Maximum (0x05)
0x15, 0x00,        //     Logical Minimum (0)
0x25, 0x01,        //     Logical Maximum (1)
0x95, 0x05,        //     Report Count (5)
0x75, 0x01,        //     Report Size (1)
0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0x95, 0x01,        //     Report Count (1)
0x75, 0x03,        //     Report Size (3)
0x81, 0x01,        //     Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
0x09, 0x30,        //     Usage (X)
0x09, 0x31,        //     Usage (Y)
0x16, 0x01, 0x80,  //     Logical Minimum (-32767)
0x26, 0xFF, 0x7F,  //     Logical Maximum (32767)
0x75, 0x10,        //     Report Size (16)
0x95, 0x02,        //     Report Count (2)
0x81, 0x06,        //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
0x09, 0x38,        //     Usage (Wheel)
0x15, 0x81,        //     Logical Minimum (-127)
0x25, 0x7F,        //     Logical Maximum (127)
0x75, 0x08,        //     Report Size (8)
0x95, 0x01,        //     Report Count (1)
0x81, 0x06,        //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
0x95, 0x01,        //     Report Count (1)
0x81, 0x03,        //     Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0xC0,              //   End Collection
0xC0,              // End Collection
0x06, 0xF0, 0xFF,  // Usage Page (Vendor Defined 0xFFF0)
0x09, 0x40,        // Usage (0x40)
0xA1, 0x01,        // Collection (Application)
0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined 0xFF00)
0x09, 0x00,        //   Usage (0x00)
0x85, 0x10,        //   Report ID (16)
0x15, 0x00,        //   Logical Minimum (0)
0x26, 0xFF, 0x00,  //   Logical Maximum (255)
0x35, 0x00,        //   Physical Minimum (0)
0x46, 0xFF, 0x00,  //   Physical Maximum (255)
0x75, 0x08,        //   Report Size (8)
0x95, 0x3F,        //   Report Count (63)
0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0x85, 0x12,        //   Report ID (18)
0x09, 0x21,        //   Usage (0x21)
0x95, 0x3F,        //   Report Count (63)
0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0x85, 0x0F,        //   Report ID (15)
0x09, 0x22,        //   Usage (0x22)
0x95, 0x3F,        //   Report Count (63)
0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
0xC0,              // End Collection

// 214 bytes
```


## HID-Mode ID 3537:1010 Length 177

**Enabled via:** Hold Menu + View WHILE connecting the controller

```
# xxd /sys/bus/hid/devices/0003:3537:1010.0031/report_descriptor
00000000: 05 01 09 05 a1 01 85 01 09 30 09 31 09 32 09 35 15 00 26 ff  .........0.1.2.5..&.
00000014: 00 75 08 95 04 81 02 09 39 15 00 25 07 35 00 46 3b 01 65 14  .u......9..%.5.F;.e.
00000028: 75 04 95 01 81 42 65 00 05 09 19 01 29 12 15 00 25 01 75 01  u....Be.....)...%.u.
0000003c: 95 12 81 02 06 00 ff 09 20 75 02 95 01 81 02 05 01 09 33 09  ........ u........3.
00000050: 34 15 00 26 ff 00 75 08 95 02 81 02 06 00 ff 09 21 95 36 81  4..&..u.........!.6.
00000064: 02 85 05 09 22 95 1f 91 02 85 03 0a 21 27 95 2f b1 02 06 80  ....".......!'./....
00000078: ff 85 e0 09 57 95 02 b1 02 c0 06 f0 ff 09 40 a1 01 06 00 ff  ....W.........@.....
0000008c: 09 00 85 10 15 00 26 ff 00 35 00 46 ff 00 75 08 95 3f 81 02  ......&..5.F..u..?..
000000a0: 85 12 09 21 95 3f 81 02 85 0f 09 22 95 3f 91 02 c0           ...!.?.....".?...
1002334289 177 /sys/bus/hid/devices/0003:3537:1010.0031/report_descriptor

0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
0x09, 0x05,        // Usage (Game Pad)
0xA1, 0x01,        // Collection (Application)
0x85, 0x01,        //   Report ID (1)
0x09, 0x30,        //   Usage (X)
0x09, 0x31,        //   Usage (Y)
0x09, 0x32,        //   Usage (Z)
0x09, 0x35,        //   Usage (Rz)
0x15, 0x00,        //   Logical Minimum (0)
0x26, 0xFF, 0x00,  //   Logical Maximum (255)
0x75, 0x08,        //   Report Size (8)
0x95, 0x04,        //   Report Count (4)
0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0x09, 0x39,        //   Usage (Hat switch)
0x15, 0x00,        //   Logical Minimum (0)
0x25, 0x07,        //   Logical Maximum (7)
0x35, 0x00,        //   Physical Minimum (0)
0x46, 0x3B, 0x01,  //   Physical Maximum (315)
0x65, 0x14,        //   Unit (System: English Rotation, Length: Centimeter)
0x75, 0x04,        //   Report Size (4)
0x95, 0x01,        //   Report Count (1)
0x81, 0x42,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,Null State)
0x65, 0x00,        //   Unit (None)
0x05, 0x09,        //   Usage Page (Button)
0x19, 0x01,        //   Usage Minimum (0x01)
0x29, 0x12,        //   Usage Maximum (0x12)
0x15, 0x00,        //   Logical Minimum (0)
0x25, 0x01,        //   Logical Maximum (1)
0x75, 0x01,        //   Report Size (1)
0x95, 0x12,        //   Report Count (18)
0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined 0xFF00)
0x09, 0x20,        //   Usage (0x20)
0x75, 0x02,        //   Report Size (2)
0x95, 0x01,        //   Report Count (1)
0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
0x09, 0x33,        //   Usage (Rx)
0x09, 0x34,        //   Usage (Ry)
0x15, 0x00,        //   Logical Minimum (0)
0x26, 0xFF, 0x00,  //   Logical Maximum (255)
0x75, 0x08,        //   Report Size (8)
0x95, 0x02,        //   Report Count (2)
0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined 0xFF00)
0x09, 0x21,        //   Usage (0x21)
0x95, 0x36,        //   Report Count (54)
0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0x85, 0x05,        //   Report ID (5)
0x09, 0x22,        //   Usage (0x22)
0x95, 0x1F,        //   Report Count (31)
0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
0x85, 0x03,        //   Report ID (3)
0x0A, 0x21, 0x27,  //   Usage (0x2721)
0x95, 0x2F,        //   Report Count (47)
0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
0x06, 0x80, 0xFF,  //   Usage Page (Vendor Defined 0xFF80)
0x85, 0xE0,        //   Report ID (-32)
0x09, 0x57,        //   Usage (0x57)
0x95, 0x02,        //   Report Count (2)
0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
0xC0,              // End Collection
0x06, 0xF0, 0xFF,  // Usage Page (Vendor Defined 0xFFF0)
0x09, 0x40,        // Usage (0x40)
0xA1, 0x01,        // Collection (Application)
0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined 0xFF00)
0x09, 0x00,        //   Usage (0x00)
0x85, 0x10,        //   Report ID (16)
0x15, 0x00,        //   Logical Minimum (0)
0x26, 0xFF, 0x00,  //   Logical Maximum (255)
0x35, 0x00,        //   Physical Minimum (0)
0x46, 0xFF, 0x00,  //   Physical Maximum (255)
0x75, 0x08,        //   Report Size (8)
0x95, 0x3F,        //   Report Count (63)
0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0x85, 0x12,        //   Report ID (18)
0x09, 0x21,        //   Usage (0x21)
0x95, 0x3F,        //   Report Count (63)
0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0x85, 0x0F,        //   Report ID (15)
0x09, 0x22,        //   Usage (0x22)
0x95, 0x3F,        //   Report Count (63)
0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
0xC0,              // End Collection

// 177 bytes
```
