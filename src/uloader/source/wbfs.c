#include <stdio.h>
#include <malloc.h>
#include <ogcsys.h>

#include "usbstorage.h"
#include "utils.h"

#include "libwbfs/libwbfs.h"

/* Constants */
#define MAX_NB_SECTORS	32

/* Variables */
static wbfs_t *hdd = NULL;
static u32 nb_sectors, sector_size;


void __WBFS_Spinner(s32 x, s32 max)
{
	static time_t start;
	static u32 expected;

	f32 percent, size;
	u32 d, h, m, s;

	/* First time */
	if (!x) {
		start    = time(0);
		expected = 300;
	}

	/* Elapsed time */
	d = time(0) - start;

	if (x != max) {
		/* Expected time */
		if (d)
			expected = (expected * 3 + d * max / x) / 4;

		/* Remaining time */
		d = (expected > d) ? (expected - d) : 0;
	}

	/* Calculate time values */
	h =  d / 3600;
	m = (d / 60) % 60;
	s =  d % 60;

	/* Calculate percentage/size */
	percent = (x * 100.0) / max;
	size    = (hdd->wbfs_sec_sz / GB_SIZE) * max;

	Con_ClearLine();

	/* Show progress */
	if (x != max) {
		printf("    %.2f%% of %.2fGB (%c) ETA: %d:%02d:%02d\r", percent, size, "/|\\-"[(x / 10) % 4], h, m, s);
		fflush(stdout);
	} else
		printf("    %.2fGB copied in %d:%02d:%02d\n", size, h, m, s);
}

s32 __WBFS_ReadDVD(void *fp, u32 lba, u32 len, void *iobuf)
{
	void *buffer = NULL;

	u64 offset;
	u32 mod, size;
	s32 ret;

	/* Calculate offset */
	offset = ((u64)lba) << 2;

	/* Calcualte sizes */
	mod  = len % 32;
	size = len - mod;

	/* Read aligned data */
	if (size) {
		ret = WDVD_UnencryptedRead(iobuf, size, offset);
		if (ret < 0)
			goto out;
	}

	/* Read non-aligned data */
	if (mod) {
		/* Allocate memory */
		buffer = memalign(32, 0x20);
		if (!buffer)
			return -1;

		/* Read data */
		ret = WDVD_UnencryptedRead(buffer, 0x20, offset + size);
		if (ret < 0)
			goto out;

		/* Copy data */
		memcpy(iobuf + size, buffer, mod);
	}

	/* Success */
	ret = 0;

out:
	/* Free memory */
	if (buffer)
		free(buffer);

	return ret;
}

s32 __WBFS_ReadUSB(void *fp, u32 lba, u32 count, void *iobuf)
{
	u32 cnt = 0;
	s32 ret;

	/* Do reads */
	while (cnt < count) {
		void *ptr     = ((u8 *)iobuf) + (cnt * sector_size);
		u32   sectors = (count - cnt);

		/* Read sectors is too big */
		if (sectors > MAX_NB_SECTORS)
			sectors = MAX_NB_SECTORS;

		/* USB read */
		ret = USBStorage_ReadSectors(lba + cnt, sectors, ptr);
		if (ret < 0)
			return ret;

		/* Increment counter */
		cnt += sectors;
	}

	return 0;
}

s32 __WBFS_WriteUSB(void *fp, u32 lba, u32 count, void *iobuf)
{
	u32 cnt = 0;
	s32 ret;

	/* Do writes */
	while (cnt < count) {
		void *ptr     = ((u8 *)iobuf) + (cnt * sector_size);
		u32   sectors = (count - cnt);

		/* Write sectors is too big */
		if (sectors > MAX_NB_SECTORS)
			sectors = MAX_NB_SECTORS;

		/* USB write */
		ret = USBStorage_WriteSectors(lba + cnt, sectors, ptr);
		if (ret < 0)
			return ret;

		/* Increment counter */
		cnt += sectors;
	}

	return 0;
}


s32 WBFS_Init(void)
{
	s32 ret;

	/* Initialize USB storage */
	ret = USBStorage_Init();
	if (ret < 0)
		return ret;

	/* Get USB capacity */
	nb_sectors = USBStorage_GetCapacity(&sector_size);
	if (!nb_sectors)
		return -1;

	return 0;
}

s32 WBFS_Open(void)
{
	/* Close hard disk */
	if (hdd)
		wbfs_close(hdd);

	/* Open hard disk */
	hdd = wbfs_open_hd(__WBFS_ReadUSB, __WBFS_WriteUSB, NULL, sector_size, nb_sectors, 0);
	if (!hdd)
		return -1;

	return 0;
}

s32 WBFS_Format(u32 lba, u32 size)
{
	wbfs_t *partition = NULL;

	/* Reset partition */
	partition = wbfs_open_partition(__WBFS_ReadUSB, __WBFS_WriteUSB, NULL, sector_size, size, lba, 1);
	if (!partition)
		return -1;

	/* Free memory */
	wbfs_close(partition);

	return 0;
}

s32 WBFS_GetCount(u32 *count)
{
	/* No USB device open */
	if (!hdd)
		return -1;

	/* Get list length */
	*count = wbfs_count_discs(hdd);

	return 0;
}

s32 WBFS_GetHeaders(void *outbuf, u32 cnt, u32 len)
{
	u32 idx, size;
	s32 ret;

	/* No USB device open */
	if (!hdd)
		return -1;

	for (idx = 0; idx < cnt; idx++) {
		u8 *ptr = ((u8 *)outbuf) + (idx * len);

		/* Get header */
		ret = wbfs_get_disc_info(hdd, idx, ptr, len, &size);
		if (ret < 0)
			return ret;
	}

	return 0;
}

s32 WBFS_CheckGame(u8 *discid)
{
	wbfs_disc_t *disc = NULL;

	/* Try to open game disc */
	disc = wbfs_open_disc(hdd, discid);
	if (disc) {
		/* Close disc */
		wbfs_close_disc(disc);

		return 1;
	}

	return 0;
}

// added by Hermes
s32 WBFS_GetProfileDatas(u8 *discid, u8 *buff)
{
	wbfs_disc_t *disc = NULL;
	int n;

	/* Try to open game disc */
	disc = wbfs_open_disc(hdd, discid);
	if (disc) {
		
		wbfs_disc_read(disc,(1024>>2), buff, 1024);
		
		if(buff[0]=='H' && buff[1]=='D' && buff[2]=='R' && buff[3]!=0)
			buff[3]=200;
			wbfs_disc_read(disc,(2048>>2), buff+1024, ((u32) buff[3])*1024);
		/* Close disc */
		wbfs_close_disc(disc);

		return 1;
	}

	return 0;
}

// added by Hermes
s32 WBFS_SetProfileDatas(u8 *discid, u8 *buff)
{
	wbfs_disc_t *disc = NULL;
	int n;

	/* Try to open game disc */
	disc = wbfs_open_disc(hdd, discid);
	if (disc) {
		
		wbfs_disc_write(disc,(1024>>2), buff, 1024);
		
		if(buff[0]=='H' && buff[1]=='D' && buff[2]=='R' && buff[3]!=0)
			
			wbfs_disc_write(disc,(2048>>2), buff+1024, ((u32) buff[3])*1024);
		/* Close disc */
		wbfs_close_disc(disc);

		return 1;
	}

	return 0;
}



s32 WBFS_AddGame(void)
{
	s32 ret;

	/* No USB device open */
	if (!hdd)
		return -1;

	/* Add game to USB device */
	ret = wbfs_add_disc(hdd, __WBFS_ReadDVD, NULL, __WBFS_Spinner, ONLY_GAME_PARTITION, 0);
	if (ret < 0)
		return ret;

	return 0;
}

s32 WBFS_RemoveGame(u8 *discid)
{
	s32 ret;

	/* No USB device open */
	if (!hdd)
		return -1;

	/* Remove game from USB device */
	ret = wbfs_rm_disc(hdd, discid);
	if (ret < 0)
		return ret;

	return 0;
}

s32 WBFS_GameSize(u8 *discid, f32 *size)
{
	wbfs_disc_t *disc = NULL;

	u32 sectors;

	/* No USB device open */
	if (!hdd)
		return -1;

	/* Open disc */
	disc = wbfs_open_disc(hdd, discid);
	if (!disc)
		return -2;

	/* Get game size in sectors */
	sectors = wbfs_sector_used(hdd, disc->header);

	/* Close disc */
	wbfs_close_disc(disc);

	/* Copy value */
	*size = (hdd->wbfs_sec_sz / GB_SIZE) * sectors;

	return 0;
}

s32 WBFS_DiskSpace(f32 *used, f32 *free)
{
	f32 ssize;
	u32 cnt;

	/* No USB device open */
	if (!hdd)
		return -1;

	/* Count used blocks */
	cnt = wbfs_count_usedblocks(hdd);

	/* Sector size in GB */
	ssize = hdd->wbfs_sec_sz / GB_SIZE;

	/* Copy values */
	*free = ssize * cnt;
	*used = ssize * (hdd->n_wbfs_sec - cnt);

	return 0;
}
