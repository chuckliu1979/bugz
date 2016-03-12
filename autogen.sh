#!/bin/sh
rm -rf autom4te.cache
rm -rf config.log config.status configure
autoreconf --force --install -I config -I m4
rm -rf autom4te.cache
rm -rf src/config.h.in~
