<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# DOX Framework

## Purpose

This subtree owns the installable xpadneo kernel module package and its DKMS,
udev, and modprobe integration.


## Ownership

- Module package makefiles and DKMS metadata.
- DKMS hook scripts.
- udev and modprobe configuration installed with the driver.
- Kernel module source under `src/`.


## Local Contracts

- Preserve DKMS install semantics unless the task explicitly changes packaging
  behavior.
- Treat udev, modprobe, and module dependency changes as system integration
  changes with user-visible side effects.
- Keep module loading assumptions aligned with the in-module soft dependency
  and documented system-start behavior.
- Follow each file's existing SPDX license. Driver source linked into the kernel
  follows its kernel-facing license conventions.


## Work Guidance

- Do not run privileged install, uninstall, reload, or DKMS commands unless the
  user explicitly asks for that action.
- Keep packaging changes separate from driver logic changes when possible.
- Read the child guide before changing driver source files.


## Verification

- For module builds, prefer `make -C hid-xpadneo modules` when the local kernel
  build tree is available.
- For DKMS metadata changes, run `make -C hid-xpadneo dkms.conf` or the smallest
  equivalent generation check.
- For shell hook changes, run `bash -n` on the touched hook scripts.


## Child DOX Index

- `src/` - Linux HID module source and build wrapper.
