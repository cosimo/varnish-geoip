#
# varnish-geoip VCL binary tests
# Test helper functions
#
# $Id: test.pm 16778 2010-01-21 16:33:09Z cosimo $

package test;

use strict;
use JSON::XS;
use Test::More;

sub trim {
    $_[0] = (($_[0] =~ /^[ \t]*(.*?)[ \t]*$/s)[0]);
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

    $result = '' if not defined $result;
sub is_country {
    my ($ip, $expected_country, $message) = @_;
    my $geoip_str = run_binary($ip);
    my $country = trim($geoip_str);
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

=head2 C<run_binary($accept_lang_header)>

Runs the C<geoip> binary with an IP address as argument. Returns the output of the binary, which should be a supported language code.

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

