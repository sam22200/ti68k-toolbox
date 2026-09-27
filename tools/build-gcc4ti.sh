#!/bin/sh
# Rebuilds GCC4TI in a Debian Jessie container (GCC 4.1.2 does not build with a modern gcc).
# Installed into $T/gcc4ti-bin (same path inside the container and on the host).
set -e
docker build -q -t gcc4ti-build -f "$(dirname "$0")/Dockerfile.gcc4ti" "$(dirname "$0")" >/dev/null
T=$(cd "$(dirname "$0")" && pwd)
docker run --rm -u "$(id -u):$(id -g)" -e HOME=/tmp -v "$T:$T" -w "$T" gcc4ti-build sh -c "
  set -e
  cd $T/gcc4ti/trunk/tigcc-linux/scripts && ./updatesrc >/dev/null
  cd ../gcc4ti-0.96b11 && cp $T/tarballs/*.tar.bz2 .
  rm -rf $T/gcc4ti-bin
  cd scripts && PREFIX_GCC4TI=$T/gcc4ti-bin CC=gcc CFLAGS='-Os -s -fno-exceptions -fomit-frame-pointer' ./Install_All
"
