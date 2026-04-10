#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""
Generate xpadneo Kernel Integration Patch

This script automatically generates a kernel patch to integrate xpadneo
for any Linux kernel version, along with required udev rules and modprobe
configuration files.

- It uses git worktrees to avoid copying large sources.
- It defaults to the current kernel version of the git source tree if no
  version is specified.

Usage: misc/generate-kernel-patch/generate-patch.py <kernel-git-tree> [<kernel-version>]
Example: misc/generate-kernel-patch/generate-patch.py ../linux 6.20.5

Output:
  - misc/generate-kernel-patch/out/
    - xpadneo-kernel-integration.patch - Kernel patch
    - 60-xpadneo.rules - Udev rules for device permissions
    - xpadneo.conf - Modprobe configuration and module aliases

Requirements:
  - kernel git source tree checkout
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path


# ANSI color codes
class Colors:
    RED = "\033[0;31m"
    GREEN = "\033[0;32m"
    YELLOW = "\033[1;33m"
    BLUE = "\033[0;34m"
    BOLD = "\033[1m"
    NC = "\033[0m"


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
        print(f"{Colors.RED}Command failed: {cmd}{Colors.NC}", file=sys.stderr)
        if e.stdout:
            print(f"STDOUT: {e.stdout}", file=sys.stderr)
        if e.stderr:
            print(f"STDERR: {e.stderr}", file=sys.stderr)
        if check:
            sys.exit(1)
        return e.stdout.strip() if capture and e.stdout else None


def get_xpadneo_version(xpadneo_dir):
    """Detect xpadneo version from VERSION file or git"""
    version_file = xpadneo_dir / "VERSION"
    if version_file.exists():
        return version_file.read_text().strip()
    if (xpadneo_dir / ".git").is_dir() or (xpadneo_dir / ".git").is_file():
        version = run_command("git describe --tags --always", cwd=xpadneo_dir)
        if version:
            return version
    return "unknown"


def get_kernel_version_from_makefile(kernel_dir):
    """Extracts the kernel version from the Makefile."""
    makefile_path = kernel_dir / "Makefile"
    if not makefile_path.exists():
        return None
    try:
        content = makefile_path.read_text()
        version_match = re.search(r"^VERSION\s*=\s*(\d+)", content, re.M)
        patchlevel_match = re.search(r"^PATCHLEVEL\s*=\s*(\d+)", content, re.M)
        sublevel_match = re.search(r"^SUBLEVEL\s*=\s*(\d+)", content, re.M)
        if version_match and patchlevel_match and sublevel_match:
            return f"{version_match.group(1)}.{patchlevel_match.group(1)}.{sublevel_match.group(1)}"
    except Exception as e:
        warning(f"Could not read kernel Makefile: {e}")
    return None


def find_kconfig_insertion_point(kconfig_content):
    """Find the best place to insert xpadneo Kconfig entry"""
    lines = kconfig_content.split("\n")
    for i, line in enumerate(lines):
        if "endif # HID" in line and "HID_SUPPORT" not in line:
            return i
    for i, line in enumerate(lines):
        if "endif # HID_SUPPORT" in line:
            return i
    return None


def run_patch_generation(kernel_repo_path, xpadneo_repo_path, kernel_ref, out_dir, script_dir):
    """Main patch generation function"""
    work_dir = out_dir / f"work-{os.getpid()}"
    work_dir.mkdir()

    kernel_worktree = work_dir / "linux"
    xpadneo_worktree = work_dir / "xpadneo"

    try:
        # 1. Setup worktrees
        info("Creating git worktrees...")
        run_command(f"git worktree add --detach {kernel_worktree} {kernel_ref}", cwd=kernel_repo_path, capture=False)
        run_command(f"git worktree add --detach {xpadneo_worktree} HEAD", cwd=xpadneo_repo_path, capture=False)

        kernel_version = get_kernel_version_from_makefile(kernel_worktree)
        if not kernel_version:
            error("Could not determine kernel version from Makefile. Cannot proceed.")

        xpadneo_src = xpadneo_worktree / "hid-xpadneo" / "src" / "xpadneo"

        # 3. Get xpadneo version
        info("Detecting xpadneo version...")
        xpadneo_version = get_xpadneo_version(xpadneo_worktree)
        info(f"xpadneo version: {xpadneo_version}")

        # Define the final output directory structure AFTER versions are known
        final_output_dir = out_dir / f"xpadneo-{xpadneo_version}" / f"linux-{kernel_version}"
        final_output_dir.mkdir(parents=True)

        patch_file = final_output_dir / "xpadneo-kernel-integration.patch"
        udev_rules_file_loader = final_output_dir / "60-xpadneo-kernel.rules"
        udev_rules_file_hidraw = final_output_dir / "70-xpadneo-kernel-disable-hidraw.rules"
        modprobe_conf_file = final_output_dir / "xpadneo-kernel.conf"

        print("=" * 60)
        print("  xpadneo Kernel Integration Patch Generator")
        print("=" * 60)
        print()
        info(f"Target kernel version: {kernel_version} (from ref: {kernel_ref})")
        info(f"Kernel source: {kernel_worktree}")
        info(f"xpadneo source: {xpadneo_worktree}")
        print()
        info("Output files:")
        info(f"  - Kernel patch: {patch_file}")
        info("  - Udev rules:")
        info(f"    - {udev_rules_file_loader}")
        info(f"    - {udev_rules_file_hidraw}")
        info(f"  - Modprobe config: {modprobe_conf_file}")
        print()

        # 4. Sanity checks
        if not xpadneo_src.exists():
            error(f"xpadneo source directory not found: {xpadneo_src}")
        if not (kernel_worktree / "Makefile").exists():
            error("Not a valid kernel source tree (Makefile not found)")
        if (kernel_worktree / "drivers" / "hid" / "xpadneo").exists():
            error("xpadneo directory already exists in kernel tree. Please use a clean kernel source.")

        # 5. Patching logic (directly on worktree)
        info("Applying modifications directly to kernel worktree...")
        xpadneo_dest = kernel_worktree / "drivers" / "hid" / "xpadneo"
        shutil.copytree(xpadneo_src, xpadneo_dest)

        info("Creating xpadneo Kconfig and Makefile...")
        (xpadneo_dest / "Kconfig").write_text((script_dir / "xpadneo.Kconfig").read_text())

        # Read the template Makefile, replace the version, and write it
        src_makefile_content = (xpadneo_worktree / "hid-xpadneo" / "src" / "Makefile").read_text()
        modified_makefile_content = src_makefile_content.replace(
            "ccflags-y += -DVERSION=$(VERSION)", f'ccflags-y += -DVERSION="{xpadneo_version}"'
        )
        (xpadneo_dest / "Makefile").write_text(modified_makefile_content)

        info("Modifying drivers/hid/Kconfig...")
        kconfig_file = kernel_worktree / "drivers" / "hid" / "Kconfig"
        kconfig_content = kconfig_file.read_text()
        insertion_point = find_kconfig_insertion_point(kconfig_content)
        if insertion_point is None:
            error("Could not find suitable insertion point in drivers/hid/Kconfig")
        lines = kconfig_content.split("\n")
        lines.insert(insertion_point, 'source "drivers/hid/xpadneo/Kconfig"\n')
        kconfig_file.write_text("\n".join(lines))
        success("Modified drivers/hid/Kconfig")

        info("Modifying drivers/hid/Makefile...")
        makefile = kernel_worktree / "drivers" / "hid" / "Makefile"
        makefile_content = makefile.read_text()
        if not makefile_content.endswith("\n"):
            makefile_content += "\n"
        makefile_content += "\nobj-$(CONFIG_HID_XPADNEO)	+= xpadneo/\n"
        makefile.write_text(makefile_content)
        success("Modified drivers/hid/Makefile")

        info("Modifying drivers/hid/hid-microsoft.c to conditionally compile Xbox controller entries...")
        ms_driver = kernel_worktree / "drivers" / "hid" / "hid-microsoft.c"
        if ms_driver.exists():
            ms_content = ms_driver.read_text()
            lines = ms_content.split("\n")

            first_xpadneo_entry_start_idx = -1
            last_xpadneo_entry_end_idx = -1

            # Find the start and end indices of the block of Xbox controller entries
            for i, line in enumerate(lines):
                if (
                    "USB_DEVICE_ID_MS_XBOX_CONTROLLER" in line or "USB_DEVICE_ID_8BITDO_SN30_PRO_PLUS" in line
                ) and "HID_BLUETOOTH_DEVICE" in line:
                    if first_xpadneo_entry_start_idx == -1:
                        first_xpadneo_entry_start_idx = i
                    # Assuming .driver_data line is immediately after the device entry line
                    last_xpadneo_entry_end_idx = i + 1

            if first_xpadneo_entry_start_idx != -1:
                new_ms_lines = []
                # Add lines before the block
                new_ms_lines.extend(lines[:first_xpadneo_entry_start_idx])

                # Insert the #if block
                new_ms_lines.append("\n#ifndef CONFIG_HID_XPADNEO")

                # Add the original lines of the block
                new_ms_lines.extend(lines[first_xpadneo_entry_start_idx : last_xpadneo_entry_end_idx + 1])

                # Insert the #endif
                new_ms_lines.append("#endif\n")

                # Add lines after the block
                new_ms_lines.extend(lines[last_xpadneo_entry_end_idx + 1 :])

                ms_driver.write_text("\n".join(new_ms_lines))
                success("Modified hid-microsoft.c to conditionally compile Xbox controller entries.")
            else:
                warning("No Xbox controller entries found in hid-microsoft.c to wrap.")
        else:
            warning("hid-microsoft.c not found, skipping modification.")

        # 6. Generate patch from commit
        info("Generating patch from git commit...")
        run_command("git add .", cwd=kernel_worktree, capture=False)

        # Create commit message from template
        commit_tmpl = (script_dir / "commit-message.tmpl").read_text()
        commit_msg = commit_tmpl.format(xpadneo_version=xpadneo_version)
        commit_msg_file = work_dir / "commit_msg.txt"
        commit_msg_file.write_text(commit_msg)

        # Create commit and export patch
        run_command(f"git commit --signoff -F {commit_msg_file}", cwd=kernel_worktree, capture=False)
        patch_content = run_command("git format-patch -1 HEAD --stdout", cwd=kernel_worktree)
        patch_file.write_text(patch_content)

        if not patch_content:
            error("Failed to generate patch with 'git format-patch'.")

        # 7. Generate other files
        info("Generating udev rules and modprobe configuration...")

        # Read and filter 60-xpadneo.rules
        rules60_content = (xpadneo_worktree / "hid-xpadneo" / "etc-udev-rules.d" / "60-xpadneo.rules").read_text()
        # Filter out comments, empty lines, and the bind action
        filtered_rules60 = [
            line.strip()
            for line in rules60_content.splitlines()
            if line.strip() and not line.strip().startswith("#") and 'ACTION=="bind"' not in line
        ]
        udev_rules_file_loader.write_text("\n".join(filtered_rules60))

        # Read and write 70-xpadneo-disable-hidraw.rules
        rules70_content = (
            xpadneo_worktree / "hid-xpadneo" / "etc-udev-rules.d" / "70-xpadneo-disable-hidraw.rules"
        ).read_text()
        # Filter out comments and empty lines
        filtered_rules70 = [
            line.strip() for line in rules70_content.splitlines() if line.strip() and not line.strip().startswith("#")
        ]
        udev_rules_file_hidraw.write_text("\n".join(filtered_rules70))

        modprobe_conf_file.write_text(
            (xpadneo_worktree / "hid-xpadneo" / "etc-modprobe.d" / "xpadneo.conf").read_text()
        )

        # 8. Final summary and verification
        patch_size = patch_file.stat().st_size
        # Get file count from the patch content itself
        file_count_str = re.search(r"(\d+)\s+files? changed", patch_content)
        file_count = int(file_count_str.group(1)) if file_count_str else 0

        print()
        print("=" * 60)
        print("  Patch Generation Complete")
        print("=" * 60)
        print()
        success(f"Patch file: {patch_file}")
        success("Udev rules:")
        success(f"  - {udev_rules_file_loader}")
        success(f"  - {udev_rules_file_hidraw}")
        success(f"Modprobe config: {modprobe_conf_file}")
        print()
        info(f"Patch size: {patch_size / 1024:.1f} KB")
        info(f"Files modified/added: {file_count}")
        print()

        info("Testing patch application (dry-run)...")
        # Reset the worktree to before our commit to test the patch
        run_command("git reset --hard HEAD~1", cwd=kernel_worktree, capture=False)
        run_command("git clean -fdx", cwd=kernel_worktree, capture=False)
        result = subprocess.run(
            f"git apply --check {patch_file.resolve()}", shell=True, cwd=kernel_worktree, capture_output=True, text=True
        )
        if result.returncode == 0:
            success(f"Patch applies cleanly to Linux {kernel_version}!")
        else:
            warning("Patch may not apply cleanly. Manual adjustments might be needed.")
            if result.stderr:
                print(f"  Details: {result.stderr[:200].strip()}...")
        print()

    finally:
        # 9. Cleanup
        info("Cleaning up temporary worktrees and files...")
        if kernel_worktree.exists():
            # The command must be run from the main git repo, not the worktree subdir
            run_command(
                f"git worktree remove --force {kernel_worktree}", cwd=kernel_repo_path, capture=False, check=False
            )
        if xpadneo_worktree.exists():
            run_command(
                f"git worktree remove --force {xpadneo_worktree}", cwd=xpadneo_repo_path, capture=False, check=False
            )
        if work_dir.exists():
            shutil.rmtree(work_dir)


def main():
    """Main entry point"""
    parser = argparse.ArgumentParser(
        description="Generate xpadneo Kernel Integration Patch.", formatter_class=argparse.RawTextHelpFormatter
    )
    parser.add_argument("kernel_repo", help="Path to the Linux kernel git repository.")
    parser.add_argument(
        "kernel_ref",
        nargs="?",  # Makes it optional
        help="Optional: A git tag, branch, or commit hash for the kernel to use.\nDefaults to HEAD of the repository.",
        default="HEAD",
    )

    args = parser.parse_args()

    script_dir = Path(__file__).parent.absolute()
    xpadneo_repo_path = (script_dir / "../../").resolve()
    kernel_repo_path = Path(args.kernel_repo).resolve()

    def is_git_repo(path):
        result = subprocess.run(
            "git rev-parse --is-inside-work-tree", shell=True, cwd=path, capture_output=True, text=True
        )
        return result.returncode == 0

    if not is_git_repo(kernel_repo_path):
        error(f"Kernel repository path not found or not a git repo: '{kernel_repo_path}'")
    if not is_git_repo(xpadneo_repo_path):
        error(f"Could not find xpadneo git repository at '{xpadneo_repo_path}'.")

    # Setup output directory
    out_dir = script_dir / "out"
    if out_dir.exists():
        shutil.rmtree(out_dir)
    out_dir.mkdir()

    try:
        run_patch_generation(kernel_repo_path, xpadneo_repo_path, args.kernel_ref, out_dir, script_dir)
        success("All files generated in 'out' directory.")
    except KeyboardInterrupt:
        print("\n\nInterrupted by user.", file=sys.stderr)
        sys.exit(130)
    except Exception as e:
        error(f"An unexpected error occurred: {e}")
        # In case of error, the 'out' dir might be left with partial work.
        # The finally block in run_patch_generation handles worktree cleanup.


if __name__ == "__main__":
    main()
