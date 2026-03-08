<!-- SPDX-License-Identifier: GPL-2.0-only -->

# Licensing

This repository contains components under different licenses.


## Summary

- Kernel driver code (linked into the kernel module), especially files under:
  - `hid-xpadneo/src/**`
  is licensed under **GPL-2.0-only**.

- Non-driver content is generally GPL-3.0-or-later **unless file headers specify otherwise**. This includes:
  - Documentation (`docs/**`)
  - Examples (`examples/**`)
  - Tooling and helper scripts (`tools/**`, `scripts/**`)


## Canonical license texts

The full license texts are provided in:

- `LICENSES/LICENSE.gpl-2.0.txt`
- `LICENSES/LICENSE.gpl-3.0.txt`


## File-level license identifiers (SPDX)

This project uses SPDX license identifiers in file headers.

- For C / header files:
  - `// SPDX-License-Identifier: GPL-2.0-only`
  - or `// SPDX-License-Identifier: GPL-3.0-or-later`
- For Markdown files:
  - `<!-- SPDX-License-Identifier: ... -->`

The SPDX identifier in each file is the authoritative license marker for that file.


## Notes on scope

The GPL-2.0-only scope is intentionally limited to code that is part of the kernel module.
Documentation and other non-module assets are out of that scope and can remain GPL-3.0-or-later.


## Contributor guidance

By submitting contributions, you agree that your changes are licensed under the license indicated by the SPDX header
of the file(s) you modify.

If a new file is added, include an SPDX license identifier at the top of the file and choose the appropriate license
for its scope.
