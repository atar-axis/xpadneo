<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# HID Descriptor for XBOX Nano Controller (Bluetooth LE)

Measured on an XBOX Nano Controller connected via Bluetooth LE:

- VID/PID `045E:0B4F`, HID version `0x0609`, BLE firmware 6.09
- OUI field `CC:C1:09` (globally-assigned, unicast)
- 285 bytes, crc16 `0xa56e`

Input report `0x01` carries the sticks and both triggers, report `0x05` carries
the keyboard application. There is a single vendor collection (usage page
`0xFF45`, usage `0x63`, report ID `0x0C`) which xpadneo leaves unhandled.

One detail differs from the older Xbox Wireless Controller (BLE, `045E:0B13`)
descriptor of the same family: the Simulation Controls page is declared **once**
and then shared by both trigger usages, instead of being repeated for each. The
existing fixups look for one page declaration per usage, so they do not match
this shape. The generic `Rx`/`Ry` fixups still apply at offsets 34/36 (the
second stick's `Z`/`Rz`), but `Brake`/`Accelerator` stay on the Simulation page
and are reported as `ABS_BRAKE`/`ABS_GAS`, leaving the `ABS_Z`/`ABS_RZ` axes
that xpadneo always advertises unfilled.

For that fixup the shared page declaration sits at offset 41 (`05 02`), `Brake`
at 44 (`09 C5`) and `Accelerator` at 59 (`09 C4`). No other usage lies between
them, so remapping the page declaration to Generic Desktop and renaming the two
usages affects only the triggers.

Verified with `evtest` on real hardware after the fixup: `ABS_X`/`ABS_Y` and
`ABS_RX`/`ABS_RY` span the full ±32768 range, `ABS_Z`/`ABS_RZ` span 0..1023 for
both triggers, and neither `ABS_GAS` nor `ABS_BRAKE` is advertised any more.

Unpatched descriptor as dumped by xpadneo with `debug_descriptor=1`:
```
00000000: 05 01 09 05 a1 01 85 01 09 01 a1 00 09 30 09 31 15 00 27 ff ff 00 00 95 02 75 10 81 02 c0 09 01
00000020: a1 00 09 32 09 35 81 02 c0 05 02 09 c5 26 ff 03 95 01 75 0a 81 02 75 06 81 03 09 c4 75 0a 81 02
00000040: 75 06 81 03 05 01 09 39 46 3b 01 65 14 15 01 25 08 75 04 81 02 81 03 05 09 19 01 29 0f 45 00 65
00000060: 00 15 00 25 01 95 0f 75 01 81 02 05 0c 09 b2 95 01 81 02 85 03 05 0f 09 21 a1 02 09 97 75 04 91
00000080: 02 91 03 09 70 25 64 95 04 75 08 91 02 09 50 55 0e 26 ff 00 95 01 91 02 09 a7 09 7c 55 00 95 02
000000a0: 91 02 c0 c0 05 01 09 06 a1 01 85 05 05 07 19 e0 29 e7 25 01 95 08 75 01 81 02 95 01 75 08 81 03
000000c0: 19 01 29 a4 15 01 26 a4 00 95 06 81 00 c0 05 01 09 02 a1 01 85 0e 09 01 a1 00 05 09 19 01 29 03
000000e0: 15 00 25 01 95 03 75 01 81 02 95 01 75 05 81 03 05 01 09 30 09 31 15 80 25 7f 95 02 75 08 81 06
00000100: c0 c0 06 45 ff 09 63 a1 01 85 0c 09 63 15 00 26 ff 00 95 3f 82 02 01 09 63 92 02 01 c0
```

The model has no vibration motors, so rumble output cannot be exercised on it,
even though the driver enables rumble streaming for BLE devices.
