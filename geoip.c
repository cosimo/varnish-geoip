/*
 * Varnish-powered Geo IP lookup
 *
 * Idea and GeoIP code taken from
 * http://svn.wikia-code.com/utils/varnishhtcpd/wikia.vcl
 *
 * Cosimo, 01/12/2011
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <GeoIP.h>
#include <GeoIPCity.h>
#include <pthread.h>
#include <syslog.h>

#define vcl_string char

/* HTTP Header will be json-like */
#define HEADER_MAXLEN 255
#define HEADER_TMPL "city:%s, country:%s, lat:%f, lon:%f, ip:%s"

pthread_mutex_t geoip_mutex = PTHREAD_MUTEX_INITIALIZER;
GeoIP* gi;
GeoIPRecord *record;
char *country;

/* Init GeoIP code */
void geoip_init () {
    if (!gi) {
        gi = GeoIP_open_type(GEOIP_COUNTRY_EDITION,GEOIP_MEMORY_CACHE);
        if (!gi) {
            syslog(LOG_ERR, "Failed to open GeoIP database");
        }
    }
}

static int geoip_lookup(vcl_string *ip, vcl_string *resolved) {
    int lookup_success = 0;

    pthread_mutex_lock(&geoip_mutex);

    if (!gi) {
        geoip_init();
    }

    if (gi) {
        record = GeoIP_record_by_addr(gi, ip);
    } else {
        record = NULL;
    }

    if (record) {
        lookup_success = 1;
        snprintf(resolved, HEADER_MAXLEN, HEADER_TMPL,
            record->city,
            record->country_code,
            record->latitude,
            record->longitude,
            ip
        );
    } else {
        snprintf(resolved, HEADER_MAXLEN, HEADER_TMPL,
            "",      /* City */
            "",
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

    if (gi) {
        country = (char *)GeoIP_country_code_by_addr(gi, ip);
    } else {
        country = NULL;
    }

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

inline int strncpy_until(vcl_string *dest, vcl_string *src, unsigned char n, char terminator) {
    while (*src && (*src != terminator) && n--) {
        *(dest++) = *(src++);
    }
    *(dest) = 0;
}

vcl_string *get_ip(const struct sess *sp, vcl_string *ip_buffer) {
    vcl_string *xff = VRT_GetHdr(sp, HDR_REQ, "\020X-Forwarded-For:");
    if(xff != NULL) {
        strncpy_until(ip_buffer, xff, 15, ',');
        return ip_buffer;
    }
    return VRT_IP_string(sp, VRT_r_client_ip(sp));
}

/* Sets "X-Geo-IP" header with the geoip resolved information */
void vcl_geoip_set_header(const struct sess *sp) {
    vcl_string hval[HEADER_MAXLEN];
    vcl_string ip_buffer[16];
    if (geoip_lookup(get_ip(sp, ip_buffer), hval)) {
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
    vcl_string ip_buffer[16];
    geoip_lookup_country(get_ip(sp, ip_buffer), hval);
    VRT_SetHdr(sp, HDR_REQ, "\021X-Geo-IP-Country:", hval, vrt_magic_string_end);
}

#else

int main(int argc, char **argv) {
    vcl_string resolved[HEADER_MAXLEN] = "";
    if (argc == 2 && argv[1]) {
        geoip_lookup_country(argv[1], resolved);
    }
    printf("%s\n", resolved);
    return 0;
}

#endif /* __VCL__ */

/* vim: syn=c ts=4 et sts=4 sw=4 tw=0
*/
