#!/usr/bin/perl

# Ce script s’attend à ce que le dossier t/ contienne des photos intitulées
# {DATE}Y{TOTAL}+….jpg, par exemple 2024-02-09Y13841+2024-02-09Y1144.jpg. Il
# doit être exécuté depuis la racine du projet.

my @wanted;
for my $name (<t/*.jpg>) {
	$name =~ s/\..*$//;
	for (split /\+/, $name) {
		/^(\d+-\d+-\d+)Y(\d+)$/ or next;
		push @wanted, "$1\t$2";
	}
}

open my $scanner, 'python -m kakeibo.receipt --format tsv t/*.jpg |';
my @output = <$scanner>;
chomp @output;
my %got = map { $_ => 1 } @output;
close $scanner;

use Test::More;
ok $got{$_}, $_ for @wanted;
done_testing;
