#!/bin/bash

# shellcheck disable=SC2046
OPTIONS=$(getopt --name "$0" --longoptions "verbose" -- "" "$@") || exit 1
# shellcheck disable=SC2086
eval set -- $OPTIONS

while true; do
    case "$1" in
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
            echo "usage: $0 [--verbose]"
            exit 1
    esac
    shift
done

[ ${#V[*]} -gt 0 ] && set -x
