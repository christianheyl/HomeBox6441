#ifndef __ARC_BOOT_H
#define __ARC_BOOT_H
#pragma pack(1)

#if !defined(u8) && !defined(u16) && !defined(u32)
#define u8 	unsigned char
#define u16	unsigned short
#define u32	unsigned int
#endif	// uchar, ushort, uint, ulong //

#define IH_MAGIC	0x27051956	/* Image Magic Number		*/
#define IH_NMLEN		32		/* Image Name Length		*/

typedef struct mkimg_hdr {
	u32		ih_magic;	/* Image Header Magic Number	*/
	u32		ih_hcrc;	/* Image Header CRC Checksum	*/
	u32		ih_time;	/* Image Creation Timestamp	*/
	u32		ih_size;	/* Image Data Size		*/
	u32		ih_load;	/* Data	 Load  Address		*/
	u32		ih_ep;		/* Entry Point Address		*/
	u32		ih_dcrc;	/* Image Data CRC Checksum	*/
	u8		ih_os;		/* Operating System		*/
	u8		ih_arch;	/* CPU architecture		*/
	u8		ih_type;	/* Image Type			*/
	u8		ih_comp;	/* Compression Type		*/
	u8		ih_name[IH_NMLEN];	/* Image Name		*/
}MKIMG_HDR, *PMKIMG_HDR;

#define MKIMG_LEN			(sizeof(MKIMG_HDR))
#define NAND_BLOCK_SIZE		(1<<7)
#define ONE_K				(1<<10)
#define ROLL_MAX			(NAND_BLOCK_SIZE*ONE_K)
#define ROLL_MIN			(MKIMG_LEN)
#define BRANCH2ARCBOOT(dst, argc, argv)			((u32 (*)(int, char *[]))dst)(argc, argv)

int arc_img_dec(u8 *dec_data, u32 total, u32 *rnd);
__inline__ void arc_read(u32 ram_addr, u8 *dst, u32 len);
void *my_memset(void *s, int c, unsigned long n);
void my_memcpy(void * dest, void *src, unsigned long n);

#pragma pack()
#endif	// __ARC_BOOT_H //
