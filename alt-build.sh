#!/bin/bash

mkdir -p build/dbus-fronius
cd build/dbus-fronius
qmake ../../software/dbus-fronius.pro && make -j$(nproc)
if [[ $? -ne 0 ]] ; then
    exit 1
fi
