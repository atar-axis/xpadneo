#!/bin/bash

# shellcheck disable=SC2046
OPTIONS=$(getopt --name "$0" --longoptions "force,kernelsourcedir:,verbose" -- "" "$@") || exit 1
# shellcheck disable=SC2086
eval set -- $OPTIONS

while true; do
    case "$1" in
        --force)
            # shellcheck disable=SC2034
            FORCE="--force"
            ;;
        --kernelsourcedir)
            shift
            # shellcheck disable=SC2034
            KERNEL_SOURCE_DIR="--kernelsourcedir=$1"
            ;;
        --verbose)
            echo "* verbose mode enabled"
            # shellcheck disable=SC2034
            V=("$1")
            ;;
        --)
            shift
            break
            ;;
        *)
            echo "usage: $0 [--kernelsourcedir] [--verbose]"
            exit 1
    esac
    shift
done

# shellcheck disable=SC2034
[ ${#V[*]} -gt 0 ] || MAKE_OPTS=("-s")
[ ${#V[*]} -eq 0 ] || set -x
