CC=cc
DEBUG=
#DEBUG=-g3

all: geoip geoip.vcl

geoip: geoip.c
	$(CC) -Wall -Wno-unused -pedantic $(DEBUG) -o geoip geoip.c -lGeoIP

geoip.vcl: Makefile geoip.c gen_vcl.pl
	./gen_vcl.pl < geoip.c > geoip.vcl

test:
	prove -I./t -v ./t

clean:
	rm geoip
	rm geoip.vcl
