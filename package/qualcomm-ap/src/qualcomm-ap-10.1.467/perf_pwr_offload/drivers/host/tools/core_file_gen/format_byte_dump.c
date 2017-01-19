/* This program convers a binary dump of memory into a txt format of
 * address value pairs
 */
#include <string.h>
#include <stdio.h>


int main(int argc, char *argv[])
{
    unsigned int address, c, val, cnt;
    FILE *fp_dmp = NULL, *fp_txt = NULL;

    if (argc < 4) {
        printf("usage: format_byte_dump <dump_file> <txt> <start_address>\n");
        exit(-1);
    }

    address = strtoul(argv[3], NULL, 0);

    fp_dmp = fopen (argv[1], "r");
    if (!fp_dmp) {
        printf("Unable to open %s for read\n", argv[1]);
        exit(-1);
    }

    fp_txt = fopen (argv[2], "w");
    if (!fp_dmp) {
        printf("Unable to open %s for write\n", argv[2]);
        fclose(fp_txt);
        exit(-1);
    }

    cnt = 4;
    val = 0;
    while ((c = fgetc(fp_dmp)) != EOF) {

        /* Assuming we run on same endian machine as dump */
        val >>= 8;
        val |= (c & 0xFF) << 24;

        if (--cnt == 0) {
            fprintf(fp_txt, "0x%08X:0x%08X\n", address, val); 
            cnt = 4;
            val = 0;
            address += 4;
        }

    }

    fflush(fp_txt);
    fclose(fp_txt);
    fclose(fp_dmp);
}



