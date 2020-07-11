# Report ID 0x04

Received from device
```
| ID | Byte 1        |
----------------------
| 04 | battery flags |
```

Flags
```
 msb          ID 04          lsb
+---+---+---+---+---+---+---+---+
| O | R | E | C | M | M | L | L |
+---+---+---+---+---+---+---+---+
O: Online (always 1, except shutdown)
R: Reserved / Unused
E: Error (?) / Unknown
C: Charging, I mean really charging the battery (P 'n C)
             not (only) the power cord powering the controller
M M: Mode
  00: Powered by USB
  01: Powered by (disposable) batteries
  10: Powered by Play 'n Charge battery pack (only, no cable)

L L: Capacity Level (always 0 with M=00)
  00: (Super) Critical
  01: Low
  10: Medium
  11: Full
```
