#!/usr/bin/perl -w

use warnings;
use strict;
use File::Basename;

sub get_patch
{
	my $path = shift;
	my $package = basename(dirname($path));

	open PATCH, "grep \"patch \" $path |" or die "Unable to get list of patches!";
	while (<PATCH>) {
		chomp;
		my @line = split /\s+/, $_, 3;
		my $patchname = basename($line[1]);
		print("$package: $patchname\n");
	}
	close PATCH;
}

sub get_log
{
	if( -d "./logs") {
		open PREPARE, "grep -rl  \"Applying \" ./logs/ | grep prepare.txt |" or die "Unable to get list of logs!";
		while (<PREPARE>) {
			chomp;
			get_patch("$_");
		}
		close PREPARE;
	}
}

get_log();
