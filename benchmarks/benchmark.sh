#!/bin/bash

wind_exec=benchmarks/testWIND.out
cpp_exec=benchmarks/testCPP.out
c_exec=benchmarks/testC.out
rs_exec=benchmarks/testRS.out

wind_flags=""
cpp_flags="-O3 -std=c++17"
c_flags="-O3"
rs_flags=""

compile_files() {
  echo "                 == [ Compiling sources... ] =="
  echo "----------------------------------------------------------------"
  echo "Compiling testWIND..."
  echo "Flags: $wind_flags"
  ./wind benchmarks/test.w -o "$wind_exec"

  echo -e "\nCompiling testCPP..."
  echo "Flags: $cpp_flags"
  g++ $cpp_flags -o "$cpp_exec" benchmarks/test.cpp

  echo -e "\nCompiling testC..."
  echo "Flags: $c_flags"
  gcc $c_flags -o "$c_exec" benchmarks/test.c

  echo -e "\nCompiling testRS..."
  echo "Flags: $rs_flags"
  rustc $rs_flags -o "$rs_exec" benchmarks/test.rs

  sudo sh -c "sync; echo 3 > /proc/sys/vm/drop_caches"

}

declare -A run_queue=(
  [$wind_exec]="WIND"
  [$cpp_exec]="C++"
  [$rs_exec]="Rust"
  [$c_exec]="C"
)
name_array=(
  "C++"
  "Rust"
  "WIND"
  "C"
)

results_array=()
run_benchmark() {
  executable=$1

  echo "----------------------------------------------------------------"
  echo "Flushing caches for $executable..."
  sudo sh -c "sync; echo 3 > /proc/sys/vm/drop_caches"

  echo "Benchmarking $executable..."
  output=$(perf stat -e duration_time,task-clock,cpu-clock benchmarks/testCPP.out 2>&1)
  echo "$output"
  results_array+=($(echo "$output" | grep -oP '\d+\.\d+(?=\s+msec)'))
}

bench_run() {
  executable=$1
  label=$2
  echo "                  == [ Benchmarking $label... ] =="
  run_benchmark "$executable"
  echo -e "----------------------------------------------------------------\n"
}

summary () {
  echo "                  == [ Benchmarking summary... ] =="
  echo "----------------------------------------------------------------"
  echo "Results:"

  for i in "${!name_array[@]}"; do
    key="test${name_array[$i]}"
    echo "${name_array[$i]}: ${results_array[$i]} msec"
  done

  echo -e "----------------------------------------------------------------\n"
}

main() {
  compile_files
  echo -e "\n\n"

  for i in "${!run_queue[@]}"; do
    bench_run "$i" "${run_queue[$i]}"
  done

  summary

  echo -e "               == [ Benchmarking complete! ] =="
  echo -e "  Keep in mind that these results aren't absolutely accurate."
  echo -e "  The results may vary depending on system and environment."
  echo -e "  Results may vary also from run to run."
  echo -e "  Either average them out, or run heavy benchmarks."
  echo -e "----------------------------------------------------------------"
}

main