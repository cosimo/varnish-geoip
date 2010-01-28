#!/usr/bin/env perl

# 
# Basic varnish-geoip tests
#

use strict;
require test;

my @ip = (
    [ '213.236.208.22' => 'NO' ],
);

Test::More::plan(tests => 1 + @ip);

test::update_binary();

# Basically test for supported languages
for (@ip) {
    my ($ip, $country) = @{ $_ };
    test::is_country($ip, $country);
}

