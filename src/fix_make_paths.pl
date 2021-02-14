#!/usr/bin/perl --

use strict;

my @files = split(/\n/, `find . -name build.make -print`);

foreach my $file(@files) {
  open(my $fh, "< :encoding(UTF-8)", $file) || die "ERROR: Cannot open $file for reading: $!";
  my $content = do { local $/; <$fh> };
  close($fh) || die "ERROR: Cannot close $file opened for reading: $!";
  $content =~ s/\/d //g;
  $content =~ s/\\/\//g;
  $content =~ s/\/([\r\n])/\\$1/g;
  open(my $fh, "> :encoding(UTF-8)", $file) || die "ERROR: Cannot open $file for writing: $!";
  print $fh $content;
  close($fh) || die "ERROR: Cannot close $file opened for writing: $!";
}

my @files = split(/\n/, `find . -name link.txt -print`);

foreach my $file(@files) {
  open(my $fh, "< :encoding(UTF-8)", $file) || die "ERROR: Cannot open $file for reading: $!";
  my $content = do { local $/; <$fh> };
  close($fh) || die "ERROR: Cannot close $file opened for reading: $!";
  $content =~ s/\\/\//g;
  $content =~ s/\/([\r\n])/\\$1/g;
  open(my $fh, "> :encoding(UTF-8)", $file) || die "ERROR: Cannot open $file for writing: $!";
  print $fh $content;
  close($fh) || die "ERROR: Cannot close $file opened for writing: $!";
}

my @files = split(/\n/, `find . -name Makefile -print`);

foreach my $file(@files) {
  open(my $fh, "< :encoding(UTF-8)", $file) || die "ERROR: Cannot open $file for reading: $!";
  my $content = do { local $/; <$fh> };
  close($fh) || die "ERROR: Cannot close $file opened for reading: $!";
  $content =~ s/\\/\//g;
  $content =~ s/\/([\r\n])/\\$1/g;
  open(my $fh, "> :encoding(UTF-8)", $file) || die "ERROR: Cannot open $file for writing: $!";
  print $fh $content;
  close($fh) || die "ERROR: Cannot close $file opened for writing: $!";
}

