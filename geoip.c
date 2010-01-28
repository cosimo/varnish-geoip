/*
 * Varnish-powered Geo IP lookup
 * Cosimo, 28/01/2010
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <GeoIPCity.h>
#include <pthread.h>

#define vcl_string char
#define JSON_MAXLEN 255

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

static inline int geoip_lookup(vcl_string *ip, vcl_string *json) {
    int lookup_success = 0;

    pthread_mutex_lock(&geoip_mutex);

    if (!gi) {
        geoip_init();
    }

    record = GeoIP_record_by_addr(gi, ip);

    /* Lookup succeeded */
    if (record) {
        lookup_success = 1;
        snprintf(json, JSON_MAXLEN, "{\"city\":\"%s\",\"country\":\"%s\",\"lat\":\"%f\",\"lon\":\"%f\",\"classC\":\"%s\",\"netmask\":\"%d\"}",
            record->city,
            record->country_code,
            record->latitude,
            record->longitude,
            ip,
            GeoIP_last_netmask(gi)
        );
    }

    /* Failed lookup */
    else {
        strncpy(json, "{}", JSON_MAXLEN);
    }

    pthread_mutex_unlock(&geoip_mutex);

    return lookup_success;
}

#ifdef __VCL__
void vcl_geoip_handler(const struct session *sp) {

    vcl_string *ip = VRT_IP_string(sp, VRT_r_client_ip(sp));
    vcl_string json[JSON_MAXLEN];

    if (geoip_lookup(ip, json)) {
        /* Return json string as synthetic response,
           here varnishd itself responds to client */
        VRT_synth_page(sp, 0, json,  vrt_magic_string_end);
    }
    else {
        /* Send an empty JSON response */
        VRT_synth_page(sp, 0, "Geo = {}",  vrt_magic_string_end);
    }

    return;
}
#else
int main(int argc, char **argv) {
    vcl_string json[JSON_MAXLEN] = "";
    if (argc == 2 && argv[1]) {
        geoip_lookup(argv[1], json);
    }
    printf("%s\n", json);
    return 0;
}
#endif /* __VCL__ */

/* vim: syn=c ts=4 et sts=4 sw=4 tw=0
*/
