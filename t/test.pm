#
# varnish-geoip VCL binary tests
# Test helper functions
#
# $Id: test.pm 16778 2010-01-21 16:33:09Z cosimo $

package test;

use strict;
use JSON::XS;
use Test::More;

sub update_binary {
    my $status = system('make');
    $status >>= 8;
    return ok(0 == $status, 'C binary updated correctly');
}

sub run_binary {
    my @args = @_;
    my $exec = './geoip';
    my $cmd = $exec;
    $cmd .= ' ';
    $cmd .= join(' ', map { q(') . $_ . q(') } @args);
    my $output = `$cmd`;
    return $output; 
}

sub is_country {
    my ($ip, $expected_country, $message) = @_;
    my $json_str = run_binary($ip);
    my $json_obj = JSON::XS->new();
    #diag('JSON:'.$json_str);
    my $parsed = $json_obj->decode($json_str);
    my $country = $parsed->{'country'};

    $message ||= qq(Lookup of ip '$ip' should correspond to country '$expected_country');

    return is($country, $expected_country, $message);
}

1;

=pod

=head1 NAME

varnish-geoip test functions

=head1 DESCRIPTION

Helper test functions for the Accept-Language varnish C code extension.

=head1 FUNCTIONS

=head2 C<update_binary()>

Runs the C<make> stage and updates the C<geoip> binary for testing.

=head2 C<run_binary($accept_lang_header)>

Runs the C<geoip> binary with the contents of the accept language
header passed as argument. Returns the output of the binary, which should
be a supported language code.

=head2 C<is_country($ip, $country)>

=head2 C<is_country($ip, $country, $message)>

Test assertion.

Looks up the given C<$ip> address and checks that it corresponds
to the given C<$country>

Optionally with a test message (C<$message>).

=head1 AUTHOR

Cosimo Streppone, E<lt>cosimo@opera.comE<gt>

=head1 LICENSE AND COPYRIGHT

Copyright (c), 2010 Opera Software ASA.
All rights reserved.

=end

