#ifndef _MODULE_H_
#define _MODULE_H_

/* IOCTL commands */
#define IOCTL_FAT_OPEN		0x01
#define IOCTL_FAT_CLOSE		0x02
#define IOCTL_FAT_READ		0x03
#define IOCTL_FAT_WRITE		0x04
#define IOCTL_FAT_SEEK		0x05
#define IOCTL_FAT_MKDIR		0x06
#define IOCTL_FAT_MKFILE	0x07
#define IOCTL_FAT_READDIR	0x08
#define IOCTL_FAT_DELETE	0x09
#define IOCTL_FAT_DELETEDIR	0x0A
#define IOCTL_FAT_RENAME	0x0B
#define IOCTL_FAT_STAT		0x0C
#define IOCTL_FAT_VFSSTATS	0x0D
#define IOCTL_FAT_FILESTATS	0x0E
#define IOCTL_FAT_GETUSAGE  0x0F
#define IOCTL_FAT_READDIR_SHORT	0xF8
#define IOCTL_FFS_MODE		0x80

#define IOCTL_FAT_MOUNTSD	0xF0
#define IOCTL_FAT_UMOUNTSD	0xF1
#define IOCTL_FAT_MOUNTUSB	0xF2
#define IOCTL_FAT_UMOUNTUSB	0xF3

/* Device names */
#define DEVICE_FAT		"fat"
#define DEVICE_SDIO		"sd:"
#define DEVICE_USB		"usb:"

#endif
