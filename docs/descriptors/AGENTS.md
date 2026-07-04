<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# DOX Framework

## Purpose

This subtree owns HID descriptor dumps and descriptor-derived reference notes.


## Ownership

- Raw or lightly annotated descriptor dumps.
- Descriptor fingerprints, lengths, CRC values, and source context.
- Device-specific notes that explain why a descriptor matters.


## Local Contracts

- Preserve raw descriptor data exactly unless the source is known to be wrong.
- Record enough context to identify the controller, transport, firmware, and
  report source when that information is available.
- Treat descriptor facts as hardware evidence, not as proof of broad vendor
  behavior unless other sources support that conclusion.


## Work Guidance

- Prefer adding concise metadata around a dump over rewriting the dump itself.
- Link descriptor facts back to driver quirks or heuristics when they explain a
  code change.


## Verification

- Re-check any stated byte length, CRC, VID, PID, name, or firmware value
  against the source material.


## Child DOX Index

No child guides are currently needed.
