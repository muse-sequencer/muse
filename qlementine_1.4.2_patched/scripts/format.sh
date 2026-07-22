#!/bin/bash

find $(dirname "$0")/../lib/src $(dirname "$0")/../lib/include  $(dirname "$0")/../sandbox/src  $(dirname "$0")/../showcase/src -iname *.hpp -o -iname *.cpp | xargs clang-format -i
