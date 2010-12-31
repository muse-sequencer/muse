#!/bin/sh

#test $# != 1 && echo "Usage: $0 <rgd-file>" && exit 1
mkdir -p MUSE_MAPS
for f in *.rgd
do
	gunzip -c $f | xsltproc $(dirname $0)/rgd2muse.xsl - > "MUSE_MAPS/$(basename $f .rgd).idf"
done

