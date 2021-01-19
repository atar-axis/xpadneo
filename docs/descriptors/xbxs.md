# USB Descriptor for Xbox Series X|S Wireless

Hex dump of the controller descriptor:
```
# xxd -c20 -g1 /sys/module/hid_xpadneo/drivers/hid:xpadneo/0005:045E:0B13.000A/report_descriptor

05 01 09 05 a1 01 85 01 09 01 a1 00 09 30 09 31 15 00 27 ff
ff 00 00 95 02 75 10 81 02 c0 09 01 a1 00 09 32 09 35 15 00
27 ff ff 00 00 95 02 75 10 81 02 c0 05 02 09 c5 15 00 26 ff
03 95 01 75 0a 81 02 15 00 25 00 75 06 95 01 81 03 05 02 09
c4 15 00 26 ff 03 95 01 75 0a 81 02 15 00 25 00 75 06 95 01
81 03 05 01 09 39 15 01 25 08 35 00 46 3b 01 66 14 00 75 04
95 01 81 42 75 04 95 01 15 00 25 00 35 00 45 00 65 00 81 03
05 09 19 01 29 0f 15 00 25 01 75 01 95 0f 81 02 15 00 25 00
75 01 95 01 81 03 05 0c 0a b2 00 15 00 25 01 95 01 75 01 81
02 15 00 25 00 75 07 95 01 81 03 05 0f 09 21 85 03 a1 02 09
97 15 00 25 01 75 04 95 01 91 02 15 00 25 00 75 04 95 01 91
03 09 70 15 00 25 64 75 08 95 04 91 02 09 50 66 01 10 55 0e
15 00 26 ff 00 75 08 95 01 91 02 09 a7 15 00 26 ff 00 75 08
95 01 91 02 65 00 55 00 09 7c 15 00 26 ff 00 75 08 95 01 91
02 c0 c0
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
0x0A, 0xB2, 0x00,  //   Usage (Record)
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
0xC0,              // End Collection

// 283 bytes

// best guess: USB HID Report Descriptor
```

btmon Logs:
```
Bluetooth monitor ver 5.55
= Note: Linux version 5.10.5-arch1-1 (x86_64)                          0.182753
= Note: Bluetooth subsystem version 2.22                               0.182760
= New Index: 9C:FC:E8:B8:2C:A6 (Primary,USB,hci0)               [hci0] 0.182763
= Open Index: 9C:FC:E8:B8:2C:A6                                 [hci0] 0.182765
= Index Info: 9C:FC:E8:B8:2C:A6 (Intel Corp.)                   [hci0] 0.182767
@ MGMT Open: bluetoothd (privileged) version 1.18             {0x0001} 0.182770
> ACL Data RX: Handle 256 flags 0x02 dlen 14                 #1 [hci0] 0.254476
      Channel: 65 len 10 [PSM 0 mode Basic (0x00)] {chan 65535}
        a1 01 00 00 00 00 00 00 00 00                    ..........


## select
> ACL Data RX: Handle 3586 flags 0x02 dlen 23                #2 [hci0] 3.043182
      ATT: Handle Value Notification (0x1b) len 18
        Handle: 0x001e
          Data: eb7e5f80eb7f85800000000000000800
> ACL Data RX: Handle 3586 flags 0x02 dlen 23                #3 [hci0] 3.160076
      ATT: Handle Value Notification (0x1b) len 18
        Handle: 0x001e
          Data: eb7e5f80eb7f85800000000000000800
## share
> ACL Data RX: Handle 3586 flags 0x02 dlen 23               #27 [hci0] 5.140647
      ATT: Handle Value Notification (0x1b) len 18
        Handle: 0x001e
          Data: eb7e5f80eb7f85800000000000000001
> ACL Data RX: Handle 3586 flags 0x02 dlen 23               #28 [hci0] 5.230076
      ATT: Handle Value Notification (0x1b) len 18
        Handle: 0x001e
          Data: eb7e5f80eb7f85800000000000000001
## back
> ACL Data RX: Handle 3586 flags 0x02 dlen 23               #43 [hci0] 6.535466
      ATT: Handle Value Notification (0x1b) len 18
        Handle: 0x001e
          Data: eb7e5f80eb7f85800000000000000400
> ACL Data RX: Handle 3586 flags 0x02 dlen 23               #44 [hci0] 6.580045
      ATT: Handle Value Notification (0x1b) len 18
        Handle: 0x001e
          Data: eb7e5f80eb7f85800000000000000400
                ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^
                1 2 3 4 5 6 7 8 9 10  12  14  16
                                    11  13  15

[15] = 0x04 Back
[15] = 0x08 Select
[16] = 0x01 Share
```
