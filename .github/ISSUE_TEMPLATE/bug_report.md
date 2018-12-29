---
name: Bug report
about: Create a report to help us improve

---

**Describe the bug**
A clear and concise description of what the bug is.

**To Reproduce**
Steps to reproduce the behavior:
1. Go to '...'
2. Click on '....'
3. Scroll down to '....'
4. See error

**Expected behavior**
A clear and concise description of what you expected to happen.

**Screenshots**
If applicable, add screenshots to help explain your problem.

**System information**
- `uname -a` output
- `dmesg` output
  Please enable debugging output before: https://atar-axis.github.io/xpadneo/index.html#debugging  
  
The following command need superuser privileges.
I recommend to run it as root directly (not using `sudo`), since this way the <TAB>-key will work as expected.
- `cat /sys/kernel/debug/hid/0005:045E:<TAB>/rdesc && sudo cksum $_`


**Additional context**  
Add any other context about the problem here.
