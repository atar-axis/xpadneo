<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# DOX Framework

## Purpose

This subtree owns captured report material used as hardware or behavior
evidence.


## Ownership

- Report captures and decoded report notes.
- Reference material from issue reports when it supports current behavior.


## Local Contracts

- Preserve captured data exactly unless the source is known to be wrong.
- Record source context when available, including issue number, controller,
  firmware, transport, and capture method.
- Do not generalize from one report capture without matching driver or
  descriptor evidence.


## Work Guidance

- Link report facts to the driver behavior or heuristic they justify.
- Keep sensitive or irrelevant user-specific data out of stored captures.


## Verification

- Re-check stated IDs, names, firmware values, and report bytes against the
  source material.


## Child DOX Index

No child guides are currently needed.
