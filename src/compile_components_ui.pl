#!/usr/bin/perl --

use strict;

my $file = 'muse/components/CMakeFiles/components.dir/build.make';

# Extract commands
open(my $fh, "< :encoding(UTF-8)", $file) || die "ERROR: Cannot open $file for reading: $!";
my $content = do { local $/; <$fh> };
close($fh) || die "ERROR: Cannot close $file opened for reading: $!";
my @commands;
while ($content =~ m/(uic.exe .*\.ui)/g) {
	my @line = split(/\s/, $1);
	$line[2] =~ s/(.*)muse\/components\/(.*)\.h/muse\/components\/$2\.h/;
	$line[3] =~ s/(.*)muse\/components\/(.*)\.ui/\.\.\/muse\/components\/$2\.ui/;
	push(@commands, join(' ', @line));
}

# Execute commands
foreach my $cmd(@commands) {
	system($cmd);
}

