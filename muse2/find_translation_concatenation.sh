#!/bin/bash

# this script tries to find all "bad" code like
# tr("the file ") + your_file + tr(" could not be loaded!")
# you should replace it by:
# tr("the file %1 could not be loaded!").arg(your_file)
#
# this script is not perfect. it misses some "bad" things, and
# finds some "good" things.

{
find . -iname '*.cpp' -print0 | xargs -0 grep -E 'tr *\("[^"]*" *\) *\+';
find . -iname '*.cpp' -print0 | xargs -0 grep -E '\+ *tr *\("[^"]*" *\)';
} | sort | uniq


