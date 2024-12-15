#!/bin/bash

wind_exec=benchmarks/testWIND
cpp_exec=benchmarks/testCPP

compile_files() {
  echo "Compiling testWIND..."
  echo "Flags: None"
  ./wind benchmarks/test.w -o "$wind_exec"

  echo "Compiling testCPP..."
  echo "Flags: -O3 -std=c++17"
  g++ -O3 -std=c++17 -o "$cpp_exec" benchmarks/test.cpp
}

run_benchmark() {
  executable=$1

  echo "---------------------------------------------------------"
  echo "Flushing caches for $executable..."
  sudo sh -c "sync; echo 3 > /proc/sys/vm/drop_caches"

  echo "Benchmarking $executable..."
  time $executable > /dev/null
}


compile_files

printf "\n\nBenchmarking testWIND...\n"
run_benchmark "$wind_exec"

printf "\nFlushing caches before testCPP..."
sudo sh -c "sync; echo 3 > /proc/sys/vm/drop_caches"

printf "\n\n\nBenchmarking testCPP...\n"
run_benchmark "$cpp_exec"
