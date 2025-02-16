#!/usr/bin/env bash

set -euo pipefail

print_configs() {
  printf 'valid config values are:\n'
  printf '\tDebug\n'
  printf '\tRelease\n'
  printf '\tRelWithDebInfo\n'
}

if [ $# -ne 1 ]; then
  printf 'script requires a single argument: the config to use\n'
  print_configs
  exit 1
fi

case $1 in
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
cmake --build build --config "$1"

"./build/src/$1/vic" 2> >(while read -r line; do printf '\e[01;31m%s\e[0m\n' "$line" >&2; done)
