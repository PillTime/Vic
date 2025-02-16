#!/usr/bin/env bash

set -euo pipefail

print_modes() {
  printf 'valid mode values are:\n'
  printf '\tBuild\n'
  printf '\tRun\n'
}

print_configs() {
  printf 'valid config values are:\n'
  printf '\tDebug\n'
  printf '\tRelease\n'
  printf '\tRelWithDebInfo\n'
}

if [ $# -ne 2 ]; then
  printf 'error: invalid number of arguments.\nexpected: %s <MODE> <CONFIG>\n' "$0"
  print_modes
  print_configs
  exit 1
fi

case $1 in
  'Build' | 'Run')
  ;;
  *)
    printf 'mode provided is invalid\n'
    print_modes
    exit 1
  ;;
esac

case $2 in
  'Debug' | 'Release' | 'RelWithDebInfo')
  ;;
  *)
    printf 'config provided is invalid\n'
    print_configs
    exit 1
  ;;
esac

export CC=/usr/bin/clang
export CXX=/usr/bin/clang++

cmake -S . -B build -G 'Ninja Multi-Config'
cmake --build build --config "$2"

if [ "$1" = 'Run' ]; then
  "./build/src/$2/vic" 2> >(while read -r line; do printf '\e[01;31m%s\e[0m\n' "$line" >&2; done)
fi
