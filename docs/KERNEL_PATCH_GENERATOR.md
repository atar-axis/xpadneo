# Kernel Patch Generator for xpadneo

This document explains how to use the `generate-patch.py` script to create a kernel integration patch for the xpadneo
Xbox Wireless Controller driver. This script is designed to streamline the process of integrating xpadneo into any
Linux kernel source tree.


## What it does

The `generate-patch.py` script automatically performs the following tasks:

1. **Creates isolated worktrees:** It sets up temporary Git worktrees for both your specified Linux kernel repository
   and the xpadneo project. This ensures a clean and non-disruptive generation process.
2. **Integrates xpadneo sources:** It copies the necessary xpadneo driver source files into the kernel's
   `drivers/hid/xpadneo/` directory.
3. **Configures kernel build:**
   - It adds a new Kconfig entry (`HID_XPADNEO`) to the kernel's `drivers/hid/Kconfig`, allowing the xpadneo driver to
     be selected during kernel configuration. The Kconfig content is sourced from
     `misc/generate-kernel-patch/xpadneo.Kconfig`.
   - It creates/modifies the Makefile in `drivers/hid/xpadneo/` to correctly compile the xpadneo driver sources. This
     Makefile is adapted from the xpadneo project's own `hid-xpadneo/src/Makefile`.
   - It modifies the kernel's `drivers/hid/Makefile` to include the new xpadneo module.
4. **Conditional compilation for `hid-microsoft.c`:** It wraps existing Xbox controller entries in
   `drivers/hid/hid-microsoft.c` with an `#if !defined(CONFIG_HID_XPADNEO)` block. This ensures that the generic
   `hid-microsoft` driver does not claim Bluetooth-connected Xbox controllers if `HID_XPADNEO` is enabled.
5. **Generates a Git commit and patch:** It creates a temporary Git commit in the kernel worktree containing all
   xpadneo-related changes. This commit message is generated from a template
   (`misc/generate-kernel-patch/commit-message.tmpl`) and automatically includes a `Signed-off-by` line using your
   local Git configuration (`user.name` and `user.email`).
6. **Exports a clean patch file:** The generated Git commit is then exported as a standard `.patch` file using
   `git format-patch`.
7. **Generates udev rules and modprobe configuration:**
   - It creates `60-xpadneo-kernel.rules` and `70-xpadneo-kernel-disable-hidraw.rules` files from the xpadneo project's
     `etc-udev-rules.d` directory. Comments and the `ACTION=="bind"` rule (not needed for in-tree kernel integration)
     are filtered out from the `60-` rules.
   - It creates an `xpadneo-kernel.conf` file for modprobe from the xpadneo project's `etc-modprobe.d` directory.
8. **Verifies patch application:** It performs a dry-run test (`git apply --check`) to ensure the generated patch can
   be applied cleanly to the kernel source.
9. **Cleans up:** All temporary Git worktrees and intermediate files are removed, leaving only the final artifacts.


## Usage

The script takes two positional arguments:

```bash
misc/generate-kernel-patch/generate-patch.py <KERNEL_REPO_PATH> [<KERNEL_VERSION_REF>]
```

- `<KERNEL_REPO_PATH>`: The absolute or relative path to your Linux kernel Git repository checkout.
- `[<KERNEL_VERSION_REF>]`: (Optional) A Git tag, branch name, or commit hash specifying the kernel version to
  integrate against. If omitted, it defaults to `HEAD` of the kernel repository.

**Example:**

```bash
# Generate patch for the 'main' branch of a kernel repo located at ../linux-main
misc/generate-kernel-patch/generate-patch.py ../linux-main main

# Generate patch for kernel version 6.18.0 from a repo located at ../linux-xpadneo
misc/generate-kernel-patch/generate-patch.py ../linux-xpadneo v6.18.0

# Generate patch for the current HEAD of a kernel repo
misc/generate-kernel-patch/generate-patch.py /path/to/my/kernel/repo
```


## Output

All generated files are placed in a structured subdirectory within `misc/generate-kernel-patch/out/`:

`misc/generate-kernel-patch/out/xpadneo-<XVERSION>/linux-<LVERSION>/`

Where:
- `<XVERSION>` is the detected version of the xpadneo driver (e.g., `v0.11-pre-38-g205a591`).
- `<LVERSION>` is the detected version of the Linux kernel (e.g., `6.18.0`).

The output directory will contain:

- `xpadneo-kernel-integration.patch`: The main patch file to apply to your kernel source tree.
- `60-xpadneo-kernel.rules`: Udev rules for device permissions (e.g., for Steam Input).
- `70-xpadneo-kernel-disable-hidraw.rules`: Udev rules to disable hidraw for specific devices.
- `xpadneo-kernel.conf`: Modprobe configuration for module aliases and dependencies.


## Applying the Generated Patch

Once the script has successfully run, you can apply the generated `xpadneo-kernel-integration.patch` to your kernel
source tree:

```bash
cd /path/to/your/kernel/repo
git apply /path/to/xpadneo/misc/generate-kernel-patch/out/xpadneo-<XVERSION>/linux-<LVERSION>/xpadneo-kernel-integration.patch
```

Alternatively, you can apply it using `patch`:

```bash
cd /path/to/your/kernel/repo
patch -p1 < /path/to/xpadneo/misc/generate-kernel-patch/out/xpadneo-<XVERSION>/linux-<LVERSION>/xpadneo-kernel-integration.patch
```

After rebuilding and installing your kernel, you should install the generated udev rules and modprobe configuration:

```bash
# Copy udev rules
sudo cp /path/to/xpadneo/misc/generate-kernel-patch/out/xpadneo-<XVERSION>/linux-<LVERSION>/60-xpadneo-kernel.rules /etc/udev/rules.d/
sudo cp /path/to/xpadneo/misc/generate-kernel-patch/out/xpadneo-<XVERSION>/linux-<LVERSION>/70-xpadneo-kernel-disable-hidraw.rules /etc/udev/rules.d/
sudo udevadm control --reload # Reload udev rules

# Copy modprobe configuration
sudo cp /path/to/xpadneo/misc/generate-kernel-patch/out/xpadneo-<XVERSION>/linux-<LVERSION>/xpadneo-kernel.conf /etc/modprobe.d/
sudo depmod -a # Update module dependencies
```

For packaging (e.g., RPM spec files), you might use `install -Dm644`:

```bash
install -Dm644 /path/to/xpadneo/misc/generate-kernel-patch/out/xpadneo-<XVERSION>/linux-<LVERSION>/60-xpadneo-kernel.rules %{buildroot}/usr/lib/udev/rules.d/60-xpadneo-kernel.rules
install -Dm644 /path/to/xpadneo/misc/generate-kernel-patch/out/xpadneo-<XVERSION>/linux-<LVERSION>/70-xpadneo-kernel-disable-hidraw.rules %{buildroot}/usr/lib/udev/rules.d/70-xpadneo-kernel-disable-hidraw.rules
install -Dm644 /path/to/xpadneo/misc/generate-kernel-patch/out/xpadneo-<XVERSION>/linux-<LVERSION>/xpadneo-kernel.conf %{buildroot}/usr/lib/modprobe.d/xpadneo-kernel.conf
```

Note the slight change in output filename for the udev rules (`60-xpadneo.rules` vs `60-xpadneo-kernel.rules`) for
direct installation to avoid file conflicts with a stand-alone version of xpadneo.
