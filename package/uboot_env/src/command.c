#include "common.h"
#include "cmd_upgrade.h"
#include "command.h"
#include "crc32.h"

env_t env;

int read_env(void)
{
	int dev_fd;
	unsigned long crc = 0;

	ifx_debug_printf("MTD_CONFIG_DEV_NAME=%s, CFG_ENV_SIZE=%d\n", MTD_CONFIG_DEV_NAME, CFG_ENV_SIZE);
	ifx_debug_printf("CFG_ENV_ADDR=0x%08lx, MTD_DEV_START_ADD=0x%08lx\n", CFG_ENV_ADDR, MTD_DEV_START_ADD);

	dev_fd = open(MTD_CONFIG_DEV_NAME,O_SYNC | O_RDWR);
	if(dev_fd < 0){
		ifx_debug_printf("The device %s could not be opened\n", MTD_CONFIG_DEV_NAME);
		return 1;
	}

	if(lseek(dev_fd, CFG_ENV_ADDR - MTD_DEV_START_ADD, SEEK_CUR) < 0) {
		close(dev_fd);
		ifx_debug_printf("lseek fail!!\n");
		return 1;
	}

	read(dev_fd, (void *)&env, sizeof(env));
	close(dev_fd);
	ifx_debug_printf("read_env : Reading env for %d bytes at offset 0x%08lx\n", sizeof(env), CFG_ENV_ADDR - MTD_DEV_START_ADD);

	crc ^= 0xffffffffL;
	crc = crc32(crc, env.data, ENV_SIZE);
	crc ^= 0xffffffffL;

	ifx_debug_printf("env.crc=0x%08lx, crc=0x%08lx\n", env.crc, crc);

	if (crc != env.crc) {
		ifx_debug_printf("For enviornment CRC32 is not OK\n");
		return 1;
	}

	return 0;
}

int envmatch (unsigned char *s1, int i2)
{
	while (*s1 == env.data[i2++])
		if (*s1++ == '=')
			return(i2);
	if (*s1 == '\0' && env.data[i2-1] == '=')
		return(i2);
	return(-1);
}

char *get_env (unsigned char *name)
{
	int i, nxt;

	for (i=0; env.data[i] != '\0'; i=nxt+1) {
		int val;

		for (nxt=i; env.data[nxt] != '\0'; ++nxt) {
			if (nxt >= ENV_SIZE) {
				ifx_debug_printf("Did not get var %s with nxt = %d\n",name,nxt);
				return (NULL);
			}
		}
		if ((val=envmatch(name, i)) < 0)
			continue;
		return (&env.data[val]);
	}

	return (NULL);
}

void env_crc_update (void)
{
	env.crc = 0x00000000 ^ 0xffffffff;
	env.crc = crc32(env.crc, env.data, ENV_SIZE);
	env.crc ^= 0xffffffff;
}

int set_env(char *name,char *val)
{
	int   i, len, oldval;
	unsigned char *envptr, *nxt = NULL;
	unsigned char *env_data = env.data;

	if (!env_data)	/* need copy in RAM */
		return 1;

	/*
	 * search if variable with this name already exists
	 */
	oldval = -1;
	for (envptr=env_data; *envptr; envptr=nxt+1) {
		for (nxt=envptr; *nxt; ++nxt)
			;
		if ((oldval = envmatch(name, envptr-env_data)) >= 0)
			break;
	}

	ifx_debug_printf("set_env : the old value of %s = %s\n",name,envptr);
	/*
	 * Delete any existing definition
	 */
	if (oldval >= 0) {
		if (*++nxt == '\0') {
			if (envptr > env_data) {
				envptr--;
			} else {
				*envptr = '\0';
			}
		} else {
			for (;;) {
				*envptr = *nxt++;
				if ((*envptr == '\0') && (*nxt == '\0'))
					break;
				++envptr;
			}
		}
		*++envptr = '\0';
	}

	/*
	 * Append new definition at the end
	 */
	for (envptr=env_data; *envptr || *(envptr+1); ++envptr)
		;
	if (envptr > env_data)
		++envptr;
	/*
	 * Overflow when:
	 * "name" + "=" + "val" +"\0\0"  > ENV_SIZE - (envptr-env_data)
	 */
	len = strlen(name) + 2;
	/* add '=' for first arg, ' ' for all others */
	len += strlen(val) + 1;
	ifx_debug_printf("set_env : setting %s=%s for %d bytes\n",name,val,len);

	if (len > (&env_data[ENV_SIZE]-envptr)) {
		ifx_debug_printf ("## Error: environment overflow, \"%s\" deleted\n", name);
		return 1;
	}
	while ((*envptr = *name++) != '\0')
		envptr++;

	*envptr = '=';
	while ((*++envptr = *val++) != '\0')
		;

	/* end is marked with double '\0' */
	*++envptr = '\0';

	/* Update CRC */
	env_crc_update ();

	return 0;
}

#define getenv(x)		get_env(x)

int saveenv()
{
	ifx_debug_printf("Saving enviornment with CRC 0x%08lx\n",env.crc);
	//program_img(&env,sizeof(env),CFG_ENV_ADDR);
	program_img_2(&env,sizeof(env),CFG_ENV_ADDR);
	return 0;
}

int saveenv_copy()
{
	unsigned long ubootconfig_copy_addr;
	char *kernel_addr;
	char ubootconfig_copy_data[sizeof(env) + sizeof(UBOOTCONFIG_COPY_HEADER)];

	kernel_addr = getenv("f_kernel_addr");	
	ubootconfig_copy_addr = strtoul(kernel_addr,NULL,16) - sizeof(ubootconfig_copy_data); 
	memset(ubootconfig_copy_data,0x00,sizeof(ubootconfig_copy_data));
	sprintf(ubootconfig_copy_data,"%s",UBOOTCONFIG_COPY_HEADER);
	memcpy(ubootconfig_copy_data + sizeof(UBOOTCONFIG_COPY_HEADER),&env,sizeof(env));

	flash_sect_erase(ubootconfig_copy_addr,ubootconfig_copy_addr + sizeof(ubootconfig_copy_data) - 1,1);

	flash_write(ubootconfig_copy_data,ubootconfig_copy_addr,sizeof(ubootconfig_copy_data));
	return 0;
}

unsigned long find_mtd(unsigned long addr_first,char *mtd_dev)
{
	char strPartName[16];
	unsigned long part_begin_addr[MAX_PARTITION];
	int nPartNo,i;


	nPartNo = strtoul((char *)getenv("total_part"),NULL,10);
	if(nPartNo <= 0 || nPartNo >= MAX_PARTITION){
		ifx_debug_printf("Total no. of current partitions [%d] is out of limit (0,%d)\n",MAX_PARTITION);
		return 0;
	}

	for(i = 0; i < nPartNo; i++){
		memset(strPartName,0x00,sizeof(strPartName));
		sprintf(strPartName,"part%d_begin",i);
		part_begin_addr[i] = strtoul((char *)getenv(strPartName),NULL,16);
	}
	part_begin_addr[i] = strtoul((char *)getenv("flash_end"),NULL,16) + 1;

	for(i = 0; i < nPartNo; i++){
		if(addr_first >= part_begin_addr[i] && addr_first < part_begin_addr[i+1])
		{
			sprintf(mtd_dev,"/dev/mtd%d",i);
			ifx_debug_printf("Use MTD: %s\n", mtd_dev);
			return (part_begin_addr[i]);
		}
	}
	return 0;

}


#ifdef IFX_DEBUG
void
DBG_dump2(const char *label, const void *p, size_t len, char *InFile, unsigned int OnLine)
{
#   define DUMP_LABEL_WIDTH 20	/* arbitrary modest boundary */
#   define DUMP_WIDTH	(4 * (1 + 4 * 3) + 1)
    char buf[DUMP_LABEL_WIDTH + DUMP_WIDTH];
    char ascBuf[DUMP_WIDTH];
    char *ap;
    char *bp;
    const unsigned char *cp = p;

    bp = buf;
    ap = ascBuf;

    if (label != NULL && label[0] != '\0')
    {
	/* Handle the label.  Care must be taken to avoid buffer overrun. */
	size_t llen = strlen(label);

	if (llen + 1 > sizeof(buf))
	{
	    fprintf( stderr, "%s\n", label);
	}
	else
	{
	    strcpy(buf, label);
	    if (buf[llen-1] == '\n')
	    {
		buf[llen-1] = '\0';	/* get rid of newline */
		fprintf( stderr, "%s\n", buf);
	    }
	    else if (llen < DUMP_LABEL_WIDTH)
	    {
		bp = buf + llen;
	    }
	    else
	    {
		fprintf( stderr, "%s\n", buf);
	    }
	}
    }

    do {
	int i, j;

	*bp++ = '+';
	for (i = 0; len!=0 && i!=4; i++) {
	    *bp++ = ' ';  //*ap++ = ' ';
	    for (j = 0; len!=0 && j!=4; len--, j++)
	    {
		static const char hexdig[] = "0123456789abcdef";


		*ap++ = ((*cp<0x20)||(*cp>=0x80))?'.':*cp;
		*bp++ = ' ';
		*bp++ = hexdig[(*cp >> 4) & 0xF];
		*bp++ = hexdig[*cp & 0xF];
		cp++;
	    }
	}
	*bp = '\0';
	*ap = '\0';

	fprintf( stderr, "%s    ", buf);
	fprintf( stderr, "%s    \n", ascBuf);
	bp = buf;
	ap = ascBuf;
    } while (len != 0);
    fprintf( stderr, "     - %s %d\n", InFile, OnLine);
#   undef DUMP_LABEL_WIDTH
#   undef DUMP_WIDTH
}
#endif

