# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected (IGNORE_EXIT_CODES => 1, [<<'EOF']);
(dir-vine) begin
(dir-vine) creating many levels of files and directories...
(dir-vine) removing all but top 10 levels of files and directories...
(dir-vine) end
EOF

# The archive should look like this:
#
# -rw-r--r-- 0/0           40642 2006-01-01 00:00:00 dir-vine
# -rw-r--r-- 0/0           42479 2006-01-01 00:00:00 tar
# drwxr-xr-x 0/0               0 2006-01-01 00:00:00 start
# -rw-r--r-- 0/0              11 2006-01-01 00:00:00 start/file0
# drwxr-xr-x 0/0               0 2006-01-01 00:00:00 start/dir0
# -rw-r--r-- 0/0              11 2006-01-01 00:00:00 start/dir0/file1
# drwxr-xr-x 0/0               0 2006-01-01 00:00:00 start/dir0/dir1
# -rw-r--r-- 0/0              11 2006-01-01 00:00:00 start/dir0/dir1/file2
# drwxr-xr-x 0/0               0 2006-01-01 00:00:00 start/dir0/dir1/dir2
# -rw-r--r-- 0/0              11 2006-01-01 00:00:00 start/dir0/dir1/dir2/file3
# drwxr-xr-x 0/0               0 2006-01-01 00:00:00 start/dir0/dir1/dir2/dir3
# -rw-r--r-- 0/0              11 2006-01-01 00:00:00 start/dir0/dir1/dir2/dir3/file4
# drwxr-xr-x 0/0               0 2006-01-01 00:00:00 start/dir0/dir1/dir2/dir3/dir4
# -rw-r--r-- 0/0              11 2006-01-01 00:00:00 start/dir0/dir1/dir2/dir3/dir4/file5
# drwxr-xr-x 0/0               0 2006-01-01 00:00:00 start/dir0/dir1/dir2/dir3/dir4/dir5
# -rw-r--r-- 0/0              11 2006-01-01 00:00:00 start/dir0/dir1/dir2/dir3/dir4/dir5/file6
# drwxr-xr-x 0/0               0 2006-01-01 00:00:00 start/dir0/dir1/dir2/dir3/dir4/dir5/dir6
# -rw-r--r-- 0/0              11 2006-01-01 00:00:00 start/dir0/dir1/dir2/dir3/dir4/dir5/dir6/file7
# drwxr-xr-x 0/0               0 2006-01-01 00:00:00 start/dir0/dir1/dir2/dir3/dir4/dir5/dir6/dir7
# -rw-r--r-- 0/0              11 2006-01-01 00:00:00 start/dir0/dir1/dir2/dir3/dir4/dir5/dir6/dir7/file8
# drwxr-xr-x 0/0               0 2006-01-01 00:00:00 start/dir0/dir1/dir2/dir3/dir4/dir5/dir6/dir7/dir8
# -rw-r--r-- 0/0              11 2006-01-01 00:00:00 start/dir0/dir1/dir2/dir3/dir4/dir5/dir6/dir7/dir8/file9
# drwxr-xr-x 0/0               0 2006-01-01 00:00:00 start/dir0/dir1/dir2/dir3/dir4/dir5/dir6/dir7/dir8/dir9
my ($dir) = {};
my ($root) = {"start" => $dir};
for (my ($i) = 0; $i < 10; $i++) {
    $dir->{"file$i"} = ["contents $i\n"];
    $dir = $dir->{"dir$i"} = {};
}
check_archive ($root);
pass;
