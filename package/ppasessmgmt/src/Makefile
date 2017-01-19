SRCS    :=$(wildcard *.c)
OBJS    :=$(patsubst %.c, %.o,$(SRCS))

OFLAGS =	-O
CFLAGS =	-g ${OFLAGS} ${CFLAGS1} -I./cgi-src -I. -DINCLUDE_DSL_CPE_API_VRX -DCONFIG_PACKAGE_IFX_DSL_CPE_API -DDSL_CHANNELS_PER_LINE=1 -DLINUX

all: ppasessmgmt ppasessmgmtd

ppasessmgmt:ppasessmgmt.c ppasessmgmt.h
	$(CC) ${CFLAGS} ${LDFLAGS} ${COM_LDFLAGS} ${CGI_LDFLAGS}  -o $@ $^

ppasessmgmtd:ppasessmgmtd.c ppasessmgmt.h
	$(CC) ${CFLAGS} ${LDFLAGS} ${COM_LDFLAGS} ${CGI_LDFLAGS}  -o $@ $^	
	
clean:
	-rm -f *.o $(SRC)/*.o ppasessmgmt ppasessmgmtd
arc_install:
	$(STRIP) ppasessmgmt
	$(STRIP) ppasessmgmtd
