
# Compiler Options.
#CFLAGS	= -D_GNU_SOURCE -Os -Wall -I$(TI_uclibc_path)
 
#CC	= gcc
#LD	= ld

%.o:		%.c  $<
		$(CC) $(CFLAGS) -c $< -o $@

all: arp

clean:  
		rm -f *.o arp

arp: arp.o 
		$(CC) -o arp arp.c


# End of Makefile.
