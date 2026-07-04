<!-- SPDX-License-Identifier: GPL-2.0-only -->

# DOX Framework

## Purpose

This subtree owns the Linux HID module source and the module build wrapper.


## Ownership

- Kernel module makefile.
- xpadneo driver implementation under `xpadneo/`.


## Local Contracts

- Keep driver behavior conservative on `master` and stable branches.
- Do not mix bug fixes with default flips, behavior changes, or broad driver
  rewrites.
- Treat report descriptor rewriting, PID spoofing, hidraw policy, HOGP
  behavior, rumble, and device classification as compatibility-sensitive.
- Isolate hardware-specific quirks behind device flags or narrowly scoped
  detection logic.
- Document descriptor or heuristic evidence when a code change depends on new
  hardware identification facts.


## Work Guidance

- Follow existing helper APIs and local coding style before adding new
  abstractions.
- Prefer feature or quirk flags over open-coded special cases when the behavior
  belongs to a device capability.
- Keep risky architecture work in focused long-lived branches or pull requests
  for a later milestone.


## Verification

- Run `make -C hid-xpadneo modules` for driver logic changes when possible.
- If module dependencies or aliases change, inspect module metadata with local
  tooling when available.
- Report when the local kernel build tree is unavailable.


## Child DOX Index

- `xpadneo/` - core driver C sources, headers, event handling, mapping, and
  rumble implementation.
