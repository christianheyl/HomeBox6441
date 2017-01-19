
LIB_VERMAJOR = 0
LIB_VERMINOR = 1
UDNSLIB_FILENAME = libudns.so

LIB_CFLAGS  = $(CFLAGS) -shared -fPIC -D_ALDK
LIB_LDFLAGS = $(LDFLAGS) -Wl,-soname,$(UDNSLIB_FILENAME).$(LIB_VERMAJOR).$(LIB_VERMINOR)

LIB_OBJ = dns_query.o dns_cache_query.o udns_jran.o udns_resolver.o udns_rr_ptr.o \
			udns_rr_a.o udns_rr_naptr.o udns_bl.o udns_misc.o udns_dn.o udns_dntosp.o \
			udns_parse.o udns_XtoX.o udns_rr_mx.o udns_rr_srv.c udns_init.o udns_codes.c inet_XtoX.o udns_rr_txt.o

# all: udns testudns
all: udns

udns: $(LIB_OBJ)
	$(CC) $(LIB_CFLAGS) $(LIB_LDFLAGS) \
		-o $(UDNSLIB_FILENAME).$(LIB_VERMAJOR).$(LIB_VERMINOR) $(LIB_OBJ)
	ln -sf $(UDNSLIB_FILENAME).$(LIB_VERMAJOR).$(LIB_VERMINOR) $(UDNSLIB_FILENAME)

testudns: testudns.o
		$(CC) $(CFLAGS) $(LDFLAGS) -L./ \
		-o testudns testudns.o -lmidcore -ludns -lc

%.o: %.c
	$(CC) $(LIB_CFLAGS) -c $< -o $@
	
clean:
	rm -f $(UDNSLIB_FILENAME) $(UDNSLIB_FILENAME)* *.o

depend:

install:
	# libs
	$(INSTALL_DIR) $(CONFIG_PREFIX)/usr/lib
	$(INSTALL_BIN) libudns.so* $(CONFIG_PREFIX)/usr/lib/

	# testudns
	# $(INSTALL_DIR) $(CONFIG_PREFIX)/usr/sbin
	# $(INSTALL_BIN) testudns $(CONFIG_PREFIX)/usr/sbin/
	
	# dev kit
	$(INSTALL_DIR) $(DEV_DIR)/usr/include
	$(CP) *.h $(DEV_DIR)/usr/include
	$(INSTALL_DIR) $(DEV_DIR)/usr/lib
	$(CP) libudns.so* $(DEV_DIR)/usr/lib

.PHONY: all clean depend install

