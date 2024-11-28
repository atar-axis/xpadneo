# GameSir Nova OUIs

Gamesir Nova uses random MAC OUIs. We collect them here to implement a
working heuristic matcher.

| OUI          | Bit representation                      | descriptor length
| ------------ | --------------------------------------- | -----------------
| 3E:42:6C     |   `0011 1110 : 0100 0010 : 0110 1100`   | 283
| ED:BC:9A     |   `1110 1101 : 1011 1100 : 1001 1010`   | 283
| 6A:07:14     |   `0110 1010 : 0000 0111 : 0001 0100`   | 283
| **AND mask** | **`0010 1000 : 0000 0000 : 0000 0000`** | **mask 0x28**
