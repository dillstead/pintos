# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected (IGNORE_EXIT_CODES => 1, [<<'EOF']);
(dir-rm-vine) begin
(dir-rm-vine) mkdir "0"
(dir-rm-vine) chdir "0"
(dir-rm-vine) mkdir "1"
(dir-rm-vine) chdir "1"
(dir-rm-vine) mkdir "2"
(dir-rm-vine) chdir "2"
(dir-rm-vine) mkdir "3"
(dir-rm-vine) chdir "3"
(dir-rm-vine) mkdir "4"
(dir-rm-vine) chdir "4"
(dir-rm-vine) mkdir "5"
(dir-rm-vine) chdir "5"
(dir-rm-vine) mkdir "6"
(dir-rm-vine) chdir "6"
(dir-rm-vine) mkdir "7"
(dir-rm-vine) chdir "7"
(dir-rm-vine) mkdir "8"
(dir-rm-vine) chdir "8"
(dir-rm-vine) mkdir "9"
(dir-rm-vine) chdir "9"
(dir-rm-vine) create "test"
(dir-rm-vine) chdir "/"
(dir-rm-vine) open "/0/1/2/3/4/5/6/7/8/9/test"
(dir-rm-vine) close "/0/1/2/3/4/5/6/7/8/9/test"
(dir-rm-vine) remove "/0/1/2/3/4/5/6/7/8/9/test"
(dir-rm-vine) remove "/0/1/2/3/4/5/6/7/8/9"
(dir-rm-vine) remove "/0/1/2/3/4/5/6/7/8"
(dir-rm-vine) remove "/0/1/2/3/4/5/6/7"
(dir-rm-vine) remove "/0/1/2/3/4/5/6"
(dir-rm-vine) remove "/0/1/2/3/4/5"
(dir-rm-vine) remove "/0/1/2/3/4"
(dir-rm-vine) remove "/0/1/2/3"
(dir-rm-vine) remove "/0/1/2"
(dir-rm-vine) remove "/0/1"
(dir-rm-vine) remove "/0"
(dir-rm-vine) open "/0/1/2/3/4/5/6/7/8/9/test" (must return -1)
(dir-rm-vine) end
EOF
