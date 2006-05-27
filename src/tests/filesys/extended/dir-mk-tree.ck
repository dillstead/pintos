# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected (IGNORE_EXIT_CODES => 1, [<<'EOF']);
(dir-mk-tree) begin
(dir-mk-tree) creating /0/0/0/0 through /3/2/2/3...
(dir-mk-tree) open "/0/2/0/3"
(dir-mk-tree) close "/0/2/0/3"
(dir-mk-tree) end
EOF
my ($tree);
for my $a (0...3) {
    for my $b (0...2) {
	for my $c (0...2) {
	    for my $d (0...3) {
		$tree->{$a}{$b}{$c}{$d} = [''];
	    }
	}
    }
}
check_archive ($tree);
pass;
