# GameSir Nova OUIs

Gamesir Nova uses random MAC OUIs. We collect them here to implement a
working heuristic matcher.

We also collect OUIs here which should not match the heuristics because we
are seeing conflicts with some official vendors.

Officially validated OUIs are marked with a "+".

| OUI          | Vendor    | Bit representation                      | descriptor length
| ------------ | --------- | --------------------------------------- | -------------------
| 3E:42:6C     | GameSir   |   `0011 1110 : 0100 0010 : 0110 1100`   | 283
| ED:BC:9A     | GameSir   |   `1110 1101 : 1011 1100 : 1001 1010`   | 283
| 6A:07:14     | GameSir   |   `0110 1010 : 0000 0111 : 0001 0100`   | 283
| **AND mask** |           | **`0010 1000 : 0000 0000 : 0000 0000`** | **mask 0x28**
| 3C:FA:06  +  | Microsoft |   `0011 1100 : 1111 1010 : 0000 0110`   | 283 inval match
| 44:16:22  +  | Microsoft |   `0100 0100 : 0001 0110 : 0010 0010`   |
| 68:6C:E6  +  | Microsoft |   `0110 1000 : 0110 1100 : 1110 0110`   | 283 inval match
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
| E4:17:D8  +  | 8BitDo    |   `1110 0100 : 0001 0111 : 1101 1000`   |
| **AND mask** |           | **`1110 0100 : 0001 0111 : 1101 1000`** | **mask 0xE4 17 D8**
