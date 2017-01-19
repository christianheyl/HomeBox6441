#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <getopt.h>

#define MAX_BUF	1024

int fd, fd_w;

void die(const char * str, ...)
{
	va_list args;
	va_start(args, str);
	vfprintf(stderr, str, args);
	fputc('\n', stderr);
	exit(1);
}

long
file_open(const char *name)
{
	struct stat sb;
	if ((fd = open(name, O_RDONLY, 0)) < 0){
		die("Unable to open `%s' : %m", name);
	}

	if (fstat (fd, &sb))
		die("Unable to stat `%s' : %m", name);

	return sb.st_size;
}

/* linghong_tan 12/05/13. fill image with 0xff */
int fill_null(int size)
{
	unsigned char buf[]={[0 ... MAX_BUF] = 0xFF};
	int i;
	long block0,block1;

	fprintf(stderr,"Fill null, size=[%d]\n", size);

	block0 = size / MAX_BUF;
	block1 = size % MAX_BUF;

	printf("block0=[%ld], block1=[%ld]\n", block0, block1);
	for (i=0; i< block0 ;i++){
		if (write(fd_w, buf, MAX_BUF) != MAX_BUF)
			return -1;
	}
	if (write(fd_w, buf, block1) != block1)
		return -1;

	return 0;
}

void usage(void)
{
	die("Usage: fillff [-i|--input] input.bin [-o|--output] output.bin [-s|--span] 0x200000");
}

int main(int argc, char ** argv)
{
	int i;
	long input_size, c, to_write;
	char *input_file = NULL, *output_file = NULL;
	long span_size = 0;
	int opt;
	int option_index=0;
	char buf[MAX_BUF];
	extern char *optarg;
	extern int optind, opterr, optopt;

	static struct option long_options[] =
	{
		{"input", 1, 0, 'i'},
		{"output", 1, 0, 'o'},
		{"span", 1, 0, 's'},
		{0, 0, 0, 0}
	};

	while (1) {
		opt = getopt_long(argc, argv, "i:o:s:",long_options, &option_index);
		if (opt == -1)
			break;

		switch(opt) {
			case 'h' : 
				usage(); break;
			case 'i' :
				input_file = optarg;
				printf("input file is [%s]\n",input_file); break;
			case 'o' :
				output_file = optarg;
				printf("output file is [%s]\n",output_file); break;
			case 's' :
				span_size = strtol(optarg, NULL, 0);
				printf("Span size is [%ld]\n",span_size); break;
			default :
				usage();
		}
	}

	if (!input_file || !output_file)
		usage();

	printf("\n---------- Fill image --------\n\n");
	
	if ((fd_w = open(output_file,O_RDWR|O_CREAT, S_IREAD | S_IWRITE)) < 0){
		die("Unable to open `%s' : %m", output_file);
	}
	
	input_size = file_open(input_file);

	fprintf(stderr, "%s is %ld bytes\n", input_file, input_size);
	
	memset(buf, 0, sizeof(buf));

	to_write = input_size;

	while (to_write > 0) {
		c = read(fd, buf, sizeof(buf));
		if (c <= 0)
			break;

		if (write(fd_w, buf, c) != c)
			die("Write call failed!\n");

		to_write -= c;
	}

	/* Fill 0xFF to other remain space */
	if (span_size > input_size)
		if (fill_null(span_size-input_size) != 0)
			die("Fill error!\n");;

fail:
	close (fd);
	close (fd_w);
	
	return 0;
}
