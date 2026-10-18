<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# GameSir Nova OUIs

Gamesir Nova uses random MAC OUIs. We collect them here to implement a
working heuristic matcher.

We also collect OUIs here which should not match the heuristics because we
are seeing conflicts with some official vendors.

Some non-GameSir devices can also use a randomized Bluetooth MAC address which
matches the heuristic. The ASUS ROG Raikiri Pro (`0B05:1ABD`) reports the same
283-byte descriptor as the Xbox Series X|S controller but may use an LAA and
multicast OUI, so it is excluded by device ID instead of by OUI.

The best indicators we currently have are the LLA or multicast bits in the MAC
address, one of which is always set. While the LLA bit is quite weak because
it is technically allowed, the multicast bit is much stronger because it should
never be used for controllers, and all other vendors follow that.

- Officially validated OUIs are flagged with a "+".
- LAA OUIs are flagged with an "L".
- Multicast OUIs are flagged with an "M".

| OUI    Flag  | Vendor    | Bit representation                      | descriptor length
| ------------ | --------- | --------------------------------------- | -------------------
| 1F:45:F3  LM | GameSir   |   `0001 1111 : 0100 0101 : 1111 0011`   | 306
| 3E:42:6C  L  | GameSir   |   `0011 1110 : 0100 0010 : 0110 1100`   | 283
| ED:BC:9A  M  | GameSir   |   `1110 1101 : 1011 1100 : 1001 1010`   | 283
| 6A:07:14  L  | GameSir   |   `0110 1010 : 0000 0111 : 0001 0100`   | 283
| E2:82:69  L  | GameSir   |   `1110 0010 : 1000 0010 : 0110 1001`   | 283
| **OR Mask**  |           | **`0000 0011 : 0000 0000 : 0000 0000`   | **mask 0x03**
| 3C:FA:06  +  | Microsoft |   `0011 1100 : 1111 1010 : 0000 0110`   | 283 inval match
| 44:16:22  +  | Microsoft |   `0100 0100 : 0001 0110 : 0010 0010`   |
| 68:6C:E6  +  | Microsoft |   `0110 1000 : 0110 1100 : 1110 0110`   | 283 inval match
| 78:86:2E  +  | Microsoft |   `0111 1000 : 1000 0110 : 0010 1110`   |
| 98:7A:14  +  | Microsoft |   `1001 1000 : 0111 1010 : 0001 0100`   |
| AC:8E:BD  +  | Microsoft |   `1010 1100 : 1000 1110 : 1011 1101`   | 283 inval match
| C8:3F:26  +  | Microsoft |   `1100 1000 : 0011 1111 : 0010 0110`   |
| **AND mask** |           | **`0000 0000 : 0000 0000 : 0000 0000`** | **mask 0x00**
| 98:B6:E9  +  | Nintendo  |   `1001 1000 : 1011 0110 : 1110 1001`   |
| **AND mask** |           | **`0010 1000 : 1011 0110 : 1110 1001`** | **mask 0x98 B6 E9**
| 98:B6:E8     | GuliKit   |   `1001 1000 : 1011 0110 : 1110 1000`   |
| 98:B6:EA     | GuliKit   |   `1001 1000 : 1011 0110 : 1110 1010`   |
| 98:B6:EC     | GuliKit   |   `1001 1000 : 1011 0110 : 1110 1100`   |
| **AND mask** |           | **`0010 1000 : 1011 0110 : 1110 1000`** | **mask 0x98 B6 E8**
| 06:71:10  L  | GuliKit   |   `0000 0110 : 0111 0001 : 0001 0000`
| **AND mask** |           | **`0000 0110 : 0111 0001 : 0001 0000`** | **mask 0x06 61 10**
| E4:17:D8  +  | 8BitDo    |   `1110 0100 : 0001 0111 : 1101 1000`   |
| **AND mask** |           | **`1110 0100 : 0001 0111 : 1101 1000`** | **mask 0xE4 17 D8**


## Firmware versions

Some GameSir devices use MAC addresses with neither the LAA nor the multicast
bit set, so the OUI heuristic misses them. These devices claim a genuine PID
but report a firmware version that genuine controllers with this PID never use,
so the driver also enables the GameSir quirks for an exact match of vendor ID,
PID and firmware version. Exact matches avoid catching genuine controllers on
old firmware. `XPADNEO_QUIRK_NO_HEURISTICS` (`512`) disables this rule, too.

| Model        | PID    | Firmware         | Reported OUIs                  | Source
| ------------ | ------ | ---------------- | ------------------------------ | ---------------
| Nova 2 Lite  | `02E0` | `1.1b` (classic) | 1F:45:F3, 0B:BE:1E             | #594, #608
| T4 Nova Lite | `0B13` | `0.23` (classic) | 6A:07:14, AC:20:06, E2:82:69   | #484, #518, #623

Genuine controllers with PID `0B13` (Xbox Series X|S) report BLE firmware
versions from `5.05` to `5.23` in all known reports, and they do not support
classic Bluetooth (BR/EDR) at all.

Not every unit has a logged firmware version: the T4 Nova Lite with OUI
`64:6F:9B` (#484) needs the GameSir quirks but did not report its version.
The quirks are confirmed for `6A:07:14` (#484), while `E2:82:69` (#623) does
not rumble at all even with them, and `AC:20:06` (#518) never confirmed a fix.

As far as we know, genuine Microsoft controllers never use firmware version
`1.1b`. The Nova 2 Lite reports this version in both its Xbox mode and its
DInput mode (`3537:100E`, #608).

The 306-byte descriptor (crc16 `0x8BC5`) of the Nova 2 Lite is byte-identical
to the genuine Xbox One S Windows-mode descriptor (307 bytes) without its
trailing NUL byte (measured from the dumps in `docs/descriptors/`). The Mobapad
Chitu 2 reports the same 306-byte descriptor, so the descriptor indicates a
clone but not GameSir specifically.
