/*
 * Varnish-powered Geo IP lookup
 *
 * Idea and GeoIP code taken from
 * http://svn.wikia-code.com/utils/varnishhtcpd/wikia.vcl
 *
 * Cosimo, 28/05/2013
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <GeoIPCity.h>
#include <pthread.h>

#define vcl_string char

/* At Opera, we use the non-existent "A6" country
   code to identify Geo::IP failed lookups */
#define FALLBACK_COUNTRY "A6"

/* HTTP Header will be json-like */
#define HEADER_MAXLEN 255
#define HEADER_TMPL "city:%s, country:%s, lat:%f, lon:%f, ip:%s"

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
        free(record);
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

    pthread_mutex_lock(&geoip_mutex);

    if (!gi) {
        geoip_init();
    }

    record = GeoIP_record_by_addr(gi, ip);

    snprintf(resolved, HEADER_MAXLEN, "country:%s", record
        ? record->country_code
        : FALLBACK_COUNTRY
    );

    free(record);
    pthread_mutex_unlock(&geoip_mutex);

    /* Assume always success: we have FALLBACK_COUNTRY */
    return 1;
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

/* Sets "X-Geo-IP" header with the geoip resolved information */
/* XFF version: takes IP from X-Forwarded-For header */
void vcl_geoip_set_header_xff(const struct sess *sp) {
    vcl_string hval[HEADER_MAXLEN];
    vcl_string *ip = VRT_GetHdr(sp, HDR_REQ, "\020X-Forwarded-For:");
    if (ip) {
        if (geoip_lookup(ip, hval)) {
            VRT_SetHdr(sp, HDR_REQ, "\011X-Geo-IP:", hval, vrt_magic_string_end);
        }
        else {
            /* Send an empty header */
            VRT_SetHdr(sp, HDR_REQ, "\011X-Geo-IP:", "", vrt_magic_string_end);
        }
    } else {
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
        /*VCL_Log("geoip: no ip from X-Forwarded-For");*/
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

    GeoIP_delete(gi);
    gi = NULL;

    return 0;
}
#endif /* __VCL__ */

/* vim: syn=c ts=4 et sts=4 sw=4 tw=0
*/
