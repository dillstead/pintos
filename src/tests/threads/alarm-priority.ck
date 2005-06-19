# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(alarm-priority) begin
(alarm-priority) Thread priority 32 woke up.
(alarm-priority) Thread priority 33 woke up.
(alarm-priority) Thread priority 34 woke up.
(alarm-priority) Thread priority 35 woke up.
(alarm-priority) Thread priority 36 woke up.
(alarm-priority) Thread priority 37 woke up.
(alarm-priority) Thread priority 38 woke up.
(alarm-priority) Thread priority 39 woke up.
(alarm-priority) Thread priority 40 woke up.
(alarm-priority) Thread priority 41 woke up.
(alarm-priority) end
EOF
