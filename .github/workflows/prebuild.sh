#!/bin/bash
set -x -e 

mkdir -p buildext/install && cd buildext
cmake \
    -DFETCH_DEPENDENCIES=ON \
    -DDEPENDENCIES_ONLY=ON \
    -DCMAKE_INSTALL_PREFIX=./install \
    ..

make && make install