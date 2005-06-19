# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected (IGNORE_EXIT_CODES => 1, [<<'EOF']);
(grow-too-big) begin
(grow-too-big) create "fumble"
(grow-too-big) open "fumble"
(grow-too-big) seek "fumble"
(grow-too-big) write "fumble"
(grow-too-big) close "fumble"
(grow-too-big) end
EOF
