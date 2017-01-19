
CC := $(TOOLPREFIX)gcc
TARGET_TYPE ?= AR9888

all:
	$(CC) -g3 -Wall  \
		-I ../../include/ \
		-I ../../../../include/ \
		-I ../../../../../include/ \
		-I ../../../../../../../drivers/wlan_modules/lmac/ath_dev/ \
		-I ../../../../../include/$(TARGET_TYPE)/ \
		-I ../../../../tools/systemtools/src2/include/ \
		athdiag.c Qc98xxEepromStructGet.c Qc98xxDevice.c -o athdiag

clean:
	rm -f athdiag
