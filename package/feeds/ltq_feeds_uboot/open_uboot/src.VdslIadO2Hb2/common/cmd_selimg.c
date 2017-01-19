/*
 * selimg function
 *
 * linghong.tan 2013-05-14
 */

#include <common.h>
#include <command.h>

#define MAX_FAIL_NUM	6

#define atoi(x)		simple_strtoul(x,NULL,10)

char *simple_itoa(unsigned int i)
{
	/* 21 digits plus null terminator, good for 64-bit or smaller ints */
	static char local[22];
	char *p = &local[21];
	*p-- = '\0';
	do {
		*p-- = '0' + i % 10;
		i /= 10;
	} while (i > 0);
	return p + 1;
}

int do_selimg ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	char *tmp;
	int countA, countB;
	int index;
	int bootup_ok;
	int ret = 0;

	if ((tmp = getenv("bootup_ok")) == NULL) {
		/* if can not get bootup_ok, just disable this feature */
		return 0;
	}
	bootup_ok = atoi(tmp);
	printf("bootup_ok=[%d]\n", bootup_ok);

	if (bootup_ok) {
		/* acked by firmware, reset to default */
		setenv("failcount_A", "0");
		setenv("failcount_B", "0");
		setenv("bootup_ok", "0");

		goto done;
	}

	if ((tmp = getenv("update_chk")) == NULL) {
		return 0;
	}
	index = atoi(tmp);
	printf("update_chk=[%d]\n", index);

	if ((tmp = getenv("failcount_A")) == NULL) {
		return 0;
	}
	countA = atoi(tmp);
	if ((tmp = getenv("failcount_B")) == NULL) {
		return 0;
	}
	countB = atoi(tmp);
	printf("failcount_A=[%d], failcount_B=[%d]\n", countA, countB);


	switch (index) {

		/* A is active */
		case 0:
		case 3:
			if (countA++ < MAX_FAIL_NUM) {
				setenv("failcount_A", simple_itoa(countA));
			}
			else if (countB < MAX_FAIL_NUM) {

				/* if B is acked, switch to B */
				ret = setenv("update_chk", "2");
				printf("[failsafe]: Switch from A to B!\n");
			}
			else {
				printf("[WARN]: Both images have not been acked!\n");
				return 0;
			}

			break;

		/* B is active */
		case 1:
		case 2:
			if (countB ++ < MAX_FAIL_NUM) {
				setenv("failcount_B", simple_itoa(countB));
			}
			else if (countA < MAX_FAIL_NUM) {
				/* if A is acked, switch to A */
				ret = setenv("update_chk", "0");
				printf("[failsafe]: Switch from B to A!\n");
			}
			else {
				printf("[WARN]: Both images have not been acked!\n");
				return 0;
			}

			break;

		default:
			/* can not get current index, do nothing */
			printf("[WARN]: unknown image index!\n");
			return 0;
	}

done:
	/* FIXME: so bad to save all envs here */
	saveenv();

	return ret;
}

U_BOOT_CMD(
	selimg,	1,	1,	do_selimg,
	"check the count of img_X failed, then try to load another image img_Y to boot",
	"\n"
);

