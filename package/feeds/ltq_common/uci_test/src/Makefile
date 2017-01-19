OBJS = uci_test.o omci_uci_config.o

all: uci_test

%.o: %.c
	$(CC) -c $(CFLAGS) $^ -o $@

uci_test: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) -lucimap -luci 

clean:
	rm -f uci_test *.o