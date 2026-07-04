<!-- SPDX-License-Identifier: GPL-2.0-only -->

# DOX Framework

## Purpose

This subtree owns the xpadneo driver implementation.


## Ownership

- Core probe, disconnect, report fixup, and raw event paths.
- Device tables, flags, quirks, and controller identification.
- Input mapping, report handling, power behavior, and rumble support.
- Local headers and compile-time configuration.


## Local Contracts

- Keep device IDs, flags, descriptors, and heuristics traceable to issue
  reports, hardware data, or existing driver behavior.
- Do not broaden a quirk beyond the hardware evidence that supports it.
- Prefer explicit device flags for known hardware capabilities and narrow skip
  paths for known false-positive heuristics.
- Preserve existing controller behavior unless the change is explicitly scoped
  as a behavior change.


## Work Guidance

- Review nearby probe, raw-event, and report-fixup flow before adding a new
  table entry or flag.
- Keep feature additions and infrastructure refactors in separate commits when
  both are needed.
- When adding hardware support, update the relevant descriptor or heuristic
  documentation if the evidence is not already covered.


## Verification

- Run `make -C hid-xpadneo modules` for code changes when possible.
- For logic that cannot be exercised locally, document the missing hardware or
  runtime coverage in the closeout.


## Child DOX Index

No child guides are currently needed.
