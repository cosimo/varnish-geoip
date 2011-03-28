#!/usr/bin/env perl

# 
# Basic varnish-geoip tests
#

use strict;
require test;

my @ip = (
    [ '213.236.208.22' => 'NO' ],
    [ '85.34.205.51'   => 'IT' ],
);

Test::More::plan(tests => (1 * @ip));

# Basically test for supported languages
for (@ip) {
    my ($ip, $country) = @{ $_ };
    test::is_country($ip, $country);
}

