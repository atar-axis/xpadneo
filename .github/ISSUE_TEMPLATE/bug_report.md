---
name: Bug report
about: Report a bug

---

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
- `sudo find "/sys/kernel/debug/hid/" -name "0005:045E:*" -exec sh -c 'echo "{}" && head -1 "{}/rdesc" | tee /dev/tty | cksum && echo' \;`

**Additional context**  
Add any other context about the problem here.
