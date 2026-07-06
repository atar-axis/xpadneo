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
- Do not reconstruct original descriptors by manually reverting xpadneo
  descriptor patches from already-patched dumps. That process is not reliable
  evidence if one patch is missed.
- Prefer original descriptor evidence from current driver logs: known
  descriptors normally log checksum, MAC parameters, size, and IDs; unknown or
  requested descriptors can be dumped through dmesg, including by enabling the
  descriptor debug module parameter when needed.


## Work Guidance

- Prefer adding concise metadata around a dump over rewriting the dump itself.
- Link descriptor facts back to driver quirks or heuristics when they explain a
  code change.
- Ask reporters to reproduce descriptor-sensitive issues on current `master`
  when existing logs predate descriptor checksum and dump support.


## Verification

- Re-check any stated byte length, CRC, VID, PID, name, or firmware value
  against the source material.


## Child DOX Index

No child guides are currently needed.
