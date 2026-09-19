<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Mobapad Chitu 2

Issue #628 reports two Mobapad Chitu 2 controllers in Xbox Bluetooth mode that identify as `045E:02E0` with name
`Xbox Wireless Controller`.

Measured facts:

- Descriptor: 306 bytes, crc16 `0x8bc5`, known as `Xbox Wireless Controller (legacy)`.
- Observed address prefix: `A0:5A:54`.
- The prefix was reported by two units, but is not known to be an officially registered Mobapad OUI.
- The descriptor exposes the old compact 10-button bitmap.
- Report ID 1 is 17 bytes and sends the sparse Linux/Android-style button bits.
- Manual quirk `+16` fixes the regular gamepad buttons and the digital trigger axes for both reported units.

The driver therefore enables `XPADNEO_QUIRK_LINUX_BUTTONS` for the observed `A0:5A:54` prefix. It also detects
sparse-only button bits in 17-byte Report ID 1 packets with the 306-byte legacy descriptor and enables the same quirk
dynamically for similar devices with other address prefixes.
