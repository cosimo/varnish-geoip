CC=cc
DEBUG=
#DEBUG=-g3

all: geoip geoip.vcl

geoip: geoip.c
	$(CC) -Wall -Wno-unused -pedantic $(DEBUG) -lGeoIP -o geoip geoip.c

geoip.vcl: Makefile geoip.c gen_vcl.pl
	./gen_vcl.pl < geoip.c > geoip.vcl

test:
	prove -I./t -v ./t

