#!/usr/bin/env python3
"""
Generate xpadneo Kernel Integration Patch

This script automatically generates a kernel patch to integrate xpadneo
for any Linux kernel version, along with required udev rules and modprobe
configuration files.

Usage: ./generate-xpadneo-patch.py <kernel-version>
Example: ./generate-xpadneo-patch.py 6.20.5

Output:
  - xpadneo-kernel-integration.patch - Kernel patch
  - 60-xpadneo.rules - Udev rules for device permissions
  - xpadneo.conf - Modprobe configuration and module aliases

Requirements:
  - linux-<version>/ directory with kernel source
  - xpadneo/ directory with xpadneo git repository
"""

import os
import sys
import shutil
import subprocess
import re
from pathlib import Path
from datetime import datetime

# ANSI color codes
class Colors:
    RED = '\033[0;31m'
    GREEN = '\033[0;32m'
    YELLOW = '\033[1;33m'
    BLUE = '\033[0;34m'
    BOLD = '\033[1m'
    NC = '\033[0m'

def error(msg):
    """Print error message and exit"""
    print(f"{Colors.RED}ERROR: {msg}{Colors.NC}", file=sys.stderr)
    sys.exit(1)

def warning(msg):
    """Print warning message"""
    print(f"{Colors.YELLOW}WARNING: {msg}{Colors.NC}", file=sys.stderr)

def info(msg):
    """Print info message"""
    print(f"{Colors.BLUE}INFO: {msg}{Colors.NC}")

def success(msg):
    """Print success message"""
    print(f"{Colors.GREEN}SUCCESS: {msg}{Colors.NC}")

def run_command(cmd, cwd=None, capture=True, check=True):
    """Run shell command and return output"""
    try:
        if capture:
            result = subprocess.run(cmd, shell=True, cwd=cwd, capture_output=True, text=True, check=check)
            return result.stdout.strip() if result.stdout else ""
        else:
            subprocess.run(cmd, shell=True, cwd=cwd, check=check)
            return None
    except subprocess.CalledProcessError as e:
        if capture and e.stdout:
            return e.stdout.strip()
        return None

def run_diff(cmd, cwd=None):
    """Run diff command - exit code 1 means files differ (expected)"""
    result = subprocess.run(cmd, shell=True, cwd=cwd, capture_output=True, text=True)
    # diff returns 0 if no differences, 1 if differences found, 2+ for errors
    if result.returncode in [0, 1]:
        return result.stdout
    else:
        error(f"diff command failed with exit code {result.returncode}: {result.stderr}")
        return None

def get_xpadneo_version(xpadneo_dir):
    """Detect xpadneo version from VERSION file or git"""
    version_file = xpadneo_dir / "VERSION"

    # Try VERSION file first
    if version_file.exists():
        return version_file.read_text().strip()

    # Try git describe
    if (xpadneo_dir / ".git").exists():
        version = run_command("git describe --tags --always", cwd=xpadneo_dir)
        if version:
            return version

    return "unknown"

def find_kconfig_insertion_point(kconfig_content):
    """Find the best place to insert xpadneo Kconfig entry"""
    lines = kconfig_content.split('\n')

    # Look for "endif # HID" - most common
    for i, line in enumerate(lines):
        if 'endif # HID' in line and 'HID_SUPPORT' not in line:
            return i

    # Fallback: look for "endif # HID_SUPPORT"
    for i, line in enumerate(lines):
        if 'endif # HID_SUPPORT' in line:
            return i

    return None

def create_xpadneo_kconfig():
    """Generate xpadneo Kconfig content"""
    return """# SPDX-License-Identifier: GPL-2.0-only
config HID_XPADNEO
	tristate "Xbox Wireless Controller Bluetooth support (xpadneo)"
	depends on HID && BT_HIDP
	help
	  Support for Xbox One S/X, Xbox Series X|S, and Xbox Elite Series 2
	  controllers connected via Bluetooth.

	  This driver provides advanced features including:
	  - Force feedback (rumble) support for main and trigger motors
	  - Battery level reporting
	  - Profile switching (Elite controllers)
	  - Mouse mode emulation
	  - Improved button mappings

	  This driver will be preferred over the generic HID driver for
	  Xbox controllers connected via Bluetooth.

	  To compile this driver as a module, choose M here: the module
	  will be called hid-xpadneo.
"""

def create_xpadneo_makefile(xpadneo_version):
    """Generate xpadneo Makefile content"""
    return f"""# SPDX-License-Identifier: GPL-2.0-only

ccflags-y += -DVERSION={xpadneo_version}

obj-$(CONFIG_HID_XPADNEO) += hid-xpadneo.o

hid-xpadneo-y := \\
	consumer.o \\
	core.o \\
	debug.o \\
	device.o \\
	events.o \\
	keyboard.o \\
	mappings.o \\
	mouse.o \\
	power.o \\
	quirks.o \\
	rumble.o \\
	synthetic.o
"""

def create_udev_rules():
    """Generate xpadneo udev rules content"""
    return """# xpadneo - Xbox Wireless Controller driver udev rules
# This file provides proper permissions and access for xpadneo controllers

# Rebind driver to xpadneo
ACTION=="bind", SUBSYSTEM=="hid", DRIVER!="xpadneo", KERNEL=="0005:045E:*", KERNEL=="*:02FD.*|*:02E0.*|*:0B05.*|*:0B13.*|*:0B20.*|*:0B22.*", ATTR{driver/unbind}="%k", ATTR{[drivers/hid:xpadneo]bind}="%k"

# Tag xpadneo devices for access in the user session
# MODE="0664" allows user read/write access for proper Steam Input detection
# LIBINPUT_IGNORE_DEVICE prevents desktop environments from using controller as mouse
ACTION!="remove", DRIVERS=="xpadneo", SUBSYSTEM=="input", ENV{ID_INPUT_JOYSTICK}=="1", TAG+="uaccess", MODE="0664", ENV{LIBINPUT_IGNORE_DEVICE}="1"

# Allow hidraw access for Steam Input to properly detect controller models and features
# This is needed with PID spoofing disabled (default) for Elite paddle detection
ACTION!="remove", DRIVERS=="xpadneo", SUBSYSTEM=="hidraw", TAG+="uaccess", MODE="0660"
"""

def create_modprobe_conf():
    """Generate xpadneo modprobe configuration"""
    return """# xpadneo - Xbox Wireless Controller driver module configuration

# Module aliases for automatic loading
alias hid:b0005g*v0000045Ep000002E0 hid_xpadneo
alias hid:b0005g*v0000045Ep000002FD hid_xpadneo
alias hid:b0005g*v0000045Ep00000B05 hid_xpadneo
alias hid:b0005g*v0000045Ep00000B13 hid_xpadneo
alias hid:b0005g*v0000045Ep00000B20 hid_xpadneo
alias hid:b0005g*v0000045Ep00000B22 hid_xpadneo

# Ensure uhid is loaded before bluetooth for controller firmware 5.x support
softdep bluetooth pre: uhid

# Optional: Enable PID spoofing for legacy SDL2 (<2.28) compatibility
# Uncomment the line below if you have games with SDL2 < 2.28 button mapping issues
# options hid_xpadneo enable_pid_spoof=1
"""

def create_patch_header(xpadneo_version):
    """Generate patch header"""
    return f"""Add xpadneo Xbox Wireless Controller driver for Bluetooth support

This patch integrates the xpadneo driver ({xpadneo_version}) into the kernel tree.
xpadneo provides advanced support for Xbox One S/X, Series X|S, and Elite
controllers connected via Bluetooth, including force feedback, battery
reporting, and profile switching.

Signed-off-by: GloriousEggroll <gloriouseggroll@gmail.com>
---

"""

def generate_patch(kernel_version):
    """Main patch generation function"""
    script_dir = Path(__file__).parent.absolute()
    kernel_dir = script_dir / f"linux-{kernel_version}"
    xpadneo_dir = script_dir / "xpadneo"
    xpadneo_src = xpadneo_dir / "hid-xpadneo" / "src" / "xpadneo"
    patch_file = script_dir / "xpadneo-kernel-integration.patch"

    print("=" * 60)
    print("  xpadneo Kernel Integration Patch Generator (Python)")
    print("=" * 60)
    print()
    info(f"Target kernel version: {kernel_version}")
    info(f"Kernel directory: {kernel_dir}")
    info(f"xpadneo directory: {xpadneo_dir}")
    print()
    info("Output files:")
    info(f"  - Kernel patch: {patch_file}")
    info(f"  - Udev rules: 60-xpadneo.rules")
    info(f"  - Modprobe config: xpadneo.conf")

    # Warn if patch file already exists
    if patch_file.exists():
        warning(f"Patch file already exists and will be overwritten")

    print()

    # Validate kernel version format
    if not re.match(r'^\d+\.\d+\.\d+$', kernel_version):
        warning(f"Kernel version format doesn't match X.Y.Z pattern: {kernel_version}")
        response = input("Continue anyway? [y/N]: ")
        if response.lower() != 'y':
            sys.exit(0)

    # Validate directories
    if not kernel_dir.exists():
        error(f"Kernel directory not found: {kernel_dir}")

    if not xpadneo_dir.exists():
        error(f"xpadneo directory not found: {xpadneo_dir}")

    if not xpadneo_src.exists():
        error(f"xpadneo source directory not found: {xpadneo_src}")

    if not (kernel_dir / "Makefile").exists():
        error("Not a valid kernel source tree (Makefile not found)")

    if not (kernel_dir / "drivers" / "hid").exists():
        error("drivers/hid directory not found in kernel source")

    # Check if xpadneo already integrated
    if (kernel_dir / "drivers" / "hid" / "xpadneo").exists():
        error("xpadneo directory already exists in kernel tree. Please use a clean kernel source.")

    # Get xpadneo version
    info("Detecting xpadneo version...")
    xpadneo_version = get_xpadneo_version(xpadneo_dir)
    info(f"xpadneo version: {xpadneo_version}")
    print()

    # Create temporary working directory
    work_dir = script_dir / f".patch-work-{os.getpid()}"
    info("Creating temporary working directory...")

    if work_dir.exists():
        shutil.rmtree(work_dir)
    work_dir.mkdir()

    try:
        # Copy kernel source
        info("Creating clean kernel copies...")
        clean_dir = work_dir / "linux-clean"
        patched_dir = work_dir / "linux-patched"

        # Ignore function to skip .git and build artifacts
        def ignore_patterns(directory, contents):
            ignored = set()
            for item in contents:
                # Skip .git, build output, and temp files
                if item in {'.git', '.gitignore', '.config', '.config.old',
                           '.version', 'Module.symvers', 'modules.order',
                           'modules.builtin', 'modules.builtin.modinfo'}:
                    ignored.add(item)
                # Skip compiled files
                elif item.endswith(('.o', '.ko', '.cmd', '.a', '.so')):
                    ignored.add(item)
            return ignored

        shutil.copytree(kernel_dir, clean_dir, symlinks=True, ignore=ignore_patterns)
        shutil.copytree(kernel_dir, patched_dir, symlinks=True, ignore=ignore_patterns)

        # Copy xpadneo sources
        info("Copying xpadneo sources to kernel tree...")
        xpadneo_dest = patched_dir / "drivers" / "hid" / "xpadneo"
        xpadneo_dest.mkdir(parents=True)

        # Copy .c and .h files
        copied_files = 0
        for ext in ['*.c', '*.h']:
            for src_file in xpadneo_src.glob(ext):
                shutil.copy2(src_file, xpadneo_dest)
                copied_files += 1

        info(f"Copied {copied_files} source files")

        if copied_files == 0:
            error(f"No source files found in {xpadneo_src}")

        # Create Kconfig
        info("Creating xpadneo Kconfig...")
        (xpadneo_dest / "Kconfig").write_text(create_xpadneo_kconfig())

        # Create Makefile
        info("Creating xpadneo Makefile...")
        (xpadneo_dest / "Makefile").write_text(create_xpadneo_makefile(xpadneo_version))

        # Modify drivers/hid/Kconfig
        info("Modifying drivers/hid/Kconfig...")
        kconfig_file = patched_dir / "drivers" / "hid" / "Kconfig"
        kconfig_content = kconfig_file.read_text()

        insertion_point = find_kconfig_insertion_point(kconfig_content)
        if insertion_point is None:
            error("Could not find suitable insertion point in drivers/hid/Kconfig")

        lines = kconfig_content.split('\n')
        lines.insert(insertion_point, '\nsource "drivers/hid/xpadneo/Kconfig"\n')
        kconfig_file.write_text('\n'.join(lines))

        # Verify
        if 'xpadneo/Kconfig' not in kconfig_file.read_text():
            error("Failed to modify drivers/hid/Kconfig")
        success("Modified drivers/hid/Kconfig")

        # Modify drivers/hid/Makefile
        info("Modifying drivers/hid/Makefile...")
        makefile = patched_dir / "drivers" / "hid" / "Makefile"
        makefile_content = makefile.read_text()

        if not makefile_content.endswith('\n'):
            makefile_content += '\n'
        makefile_content += '\nobj-$(CONFIG_HID_XPADNEO)\t+= xpadneo/\n'
        makefile.write_text(makefile_content)

        # Verify
        if 'CONFIG_HID_XPADNEO' not in makefile.read_text():
            error("Failed to modify drivers/hid/Makefile")
        success("Modified drivers/hid/Makefile")

        # Remove Xbox controller entries from hid-microsoft.c to give xpadneo priority
        info("Removing Xbox controller entries from hid-microsoft.c...")
        ms_driver = patched_dir / "drivers" / "hid" / "hid-microsoft.c"
        ms_content = ms_driver.read_text()

        # Find and remove the Xbox controller device table entries
        lines = ms_content.split('\n')
        new_lines = []
        skip_next = False
        removed_count = 0
        comment_added = False

        for i, line in enumerate(lines):
            # Skip lines marked from previous iteration (e.g., .driver_data lines)
            if skip_next:
                skip_next = False
                removed_count += 1
                continue

            # Check if this line is an Xbox controller entry
            if ('USB_DEVICE_ID_MS_XBOX_CONTROLLER' in line or
                'USB_DEVICE_ID_8BITDO_SN30_PRO_PLUS' in line) and 'HID_BLUETOOTH_DEVICE' in line:
                # Add comment only once before first removal
                if not comment_added:
                    new_lines.append('')
                    new_lines.append('\t/* Xbox controller entries removed - handled by hid-xpadneo driver */')
                    comment_added = True
                removed_count += 1
                skip_next = True  # Skip the next line too (.driver_data)
                continue

            new_lines.append(line)

        ms_driver.write_text('\n'.join(new_lines))
        success(f"Removed {removed_count} Xbox controller device entries from hid-microsoft.c")

        # Generate diff
        info("Generating unified diff patch...")
        print()

        os.chdir(work_dir)
        diff_output = run_diff("diff -Naur linux-clean linux-patched")

        # Create patch file with header
        patch_content = create_patch_header(xpadneo_version)
        if diff_output:
            patch_content += diff_output
        else:
            error("Failed to generate diff output")

        patch_file.write_text(patch_content)

    finally:
        # Clean up
        os.chdir(script_dir)
        if work_dir.exists():
            shutil.rmtree(work_dir)

    # Validate patch
    if not patch_file.exists():
        error("Patch file was not created")

    # Generate udev rules and modprobe configuration
    info("Generating udev rules and modprobe configuration...")
    udev_rules_file = script_dir / "60-xpadneo.rules"
    modprobe_conf_file = script_dir / "xpadneo.conf"

    udev_rules_file.write_text(create_udev_rules())
    modprobe_conf_file.write_text(create_modprobe_conf())

    success(f"Created udev rules: {udev_rules_file}")
    success(f"Created modprobe config: {modprobe_conf_file}")

    # Get patch statistics
    patch_size = patch_file.stat().st_size
    patch_lines = len(patch_file.read_text().split('\n'))
    file_count = patch_file.read_text().count('diff --git')

    print()
    print("=" * 60)
    print("  Patch Generation Complete")
    print("=" * 60)
    print()
    success(f"Patch file: {patch_file}")
    success(f"Udev rules: {udev_rules_file}")
    success(f"Modprobe config: {modprobe_conf_file}")
    print()
    info(f"Patch size: {patch_size / 1024:.1f} KB ({patch_lines} lines)")
    info(f"Files modified/added: {file_count}")
    print()

    # Show summary
    print("Summary of changes:")
    print("-" * 60)
    patch_text = patch_file.read_text()

    print("Modified files:")
    for line in patch_text.split('\n'):
        if line.startswith('diff --git a/drivers/hid/') and 'xpadneo' not in line:
            parts = line.split()
            if len(parts) >= 3:
                print(f"  - {parts[2].replace('a/', '')}")

    print()
    print("New directory: drivers/hid/xpadneo/")
    xpadneo_files = [line for line in patch_text.split('\n')
                     if line.startswith('diff --git a/drivers/hid/xpadneo')]
    print(f"  {len(xpadneo_files)} files added")
    print()

    # Test patch application
    info("Testing patch application (dry-run)...")
    os.chdir(kernel_dir)
    # Use check=False because patch returns non-zero on failure, which is expected
    result = subprocess.run(f"patch -p1 --dry-run < {patch_file}", shell=True,
                           capture_output=True, text=True)
    if result.returncode == 0:
        success(f"Patch applies cleanly to Linux {kernel_version}!")
    else:
        warning("Patch may not apply cleanly. Manual adjustments might be needed.")
        if result.stderr:
            print(f"  Error details: {result.stderr[:200]}")
    os.chdir(script_dir)

    print()
    print("To apply this patch:")
    print(f"  cd linux-{kernel_version}")
    print(f"  patch -p1 < {patch_file}")
    print()
    print("Or with git:")
    print(f"  cd linux-{kernel_version}")
    print(f"  git apply {patch_file}")
    print()
    print("After building and installing the kernel, install udev rules:")
    print(f"  sudo cp {udev_rules_file.name} /etc/udev/rules.d/")
    print(f"  sudo cp {modprobe_conf_file.name} /etc/modprobe.d/")
    print("  sudo udevadm control --reload")
    print()
    print("Or for packaging (e.g., RPM spec):")
    print(f"  install -Dm644 {udev_rules_file.name} %{{buildroot}}/usr/lib/udev/rules.d/60-xpadneo.rules")
    print(f"  install -Dm644 {modprobe_conf_file.name} %{{buildroot}}/usr/lib/modprobe.d/xpadneo.conf")
    print()

def main():
    """Main entry point"""
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <kernel-version>", file=sys.stderr)
        print()
        print("Examples:")
        print(f"  {sys.argv[0]} 6.19.9")
        print(f"  {sys.argv[0]} 6.20.0")
        print()
        sys.exit(1)

    kernel_version = sys.argv[1]

    try:
        generate_patch(kernel_version)
    except KeyboardInterrupt:
        print("\n\nInterrupted by user")
        sys.exit(130)
    except Exception as e:
        error(f"Unexpected error: {e}")

if __name__ == "__main__":
    main()
