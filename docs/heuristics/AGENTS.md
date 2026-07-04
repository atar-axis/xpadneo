<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# DOX Framework

## Purpose

This subtree owns notes about detection heuristics and the evidence behind
hardware classification decisions.


## Ownership

- Heuristic documentation for controller families.
- Notes that compare descriptor, name, OUI, VID, PID, or report behavior.
- Limitations and false-positive risks for detection logic.


## Local Contracts

- State whether a rule is based on measured data, issue reports, source review,
  or inference.
- Keep vendor and product claims narrow when evidence only covers one device or
  firmware family.
- Document when a driver flag deliberately bypasses a heuristic.


## Work Guidance

- Update this area when a code change changes classification logic, skip logic,
  or hardware-specific quirk selection.
- Prefer concise tables or bullet lists for evidence that needs comparison.


## Verification

- Cross-check heuristic claims against the matching driver tables and descriptor
  notes.


## Child DOX Index

No child guides are currently needed.
