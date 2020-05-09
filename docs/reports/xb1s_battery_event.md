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
O: Online
R: Reserved / Unused
E: Error (?) / Unknown
C: Charging, I mean really charging the battery (P 'n C)
             not (only) the power cord powering the controller
M M: Mode
  00: Powered by USB
  01: Powered by (disposable) batteries
  10: Powered by Play 'n Charge battery pack (only, no cable)
L L: Capacity Level
  00: (Super) Critical
  01: Low
  10: Medium
  11: Full
```

Notes:

  * I think `online` means whether the dev is online or shutting down
  * The _battery_ is only `present` if not powered by USB
  * Capacity level is only valid as long as the battery is `present`
