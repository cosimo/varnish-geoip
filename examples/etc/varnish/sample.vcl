include "/etc/varnish/geoip.vcl";

backend default {
    .host = "localhost";
    .port = "8080";
}

sub vcl_recv {

    # Lookup IP only for the first request restart
    if (req.restarts == 0) {
        if (req.request == "GET" || req.request == "POST") {
            C{
                vcl_geoip_set_header(sp);
            }C
        }
    }

}

sub vcl_fetch {

    # If you want to echo X-Geo-IP in the response too
    #set beresp.http.X-Geo-IP = req.http.X-Geo-IP;

}

