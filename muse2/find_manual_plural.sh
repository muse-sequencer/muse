#!/bin/bash

# this script tries to find all "bad" code like
# (number > 1 ? tr ("tracks") : tr("track"))
# that is, all manual plural diversifications
# you should replace them by:
# tr("processed %n track(s)", "", number)
# the "" is a translator comment. you may write what you want
#
# you have to create appropriate translations for this (even for
# english!). linguist will ask you for the singular, plural,
# and in some language even paucal form then.
#
# this script is not perfect. it misses some "bad" things, and
# finds some "good" things.

{
find . -iname '*.cpp' -print0 | xargs -0 grep -E '[^:]: *tr *\("[^"]*".*\)'
find . -iname '*.cpp' -print0 | xargs -0 grep -E '\? *tr *\("[^"]*".*\)'
} | sort | uniq
