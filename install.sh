VERSION="0.2.0"

sudo ln --symbolic --force $PWD/hid-xpadneo-$VERSION/ /usr/src/
sudo dkms remove hid-xpadneo -v $VERSION --all
sudo dkms add hid-xpadneo -v $VERSION
sudo dkms install hid-xpadneo -v $VERSION
