**Describe the bug**
A clear and concise description of what the bug is.

**To Reproduce**
Steps to reproduce the behavior:

**Expected behavior**
A clear and concise description of what you expected to happen.

**Screenshots/Gifs**
If applicable, add screenshots or animated gifs to help explain your problem.

**System information**
Please enable debugging output (https://atar-axis.github.io/xpadneo/index.html#debugging)
and add at least the following outputs
- `uname -a`
- `dmesg`

The following command need superuser privileges.
I recommend to run it as root directly (do not use `sudo`),
otherwise the auto-completion using <TAB> will not work as expected.

- `head -1 "/sys/kernel/debug/hid/0005:045E:<TAB>/rdesc" | tee /dev/tty | cksum`

**Additional context**  
Add any other context about the problem here.
