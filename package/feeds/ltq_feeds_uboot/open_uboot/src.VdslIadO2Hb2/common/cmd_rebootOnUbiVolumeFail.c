/*
 * reboot on ubi volume fail function
 *
 * linghong.tan 2013-10-16
 */

#include <common.h>
#include <command.h>

#define atoi(x)		simple_strtoul(x,NULL,10)

int do_reboot ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	char *tmp;
	int ubiErrorNumber;
	int ret = 0;

	if ((tmp = getenv("ubi_error_number")) == NULL) {
		/* if can not get ubi_error_number, just return 0 */
		return 0;
	}
	ubiErrorNumber = atoi(tmp);
	printf("ubi_error_number=[%d]\n", ubiErrorNumber);

	if(ubiErrorNumber){
		do_reset(NULL, NULL, NULL, NULL);
	}
	return 0;
}

U_BOOT_CMD(
	reboot_on_ubi_volume_fail,	1,	0,	do_reboot,
	"reboot if ubi_error_number is not 0",
	"\n"
);

