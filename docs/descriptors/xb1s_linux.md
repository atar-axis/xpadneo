# USB Descriptor for Xbox One S Wireless (Linux mode)

Hex dump of the controller descriptor:
```
# xxd -c20 -g1 /sys/module/hid_xpadneo/drivers/hid:xpadneo/0005:045E:02FD.005C/report_descriptor

05 01 09 05 a1 01 85 01 09 01 a1 00 09 30 09 31 15 00 27 ff
ff 00 00 95 02 75 10 81 02 c0 09 01 a1 00 09 32 09 35 15 00
27 ff ff 00 00 95 02 75 10 81 02 c0 05 02 09 c5 15 00 26 ff
03 95 01 75 0a 81 02 15 00 25 00 75 06 95 01 81 03 05 02 09
c4 15 00 26 ff 03 95 01 75 0a 81 02 15 00 25 00 75 06 95 01
81 03 05 01 09 39 15 01 25 08 35 00 46 3b 01 66 14 00 75 04
95 01 81 42 75 04 95 01 15 00 25 00 35 00 45 00 65 00 81 03
05 09 19 01 29 0f 15 00 25 01 75 01 95 0f 81 02 15 00 25 00
75 01 95 01 81 03 05 0c 0a 24 02 15 00 25 01 95 01 75 01 81
02 15 00 25 00 75 07 95 01 81 03 05 0c 09 01 85 02 a1 01 05
0c 0a 23 02 15 00 25 01 95 01 75 01 81 02 15 00 25 00 75 07
95 01 81 03 c0 05 0f 09 21 85 03 a1 02 09 97 15 00 25 01 75
04 95 01 91 02 15 00 25 00 75 04 95 01 91 03 09 70 15 00 25
64 75 08 95 04 91 02 09 50 66 01 10 55 0e 15 00 26 ff 00 75
08 95 01 91 02 09 a7 15 00 26 ff 00 75 08 95 01 91 02 65 00
55 00 09 7c 15 00 26 ff 00 75 08 95 01 91 02 c0 05 06 09 20
85 04 15 00 26 ff 00 75 08 95 01 81 02 c0 00
```

Parsed descriptor:
(via https://eleccelerator.com/usbdescreqparser/)
```
0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
0x09, 0x05,        // Usage (Game Pad)
0xA1, 0x01,        // Collection (Application)
0x85, 0x01,        //   Report ID (1)
0x09, 0x01,        //   Usage (Pointer)
0xA1, 0x00,        //   Collection (Physical)
0x09, 0x30,        //     Usage (X)
0x09, 0x31,        //     Usage (Y)
0x15, 0x00,        //     Logical Minimum (0)
0x27, 0xFF, 0xFF, 0x00, 0x00,  //     Logical Maximum (65534)
0x95, 0x02,        //     Report Count (2)
0x75, 0x10,        //     Report Size (16)
0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0xC0,              //   End Collection
0x09, 0x01,        //   Usage (Pointer)
0xA1, 0x00,        //   Collection (Physical)
0x09, 0x32,        //     Usage (Z)
0x09, 0x35,        //     Usage (Rz)
0x15, 0x00,        //     Logical Minimum (0)
0x27, 0xFF, 0xFF, 0x00, 0x00,  //     Logical Maximum (65534)
0x95, 0x02,        //     Report Count (2)
0x75, 0x10,        //     Report Size (16)
0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0xC0,              //   End Collection
0x05, 0x02,        //   Usage Page (Sim Ctrls)
0x09, 0xC5,        //   Usage (Brake)
0x15, 0x00,        //   Logical Minimum (0)
0x26, 0xFF, 0x03,  //   Logical Maximum (1023)
0x95, 0x01,        //   Report Count (1)
0x75, 0x0A,        //   Report Size (10)
0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0x15, 0x00,        //   Logical Minimum (0)
0x25, 0x00,        //   Logical Maximum (0)
0x75, 0x06,        //   Report Size (6)
0x95, 0x01,        //   Report Count (1)
0x81, 0x03,        //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0x05, 0x02,        //   Usage Page (Sim Ctrls)
0x09, 0xC4,        //   Usage (Accelerator)
0x15, 0x00,        //   Logical Minimum (0)
0x26, 0xFF, 0x03,  //   Logical Maximum (1023)
0x95, 0x01,        //   Report Count (1)
0x75, 0x0A,        //   Report Size (10)
0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0x15, 0x00,        //   Logical Minimum (0)
0x25, 0x00,        //   Logical Maximum (0)
0x75, 0x06,        //   Report Size (6)
0x95, 0x01,        //   Report Count (1)
0x81, 0x03,        //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
0x09, 0x39,        //   Usage (Hat switch)
0x15, 0x01,        //   Logical Minimum (1)
0x25, 0x08,        //   Logical Maximum (8)
0x35, 0x00,        //   Physical Minimum (0)
0x46, 0x3B, 0x01,  //   Physical Maximum (315)
0x66, 0x14, 0x00,  //   Unit (System: English Rotation, Length: Centimeter)
0x75, 0x04,        //   Report Size (4)
0x95, 0x01,        //   Report Count (1)
0x81, 0x42,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,Null State)
0x75, 0x04,        //   Report Size (4)
0x95, 0x01,        //   Report Count (1)
0x15, 0x00,        //   Logical Minimum (0)
0x25, 0x00,        //   Logical Maximum (0)
0x35, 0x00,        //   Physical Minimum (0)
0x45, 0x00,        //   Physical Maximum (0)
0x65, 0x00,        //   Unit (None)
0x81, 0x03,        //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0x05, 0x09,        //   Usage Page (Button)
0x19, 0x01,        //   Usage Minimum (0x01)
0x29, 0x0F,        //   Usage Maximum (0x0F)
0x15, 0x00,        //   Logical Minimum (0)
0x25, 0x01,        //   Logical Maximum (1)
0x75, 0x01,        //   Report Size (1)
0x95, 0x0F,        //   Report Count (15)
0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0x15, 0x00,        //   Logical Minimum (0)
0x25, 0x00,        //   Logical Maximum (0)
0x75, 0x01,        //   Report Size (1)
0x95, 0x01,        //   Report Count (1)
0x81, 0x03,        //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0x05, 0x0C,        //   Usage Page (Consumer)
0x0A, 0x24, 0x02,  //   Usage (AC Back)
0x15, 0x00,        //   Logical Minimum (0)
0x25, 0x01,        //   Logical Maximum (1)
0x95, 0x01,        //   Report Count (1)
0x75, 0x01,        //   Report Size (1)
0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0x15, 0x00,        //   Logical Minimum (0)
0x25, 0x00,        //   Logical Maximum (0)
0x75, 0x07,        //   Report Size (7)
0x95, 0x01,        //   Report Count (1)
0x81, 0x03,        //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0x05, 0x0C,        //   Usage Page (Consumer)
0x09, 0x01,        //   Usage (Consumer Control)
0x85, 0x02,        //   Report ID (2)
0xA1, 0x01,        //   Collection (Application)
0x05, 0x0C,        //     Usage Page (Consumer)
0x0A, 0x23, 0x02,  //     Usage (AC Home)
0x15, 0x00,        //     Logical Minimum (0)
0x25, 0x01,        //     Logical Maximum (1)
0x95, 0x01,        //     Report Count (1)
0x75, 0x01,        //     Report Size (1)
0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0x15, 0x00,        //     Logical Minimum (0)
0x25, 0x00,        //     Logical Maximum (0)
0x75, 0x07,        //     Report Size (7)
0x95, 0x01,        //     Report Count (1)
0x81, 0x03,        //     Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0xC0,              //   End Collection
0x05, 0x0F,        //   Usage Page (PID Page)
0x09, 0x21,        //   Usage (0x21)
0x85, 0x03,        //   Report ID (3)
0xA1, 0x02,        //   Collection (Logical)
0x09, 0x97,        //     Usage (0x97)
0x15, 0x00,        //     Logical Minimum (0)
0x25, 0x01,        //     Logical Maximum (1)
0x75, 0x04,        //     Report Size (4)
0x95, 0x01,        //     Report Count (1)
0x91, 0x02,        //     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
0x15, 0x00,        //     Logical Minimum (0)
0x25, 0x00,        //     Logical Maximum (0)
0x75, 0x04,        //     Report Size (4)
0x95, 0x01,        //     Report Count (1)
0x91, 0x03,        //     Output (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
0x09, 0x70,        //     Usage (0x70)
0x15, 0x00,        //     Logical Minimum (0)
0x25, 0x64,        //     Logical Maximum (100)
0x75, 0x08,        //     Report Size (8)
0x95, 0x04,        //     Report Count (4)
0x91, 0x02,        //     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
0x09, 0x50,        //     Usage (0x50)
0x66, 0x01, 0x10,  //     Unit (System: SI Linear, Time: Seconds)
0x55, 0x0E,        //     Unit Exponent (-2)
0x15, 0x00,        //     Logical Minimum (0)
0x26, 0xFF, 0x00,  //     Logical Maximum (255)
0x75, 0x08,        //     Report Size (8)
0x95, 0x01,        //     Report Count (1)
0x91, 0x02,        //     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
0x09, 0xA7,        //     Usage (0xA7)
0x15, 0x00,        //     Logical Minimum (0)
0x26, 0xFF, 0x00,  //     Logical Maximum (255)
0x75, 0x08,        //     Report Size (8)
0x95, 0x01,        //     Report Count (1)
0x91, 0x02,        //     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
0x65, 0x00,        //     Unit (None)
0x55, 0x00,        //     Unit Exponent (0)
0x09, 0x7C,        //     Usage (0x7C)
0x15, 0x00,        //     Logical Minimum (0)
0x26, 0xFF, 0x00,  //     Logical Maximum (255)
0x75, 0x08,        //     Report Size (8)
0x95, 0x01,        //     Report Count (1)
0x91, 0x02,        //     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
0xC0,              //   End Collection
0x05, 0x06,        //   Usage Page (Generic Dev Ctrls)
0x09, 0x20,        //   Usage (Battery Strength)
0x85, 0x04,        //   Report ID (4)
0x15, 0x00,        //   Logical Minimum (0)
0x26, 0xFF, 0x00,  //   Logical Maximum (255)
0x75, 0x08,        //   Report Size (8)
0x95, 0x01,        //   Report Count (1)
0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0xC0,              // End Collection
0x00,              // Unknown (bTag: 0x00, bType: 0x00)

// 335 bytes

// best guess: USB HID Report Descriptor
```

Converted to tables of report IDs:
```
Bit | 0       | 8 | 16 | 24 | 32 | 40    | 48    | 56    | 64 | 72 | 80 | 88 | 96    | 104 | 112     | 120     | 128
Byt | 0       | 1 | 2  | 3  | 4  | 5     | 6     | 7     | 8  | 9  | 10 | 11 | 12    | 13  | 14      | 15      | EOF
ID  |         |   |    |    |    |       |       |       |    |    |    |    |       |     |         |         |
----|----------------------------------------------------------------------------------------------------------|----------
01  | X           : Y       | Z          : Rz            | Brake   | Accel   | Dpad  | Buttons       | AC Back |
Rng | 0-65534     :         | 0-65534    :               | 0-1023  | 0-1023  | 0-1   | 0-1           | 0-1     |
Sz  | 16          : 16      | 16         : 16            | 10+(6)  | 10+(6)  | 4+(4) | 15+(1)        | 1+(7)   | 128 Bits
----|---------------------------------------------------------------------------------------------------------------------
02  | AC Home |   |    |    |    |       |       |       |    |    |    |    |       |     |         |         |
Rng | 0-1     |   |    |    |    |       |       |       |    |    |    |    |       |     |         |         |
Sz  | 1+(7)   |   |    |    |    |       |       |       |    |    |    |    |       |     |         |         | 8 Bits
----|---------------------------------------------------------------------------------------------------------------------
03  | 0x97    | 0x70 (Motors)    | 0x50  | 0xA7  | 0x7C  |    |    |    |    |       |     |         |         |
Rng | 0-1     | 0-100            | 0-255 | 0-255 | 0-255 |    |    |    |    |       |     |         |         |
Sz  | 4+(4)   | 8 : 8  : 8  : 8  | 8     | 8     | 8     |    |    |    |    |       |     |         |         | 64 Bits
----|---------------------------------------------------------------------------------------------------------------------
04  | Bat     |   |    |    |    |       |       |       |    |    |    |    |       |     |         |         |
Rng | 0-255   |   |    |    |    |       |       |       |    |    |    |    |       |     |         |         |
Sz  | 8       |   |    |    |    |       |       |       |    |    |    |    |       |     |         |         | 8 Bits
```

Button bitmap:
```
BTN_A       0000 0001 .000 0000   01 00   Bit 1
BTN_B       0000 0010 .000 0000   02 00   Bit 2

BTN_X       0000 1000 .000 0000   08 00   Bit 4
BTN_Y       0001 0000 .000 0000   10 00   Bit 5

BTN_TL      0100 0000 .000 0000   40 00   Bit 7
BTN_TR      1000 0000 .000 0000   80 00   Bit 8

BTN_SELECT                                         AC Back (left)
BTN_START   0000 0000 .000 1000   00 08   Bit 12   (right)
BTN_HOME    0000 0000 .001 0000   00 10   Bit 13   AC Home (Xbox button, center)

BTN_THUMBL  0000 0000 .010 0000   00 20   Bit 14
BTN_THUMBR  0000 0000 .100 0000   00 40   Bit 15
```
