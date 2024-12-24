## Packaging

From v0.10 on, xpadneo provides a `Makefile` to help package maintainers
avoiding a lot of the duplicate work which was historically done by the
installer script. This also allows for prefixed installation to prepare an
install image before merging the package to the live system.

Run `make help` for an overview of the options.


### Configuration

The top-level `Makefile` supports some configuration variables:

- `PREFIX`: Allows to install the package to a pre-install image which
  can be put in an archive for package deployment and file tracking. If
  this variable is provided, it will also skip deployment of the DKMS
  source tree. This is mainly due to limitations of DKMS handling
  prefixed installations properly. Package maintainers have to do this
  manually in a post-installation step.

- `ETC_PREFIX`: Allows to install files to `/usr/lib` which would
  normally go to `/etc`. This is applied relative to `PREFIX`.

**Example:**
```bash
make PREFIX=/tmp/xpadneo-image ETC_PREFIX=/usr/lib install
```


### Installation

When using the `Makefile`, xpadneo will no longer automatically try to build
the DKMS module. How to handle this in detail is left up to the package
maintainer. If you run `make install` without a `PREFIX` or use an empty
prefix, `make install` will run `dkms add` to add the source code to the
DKMS source tree. No other DKMS action will be taken.

**Examples:**
```bash
# Install package files to `/` and deploy the DKMS sources
make install

# Install package files but deploy the DKMS sources as a separate step
make PREFIX=/ install
dkms add hid-xpadneo
```

Instead of directly building the DKMS module, this allows a package installer
to just provide the source code and install the support files. Building of
the actual module via DKMS is left to the package maintainer.

Many distributions recommend to install base configuration to `/usr/lib`
instead of `/etc`. The make-based installer supports this by supplying a
`ETC_PREFIX` variable.

**Example:**
```bash
# Install package files to `/usr/lib` instead of `/etc`
make ETC_PREFIX=/usr/lib install

# This can be combined with prefixed installation
make PREFIX=/tmp/xpadneo-image ETC_PREFIX=/usr/lib install
dkms add hid-xpadneo
```

In all these usages, xpadneo will never try to build and install the kernel
module on its own. This step is left up to the package maintainer.

**Background:** DKMS does not support prefixed installations. If you try to
do so, symlinks will point to the wrong location. DKMS also doesn't support
proper uninstallation: It will remove the kernel module from its build system
but it won't touch the files it copied to `/usr/src` during `dkms add`.
Package maintainers are required to properly handle this according to their
specific distribution. xpadneo won't try to mess with this to prevent errors
in the boot system of the distribution. It cannot know the implementation
details.

`make PREFIX=...` will warn about this but still show the command which would
be run:
```
# make PREFIX=...
Makefile:7: Installing to prefix, dkms commands will not be run!
...
: SKIPPING dkms add hid-xpadneo
```

Documentation will be installed to `DOC_PREFIX` and follows the same rules as
above. Default location is `/usr/share/doc/xpadneo`. Some distributions may
want to override this with a versioned path.


### Uninstallation

Package maintainers usually don't need to care about uninstallation because
the packaging system tracks installed files and also properly removes them.
Still, xpadneo provides `make uninstall` which runs the following steps for
that **exact** xpadneo version only:

1. Removes xpadneo from *all* kernel instances of DKMS
2. Removes xpadneo kernel sources from `/usr/src`
3. Removes installed udev rules and module configurations **unconditionally**

Step 3 means that it will remove files which may be shared with an already
installed xpadneo package. **Be careful!**

If using `PREFIX` or `ETC_PREFIX`, the same rules as above apply: If a
`PREFIX` is provided, `dkms remove` won't be run.

This is not meant to be used by package maintainers as part of handling the
package installation. It is a development tool.


### Summary

Package maintainers are advised to migrate to the new installation method.
Please provide feedback if you see problems or missing features.

Package maintainers **SHOULD NOT** use `install.sh` or its sibling scripts
to maintain the installation. But you may use the contents of it as a guide
for your package installation script.
