#!/bin/bash

# shellcheck disable=SC1090
source "$(dirname "$0")/lib/installer.sh"

LATEST=$(get_upstream_version_latest)


if [[ $LATEST == "$VERSION" ]]; then

    echo "Looks like the repo is up to date, fine!"
    
    if [[ $LATEST == "$INSTALLED" ]]; then
        echo "You have already installed the latest version! Yay."
    else
        echo "* uninstalling outdated modules"
        ./uninstall.sh
            
        echo "* installing latest version"
        ./install.sh
    fi
    
else
    
    if [[ "${GIT_ROOT}" != "" ]]; then
        echo "Please update this directory by running 'git reset --hard' and 'git pull', afterwards run this script again"
    else
        echo "Please update this directory by downloading the latest version from https://github.com/atar-axis/xpadneo/archive/master.zip"
    fi
    
    exit -4

fi


