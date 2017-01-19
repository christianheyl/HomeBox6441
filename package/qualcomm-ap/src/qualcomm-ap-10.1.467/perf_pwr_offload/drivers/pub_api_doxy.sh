#!/bin/bash
export DOXYISODATE=$(date +%Y-%m-%d)
export DOXYBUILDDATE="as of $DOXYISODATE"
export DOXYDIR=pub_api_dox
if -d $DOXYDIR; then rm -f $DOXYDIR/xml; rm -f $DOXYDIR/html; fi
doxygen pub_api_doxy.conf
