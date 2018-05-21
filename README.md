This is the DKMS version of my hid-xpadneo driver. Take a look in the master branch for further information.

## Prerequisites
To install it, make sure you have installed
* dkms
* linux-headers

## Installation
* Download the Repository to your local machine
* Copy the `hid-xpadneo-<version>` folder into the `/usr/src/` directory
* Run the following commands
  * sudo dkms add hid-xpadneo -v <version>
  * sudo dkms install hid-xpadneo -v <version>
