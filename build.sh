#!/bin/bash
cmake -S . -B build -DASMJIT_STATIC=ON -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY=build/lib -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=.
make -C build
mv build/wind ./wind