#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <getopt.h>

#define simple_strtoX(func, type) \
	        static inline type simple_##func(const char *snum, int *error) \
{ \
	char *endptr; \
	type ret = func(snum, &endptr, 0); \
	\
	if (error && (!*snum || *endptr)) { \
		printf("%s: unable to parse the number '%s'", #func, snum); \
		*error = 1; \
	} \
	\
	return ret; \
}
simple_strtoX(strtoul, unsigned long int)

#define MAX_BUF         1024

unsigned char buf[MAX_BUF];
int fd, fd_w;

void die(const char * str, ...)
{
	va_list args;
	va_start(args, str);
	vfprintf(stderr, str, args);
	fputc('\n', stderr);
	close(fd);
	close(fd_w);
	exit(1);
}

long file_open(const char *name)
{
	struct stat sb;
	if ((fd = open(name, O_RDONLY, 0)) < 0){
		die("Unable to open `%s' : %m", name);
	}

	if (fstat (fd, &sb))
		die("Unable to stat `%s' : %m", name);

	return sb.st_size;
}

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
			return 0;
	}
	if (write(fd_w, buf, block1) != block1)
		return 0;

	return 1;
}

void usage(void)
{
	die("Usage: mkrom [-b|--boot] boot.bin [--bootsize] 0x40000 [-c|--code] code.bin [--codesize] 0x400000 [-o|--output] XXX.rom");
}

int main(int argc, char ** argv)
{
	int i,c;
	int debug, error = 0;
	long boot_size = 0, code_size = 0, boot_size_filled = 0, code_size_filled = 0;
	char *boot_file = NULL, *code_file = NULL, *output_file = NULL;

	int opt;
	extern char *optarg;
	extern int optind, opterr, optopt;
	int option_index=0;
	static struct option long_options[] =
	{
		{"debug", 0, 0, 'd'},
		{"boot", 1, 0, 'b'},
		{"bootsize", 1, 0, 0},
		{"code", 1, 0, 'c'},
		{"codesize", 1, 0, 0},
		{"output", 1, 0, 'o'},
		{0, 0, 0, 0}
	};

    printf("\n---------- build rom --------\n");

    while (1) {

        opt = getopt_long(argc, argv, "db:c:o:h?",long_options, &option_index);
        if (opt == -1)
            break;
        switch(opt) {
			case 0:
				switch(option_index) {
					case 2:
						boot_size_filled = simple_strtoul(optarg, &error);
						if (error || boot_size_filled <= 0) {
							printf("Invalid boot size!\n");
							return -1;
						}
						break;
					case 4:
						code_size_filled = simple_strtoul(optarg, &error);
						if (error || code_size_filled <= 0) {
							printf("Invalid code size!\n");
							return -1;
						}
						break;
					default:
						break;
				}
				break;

			case 'd' :
                debug=1; break;
            case 'h' :
			case '?':
                usage(); break;
            case 'b' :
                boot_file = optarg;
                printf("boot is [%s]\n", boot_file); break;
            case 'c' :
                code_file = optarg;
                printf("code is [%s]\n", code_file); break;
            case 'o' :
                output_file = optarg;
                printf("output file is [%s]\n", output_file); break;
            default :
                usage();
        }
	}

	if (!boot_file || !code_file || !output_file)
		usage();

	unlink(output_file);
	if ((fd_w = open(output_file, O_WRONLY|O_CREAT, S_IREAD | S_IWRITE)) < 0){
		die("Unable to open `%s' : %m", output_file);
	}

	/* write boot */
	boot_size = file_open(boot_file);

	fprintf(stderr, "%s is %ld bytes\n", boot_file, boot_size);

	for (i=0 ; (c=read(fd, buf, sizeof(buf)))>0 ; i+=c )
		if (write(fd_w, buf, c) != c)
			die("Write failed!\n");

	close (fd);

	if (boot_size < boot_size_filled) {
		/* fill NULL */
		if (!fill_null(boot_size_filled - boot_size))		/* default erase boot ENV */
			die("error!\n");
		printf("boot has been filled to (0x%lx) bytes\n", boot_size_filled);
	}

	/* write firmware: kernel+rootfs */
	code_size = file_open(code_file);

	fprintf (stderr, "%s is %ld bytes\n", code_file, code_size);

	for (i=0 ; (c=read(fd, buf, sizeof(buf)))>0 ; i+=c )
		if (write(fd_w, buf, c) != c)
			die("Write call failed");

	if (c != 0)
		die("read-error on `setup'");

	close (fd);

	/* fill NULL */
	if (code_size < code_size_filled) {
		if (!fill_null(code_size_filled - code_size))
			printf("error!\n");
		printf("code has been filled to (0x%lx) bytes\n", code_size_filled);
	}


	close(fd);
	close(fd_w);

	return 0;                       /* Everything is OK */
}
