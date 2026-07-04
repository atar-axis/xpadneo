<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# DOX Framework

## Purpose

This subtree owns project documentation, hardware evidence, descriptor records,
licensing notes, and captured report material.


## Ownership

- User and contributor documentation.
- HID descriptor dumps and controller-specific evidence.
- Heuristic notes and report captures.
- Documentation site configuration and assets.


## Local Contracts

- Follow the existing SPDX license in each file. New documentation defaults to
  GPL-3.0-or-later unless a more specific local convention applies.
- Hardware documentation must distinguish measured facts from inference.
- Do not duplicate descriptor dumps unless the duplicate carries useful device,
  firmware, connection, or report context.
- Keep references to issues, commits, descriptors, CRCs, OUIs, names, and
  firmware versions precise when they support a driver decision.


## Work Guidance

- Put durable hardware evidence near the descriptor, heuristic, or report area
  that best matches the source material.
- Keep user-facing instructions operational and avoid describing internal
  implementation history unless it changes current behavior.
- When driver support is added for new hardware, document the evidence that
  justifies the device match or quirk.


## Verification

- Inspect documentation diffs for stale paths, repeated wording, and broken
  cross references.
- Run a local site build only when site configuration or generated site behavior
  is touched and the required tooling is already available.


## Child DOX Index

- `descriptors/` - HID descriptor dumps and descriptor-derived metadata.
- `heuristics/` - notes that explain hardware detection heuristics.
- `licensing/` - license tracking and reuse documentation.
- `reports/` - captured reports and report-derived reference material.
