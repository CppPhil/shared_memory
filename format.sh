#!/bin/bash

### This script is used to run clang-format on the source files.

# Directory containing this bash script.
readonly DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

readonly PREV_DIR="$(pwd)"

readonly DOT_CLANG_FORMAT="$DIR/.clang-format"

delete_file_if_exists () {
  if [ -e "$1" ]; then
    rm -f "$1"
  fi
}

format () {
  cd "$1"
  delete_file_if_exists ./.clang-format
  cp "$DOT_CLANG_FORMAT" ./.clang-format
  find . -name '*.cpp' -o -name '*.hh' -o -name '*.hpp' -o -name '*.h' | xargs clang-format -i
  rm -f ./.clang-format
}

cd "$DIR"

# Format the source files.
format "$DIR/client"
format "$DIR/lib"
format "$DIR/server"

cd "$PREV_DIR"

exit 0
