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
#include <GeoIP.h>
#include <pthread.h>

#define vcl_string char

/* HTTP Header will be json-like */
#define HEADER_MAXLEN 255
#define HEADER_TMPL "city:%s, country:%s, lat:%f, lon:%f, ip:%s"

pthread_mutex_t geoip_mutex = PTHREAD_MUTEX_INITIALIZER;
GeoIP* gi;
char *country;

/* Init GeoIP code */
void geoip_init () {
    if (!gi) {
        gi = GeoIP_open_type(GEOIP_COUNTRY_EDITION,GEOIP_MEMORY_CACHE);
        assert(gi);
    }
}

static int geoip_lookup(vcl_string *ip, vcl_string *resolved) {
    int lookup_success = 0;

    pthread_mutex_lock(&geoip_mutex);

    if (!gi) {
        geoip_init();
    }

    record = GeoIP_record_by_addr(gi, ip);

    /* Lookup succeeded */
    if (record) {
        lookup_success = 1;
        snprintf(resolved, HEADER_MAXLEN, HEADER_TMPL,
            record->city,
            record->country_code,
            record->latitude,
            record->longitude,
            ip
        );
    }

    /* Failed lookup */
    else {
        snprintf(resolved, HEADER_MAXLEN, HEADER_TMPL,
            "",      /* City */
            FALLBACK_COUNTRY,
            0.0f,    /* Latitude */
            0.0f,    /* Longitude */
            ip
        );
    }

    pthread_mutex_unlock(&geoip_mutex);

    return lookup_success;
}

static int geoip_lookup_country(vcl_string *ip, vcl_string *resolved) {
    int lookup_success = 0;

    pthread_mutex_lock(&geoip_mutex);

    if (!gi) {
        geoip_init();
    }

    country = (char *)GeoIP_country_code_by_addr(gi, ip);

    if (country) {
        lookup_success = 1;
        snprintf(resolved, HEADER_MAXLEN, "%s", country);
    } else {
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
    vcl_string *xff = VRT_GetHdr(sp, HDR_REQ, "\020X-Forwarded-For:");
    if(xff != NULL) ip = xff;
    if (geoip_lookup(ip, hval)) {
        VRT_SetHdr(sp, HDR_REQ, "\011X-Geo-IP:", hval, vrt_magic_string_end);
    }
    else {
        /* Send an empty header */
        VRT_SetHdr(sp, HDR_REQ, "\011X-Geo-IP:", "", vrt_magic_string_end);
    }
}

/* Simplified version: sets "X-Geo-IP" header with the country only */
void vcl_geoip_country_set_header(const struct sess *sp) {
    vcl_string hval[HEADER_MAXLEN];
    vcl_string *ip = VRT_IP_string(sp, VRT_r_client_ip(sp));
    geoip_lookup_country(ip, hval);
    VRT_SetHdr(sp, HDR_REQ, "\011X-Geo-IP:", hval, vrt_magic_string_end);
}

/* XFF version: takes IP from X-Forwarded-For header */
void vcl_geoip_country_set_header_xff(const struct sess *sp) {
    vcl_string hval[HEADER_MAXLEN];
    vcl_string *ip = VRT_GetHdr(sp, HDR_REQ, "\020X-Forwarded-For:");
    if (ip) {
        geoip_lookup_country(ip, hval);
        VRT_SetHdr(sp, HDR_REQ, "\011X-Geo-IP:", hval, vrt_magic_string_end);
    } else {
        VCL_Log("geoip: no ip from X-Forwarded-For");
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
