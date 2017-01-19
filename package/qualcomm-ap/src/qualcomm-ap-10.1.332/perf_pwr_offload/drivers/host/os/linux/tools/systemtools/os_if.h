#ifndef _OSIF_H_
#define _OSIF_H_

int cmd_init (char *ifname, void (*rx_cb)(void *buf));
int cmd_end();
void cmd_send (void *buf, int len, unsigned char responseNeeded );

unsigned int cmd_Art2ReadPciConfigSpace (unsigned int offset);
int cmd_Art2WritePciConfigSpace(unsigned int offset, unsigned int data);

#endif /* _OSIF_H_ */
