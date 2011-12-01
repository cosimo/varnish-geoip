/*
 * Varnish-powered Geo IP lookup
 *
 * Idea and GeoIP code taken from
 * http://svn.wikia-code.com/utils/varnishhtcpd/wikia.vcl
 *
 * Cosimo, 01/12/2011
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <GeoIPCity.h>
#include <pthread.h>

#define vcl_string char
#define HEADER_MAXLEN 255

pthread_mutex_t geoip_mutex = PTHREAD_MUTEX_INITIALIZER;
GeoIP* gi;
GeoIPRecord *record;

/* Init GeoIP code */
void geoip_init () {
    if (!gi) {
        gi = GeoIP_open_type(GEOIP_CITY_EDITION_REV1,GEOIP_MEMORY_CACHE);
        assert(gi);
    }
}

static inline int geoip_lookup(vcl_string *ip, vcl_string *resolved) {
    int lookup_success = 0;

    pthread_mutex_lock(&geoip_mutex);

    if (!gi) {
        geoip_init();
    }

    record = GeoIP_record_by_addr(gi, ip);

    /* Lookup succeeded */
    if (record) {
        lookup_success = 1;
        snprintf(resolved, HEADER_MAXLEN, "city:%s, country:%s, lat:%f, lon:%f, ip:%s",
            record->city,
            record->country_code,
            record->latitude,
            record->longitude,
            ip
        );
    }

    /* Failed lookup */
    else {
        strncpy(resolved, "", HEADER_MAXLEN);
    }

    pthread_mutex_unlock(&geoip_mutex);

    return lookup_success;
}

#ifdef __VCL__
/* Returns the GeoIP info as synthetic response */
void vcl_geoip_send_synthetic(const struct sess *sp) {
    vcl_string hval[HEADER_MAXLEN];
    vcl_string *ip = VRT_IP_string(sp, VRT_r_client_ip(sp));
    if (geoip_lookup(ip, hval)) {
        VRT_synth_page(sp, 0, hval, vrt_magic_string_end);
    }
    else {
        VRT_synth_page(sp, 0, "", vrt_magic_string_end);
    }
}

/* Sets "X-Geo-IP" header with the geoip resolved information */
void vcl_geoip_set_header(const struct sess *sp) {
    vcl_string hval[HEADER_MAXLEN];
    vcl_string *ip = VRT_IP_string(sp, VRT_r_client_ip(sp));
    if (geoip_lookup(ip, hval)) {
        VRT_SetHdr(sp, HDR_REQ, "\011X-Geo-IP:", hval, vrt_magic_string_end);
    }
    else {
        /* Send an empty header */
        VRT_SetHdr(sp, HDR_REQ, "\011X-Geo-IP:", "", vrt_magic_string_end);
    }
}
#else
int main(int argc, char **argv) {
    vcl_string resolved[HEADER_MAXLEN] = "";
    if (argc == 2 && argv[1]) {
        geoip_lookup(argv[1], resolved);
    }
    printf("%s\n", resolved);
    return 0;
}
#endif /* __VCL__ */

/* vim: syn=c ts=4 et sts=4 sw=4 tw=0
*/
