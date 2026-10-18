# Module Sining

In order to make the dkms modules work with secure boot they need to be singed.
This directory contains scripts that help you with that task.

## Known Issues / Help Wanted

- doctor script does nothing
- only supports amd64
- only tested on Debian

## Run Doctor

The doctor script will check your environment and asks you to to install
additional packages if something is missing.

## Cert Import

If you alread have a certificate with a key under your control imported into
the bios you can scipt this section.

bla bla

## Signing of VBOX modules

You need to either run the script in the directory that  contains your
`MOK.der` and MOK.priv` or export the path in the `MOK_KEY_DIR` evironment
variable.

Execute the scipt `MOK_KEY_DIR=/path/to/your/key/dir ./sign-xbox-modules` as
root or via sudo to sign the modules.
