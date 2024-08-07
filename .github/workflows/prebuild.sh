#!/bin/bash
set -x -e 

mkdir -p be/install && cd be
cmake \
    -DFETCH_DEPENDENCIES=ON \
    -DDEPENDENCIES_ONLY=ON \
    -DCMAKE_INSTALL_PREFIX=./install \
    ..

make && make install