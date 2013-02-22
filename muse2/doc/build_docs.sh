#!/bin/bash
#=============================================================================
#  MusE
#  Linux Music Editor
#  Copyright (C) 1999-2013 by Werner Schweer and others
#
#  build_docs.sh Copyright (C) 2013 Tim E. Real <terminator356 at users dot sourceforge dot net>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the
#  Free Software Foundation, Inc.,
#  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
#=============================================================================

#=============================================================
#
# Run this script to re-create the PDF and HTML documentation.
# Usage: build_docs.sh [--help] [--no_post_clean]
#
#=============================================================

if [ "$1" == "--help" ]; then
        echo "Usage: build_docs.sh [--no_post_clean (don't clean up after build)]"
        exit 1
fi

#
# Pre clean
#
rm -rf pdf/*.*
rm -rf html/single/documentation/*.*
rm -rf html/single/developer_docs/*.*
rm -rf html/split/documentation/*.*
rm -rf html/split/developer_docs/*.*
rm -rf tmp

mkdir -p tmp/pdf/documentation
mkdir -p tmp/pdf/developer_docs

mkdir -p tmp/html/single/documentation
mkdir -p tmp/html/single/developer_docs
mkdir -p tmp/html/split/documentation
mkdir -p tmp/html/split/developer_docs

#
# Run the PDF conversions first to get the *.aux files required for proper HTML links displaying
# Run each conversion at least three times to resolve all references
#
# pdflatex -output-directory=pdf documentation.tex
# pdflatex -output-directory=pdf documentation.tex
# pdflatex -output-directory=pdf documentation.tex
# 
# pdflatex -output-directory=pdf developer_docs.tex
# pdflatex -output-directory=pdf developer_docs.tex
# pdflatex -output-directory=pdf developer_docs.tex

pdflatex -interaction=batchmode -halt-on-error -file-line-error -output-directory=tmp/pdf/documentation documentation.tex
pdflatex -interaction=batchmode -halt-on-error -file-line-error -output-directory=tmp/pdf/documentation documentation.tex
pdflatex -interaction=batchmode -halt-on-error -file-line-error -output-directory=tmp/pdf/documentation documentation.tex

pdflatex -interaction=batchmode -halt-on-error -file-line-error -output-directory=tmp/pdf/developer_docs developer_docs.tex
pdflatex -interaction=batchmode -halt-on-error -file-line-error -output-directory=tmp/pdf/developer_docs developer_docs.tex
pdflatex -interaction=batchmode -halt-on-error -file-line-error -output-directory=tmp/pdf/developer_docs developer_docs.tex

#
# Now run the HTML conversions
#
# latex2html -nonavigation -noaddress -noinfo -split 0 -external_file pdf/documentation -dir html/single/documentation documentation.tex
# latex2html -nonavigation -noaddress -noinfo -split 0 -external_file pdf/developer_docs -dir html/single/developer_docs developer_docs.tex
# latex2html -noaddress -noinfo -split 4 -external_file pdf/documentation -dir html/split/documentation documentation.tex
# latex2html -noaddress -noinfo -split 4 -external_file pdf/developer_docs -dir html/split/developer_docs developer_docs.tex

latex2html -nonavigation -noaddress -noinfo -split 0 -external_file tmp/pdf/documentation/documentation -mkdir -dir tmp/html/single/documentation documentation.tex
latex2html -nonavigation -noaddress -noinfo -split 0 -external_file tmp/pdf/developer_docs/developer_docs -mkdir -dir tmp/html/single/developer_docs developer_docs.tex

latex2html -noaddress -noinfo -split 4 -external_file tmp/pdf/documentation/documentation -mkdir -dir tmp/html/split/documentation documentation.tex
latex2html -noaddress -noinfo -split 4 -external_file tmp/pdf/developer_docs/developer_docs -mkdir -dir tmp/html/split/developer_docs developer_docs.tex

#
# Move the files
#
mv -f tmp/pdf/documentation/*.pdf pdf
mv -f tmp/pdf/developer_docs/*.pdf pdf

mv -f tmp/html/single/documentation/*.css   html/single/documentation
mv -f tmp/html/single/documentation/*.html  html/single/documentation
# mv -f tmp/html/single/documentation/*.jpg   html/single/documentation
mv -f tmp/html/single/documentation/*.png   html/single/documentation
# mv -f tmp/html/single/documentation/toc_.txt html/single/documentation

mv -f tmp/html/single/developer_docs/*.css   html/single/developer_docs
mv -f tmp/html/single/developer_docs/*.html  html/single/developer_docs
# mv -f tmp/html/single/developer_docs/*.jpg   html/single/developer_docs
mv -f tmp/html/single/developer_docs/*.png   html/single/developer_docs
# mv -f tmp/html/single/developer_docs/toc_.txt html/single/developer_docs

mv -f tmp/html/split/documentation/*.css    html/split/documentation
mv -f tmp/html/split/documentation/*.html   html/split/documentation
# mv -f tmp/html/split/documentation/*.jpg    html/split/documentation
mv -f tmp/html/split/documentation/*.png    html/split/documentation
# mv -f tmp/html/split/documentation/toc_.txt html/split/documentation

mv -f tmp/html/split/developer_docs/*.css    html/split/developer_docs
mv -f tmp/html/split/developer_docs/*.html   html/split/developer_docs
# mv -f tmp/html/split/developer_docs/*.jpg    html/split/developer_docs
mv -f tmp/html/split/developer_docs/*.png    html/split/developer_docs
# mv -f tmp/html/split/developer_docs/toc_.txt html/split/developer_docs

#
# Post clean
#
if [ "$1" != "--no_post_clean" ]; then
# rm -f pdf/*.aux pdf/*.log pdf/*.out pdf/*.toc
# rm -f html/single/documentation/*.aux html/single/documentation/*.log html/single/documentation/*.out html/single/documentation/*.pl html/single/documentation/*.tex html/single/documentation/WARNINGS
# rm -f html/single/developer_docs/*.aux html/single/developer_docs/*.log html/single/developer_docs/*.out html/single/developer_docs/*.pl html/single/developer_docs/*.tex html/single/developer_docs/WARNINGS
# rm -f html/split/documentation/*.aux html/split/documentation/*.log html/split/documentation/*.out html/split/documentation/*.pl html/split/documentation/*.tex html/split/documentation/WARNINGS
# rm -f html/split/developer_docs/*.aux html/split/developer_docs/*.log html/split/developer_docs/*.out html/split/developer_docs/*.pl html/split/developer_docs/*.tex html/split/developer_docs/WARNINGS
rm -rf tmp
fi
