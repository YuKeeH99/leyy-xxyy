#!/usr/bin/env bash
set -e
if [ -z "$1" ]; then
    echo "Usage: $0 <ql-sdk-path>"
    exit 1
fi
SDK="$1"
HERE="$(cd "$(dirname "$0")" && pwd)"
if [ ! -d "$SDK/ql-application/threadx" ]; then
    echo "SDK path not found: $SDK/ql-application/threadx"
    exit 1
fi
mkdir -p "$SDK/ql-application/threadx/venus"
cp -R "$HERE/../venus/." "$SDK/ql-application/threadx/venus/"
cp "$HERE/app_linkscript.ld" "$SDK/ql-application/threadx/config/common/app_linkscript.ld"
echo "Done."
echo "Next: edit $SDK/ql-application/threadx/Makefile and add venus to COMMPILE_DIRS."
