include "/etc/varnish/geoip.vcl";

backend default {
    .host = "localhost";
    .port = "8080";
}

sub vcl_recv {
    C{
        vcl_geoip_set_header(sp);
    }C
}

sub vcl_fetch {
    set obj.http.X-Geo-IP = req.http.X-Geo-IP;
}

