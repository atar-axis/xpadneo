#!/bin/bash

# shellcheck disable=SC1090
source "$(dirname "$0")/lib/installer.sh"

LATEST=$(get_upstream_version_latest)

reinstall() {
    if [[ "${VERSION}" == "${INSTALLED}" ]]; then
        echo "You have already installed ${VERSION}!"
    else
        echo "* uninstalling outdated modules"
        ./uninstall.sh

        echo "* installing latest version"
        ./install.sh
    fi
}

if [[ "${LATEST}" == "${VERSION}" ]]; then
    echo "Looks like the repo is up to date, fine!"
    reinstall
elif __version_lte "${LATEST}" "${VERSION}"; then
    echo "Looks like you are on a developer version, reinstalling!"
    reinstall
else
    echo "Latest stable version: ${LATEST}"
    echo "Installed versions: ${INSTALLED}"
    echo "Repository version: ${VERSION}"

    if [[ "${GIT_ROOT}" != "" ]]; then
        echo "Please update this directory by running 'git reset --hard' and 'git pull', afterwards run this script again"
    else
        echo "Please update this directory by downloading the latest version from https://github.com/atar-axis/xpadneo/archive/master.zip"
    fi
    
    exit -4

fi


