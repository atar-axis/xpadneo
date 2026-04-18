<!-- SPDX-License-Identifier: GPL-2.0-only -->

# Contributing and licensing

xpadneo uses file-level licensing via SPDX identifiers.


## License scope

- Kernel driver code (files linked into the kernel module, especially `hid-xpadneo/src/**`) is licensed under
  **GPL-2.0-only**.
- Non-driver content (for example documentation, examples, tooling, and helper scripts) is generally licensed under
  **GPL-3.0-or-later**, unless a file states otherwise.


## Your contribution

By submitting a contribution, you agree that your changes are licensed under the license specified by the SPDX header
in each file you modify.

If you add a new file, you must add an SPDX license identifier at the top of that file and choose the license
appropriate for its scope.


## Signed-off-by requirement

This project encourages the use of `Signed-off-by` lines in commits to certify that you, as a contributor, adhere to
the Developer's Certificate of Origin (DCO).

- If you **use AI tools** for your contribution (as described in the "Contributions using Artificial Intelligence"
  section), adding your `Signed-off-by` line is **mandatory** to confirm your responsibility for the code.
- If your contribution **does not involve AI tools**, a `Signed-off-by` line is **optional**. While not strictly
  enforced for every commit in this scenario, it is a good practice that certifies your contribution to the project.

For your `Signed-off-by` line, you should use your real name or a pseudonym. The name and email address you use should
identify you consistently across your contributions.


## Contributions using Artificial Intelligence

We acknowledge that Artificial Intelligence (AI) tools, such as code assistants or chatbots, can be useful aids in
software development. Contributions created with the help of such tools are welcome in this project, but they are
subject to clear rules to ensure legal integrity and high code quality.

If you use AI to create your contribution, the full responsibility for the resulting code lies with you as the human
author. You must ensure that the contribution has been fully reviewed, tested, and verified by you. By adding your
`Signed-off-by` line to the commit, you confirm that you are the author of the commit and take full responsibility
for it.

We consider 'help from AI' to be any form of assistance, including, but not limited to:

- Code completion by tools like GitHub Copilot.
- Generation of code blocks by AI models.
- Research and problem-solving using AI chatbots.
- AI-assisted reviews and suggestions (e.g., GitHub Copilot PR suggestions).

Since you, as the human, take responsibility, commits must **not** contain a `Co-authored-by` signature of an AI agent.
The use of AI must be declared accordingly in the pull request.


## Reference

See `LICENSE.md` and the full license texts in:

- `LICENSES/LICENSE.gpl-2.0.txt`
- `LICENSES/LICENSE.gpl-3.0.txt`
