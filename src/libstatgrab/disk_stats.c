/*
 * i-scream central monitoring system
 * http://www.i-scream.org
 * Copyright (C) 2000-2004 i-scream
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307 USA
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "statgrab.h"

#ifdef SOLARIS
#include <sys/mnttab.h>
#include <sys/statvfs.h>
#include <kstat.h>
#define VALID_FS_TYPES {"ufs", "tmpfs"}
#endif

#if defined(LINUX) || defined(CYGWIN)
#include <mntent.h>
#include <sys/vfs.h>
#include "tools.h"
#endif

#ifdef LINUX
#define VALID_FS_TYPES {"adfs", "affs", "befs", "bfs", "efs", "ext2", \
                        "ext3", "vxfs", "hfs", "hfsplus", "hpfs", "jffs", \
                        "jffs2", "minix", "msdos", "ntfs", "qnx4", "ramfs", \
                        "rootfs", "reiserfs", "sysv", "v7", "udf", "ufs", \
                        "umsdos", "vfat", "xfs", "jfs"}
#endif

#ifdef CYGWIN
#define VALID_FS_TYPES {"user"}
#endif

#ifdef ALLBSD
#include <sys/param.h>
#include <sys/ucred.h>
#include <sys/mount.h>
#endif
#if defined(FREEBSD) || defined(DFBSD)
#include <sys/dkstat.h>
#include <devstat.h>
#define VALID_FS_TYPES {"hpfs", "msdosfs", "ntfs", "udf", "ext2fs", \
                        "ufs", "mfs"}
#endif
#if defined(NETBSD) || defined(OPENBSD)
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/disk.h>
#define VALID_FS_TYPES {"ffs", "mfs", "msdos", "lfs", "adosfs", "ext2fs", \
                        "ntfs"}
#endif

#define START_VAL 1

char *copy_string(char *orig_ptr, const char *newtext){

	/* Maybe free if not NULL, and strdup rather than realloc and strcpy? */
	orig_ptr=realloc(orig_ptr, (1+strlen(newtext)));
        if(orig_ptr==NULL){
        	return NULL;
        }
        strcpy(orig_ptr, newtext);

	return orig_ptr;
}


void init_disk_stat(int start, int end, disk_stat_t *disk_stats){

	for(disk_stats+=start; start<=end; start++){
		disk_stats->device_name=NULL;
		disk_stats->fs_type=NULL;
		disk_stats->mnt_point=NULL;
		
		disk_stats++;
	}
}

int is_valid_fs_type(const char *type) {
	const char *types[] = VALID_FS_TYPES;
	int i;

	for (i = 0; i < (sizeof types / sizeof *types); i++) {
		if (strcmp(types[i], type) == 0) {
			return 1;
		}
	}
	return 0;
}

disk_stat_t *get_disk_stats(int *entries){

	static disk_stat_t *disk_stats;
	static int watermark=-1;

	int valid_type;
	int num_disks=0;
#if defined(LINUX) || defined (SOLARIS) || defined(CYGWIN)
	FILE *f;
#endif

	disk_stat_t *disk_ptr;

#ifdef SOLARIS
	struct mnttab mp;
	struct statvfs fs;
#endif
#if defined(LINUX) || defined(CYGWIN)
	struct mntent *mp;
	struct statfs fs;
#endif
#ifdef ALLBSD
	int nummnt;
	struct statfs *mp;
#endif

	if(watermark==-1){
		disk_stats=malloc(START_VAL * sizeof(disk_stat_t));
		if(disk_stats==NULL){
			return NULL;
		}
		watermark=START_VAL;
		init_disk_stat(0, watermark-1, disk_stats);
	}
#ifdef ALLBSD
	nummnt=getmntinfo(&mp , MNT_LOCAL);
	if (nummnt<=0){
		return NULL;
	}
	for(;nummnt--; mp++){
		valid_type = is_valid_fs_type(mp->f_fstypename);
#endif

#if defined(LINUX) || defined(CYGWIN)
	if ((f=setmntent("/etc/mtab", "r" ))==NULL){
		return NULL;
	}

	while((mp=getmntent(f))){
		if((statfs(mp->mnt_dir, &fs)) !=0){
			continue;
		}	

		valid_type = is_valid_fs_type(mp->mnt_type);
#endif

#ifdef SOLARIS
	if ((f=fopen("/etc/mnttab", "r" ))==NULL){
		return NULL;
	}
	while((getmntent(f, &mp)) == 0){
		if ((statvfs(mp.mnt_mountp, &fs)) !=0){
			continue;
		}
		valid_type = is_valid_fs_type(mp.mnt_fstype);
#endif

		if(valid_type){
			if(num_disks>watermark-1){
				disk_ptr=disk_stats;
				if((disk_stats=realloc(disk_stats, (watermark*2 * sizeof(disk_stat_t))))==NULL){
					disk_stats=disk_ptr;
					return NULL;
				}

				watermark=watermark*2;
				init_disk_stat(num_disks, watermark-1, disk_stats);
			}

			disk_ptr=disk_stats+num_disks;
#ifdef ALLBSD
			if((disk_ptr->device_name=copy_string(disk_ptr->device_name, mp->f_mntfromname))==NULL){
				return NULL;
			}

			if((disk_ptr->fs_type=copy_string(disk_ptr->fs_type, mp->f_fstypename))==NULL){
				return NULL;
			}

			if((disk_ptr->mnt_point=copy_string(disk_ptr->mnt_point, mp->f_mntonname))==NULL){
				return NULL;
			}

			disk_ptr->size = (long long)mp->f_bsize * (long long) mp->f_blocks;
			disk_ptr->avail = (long long)mp->f_bsize * (long long) mp->f_bavail;
			disk_ptr->used = (disk_ptr->size) - ((long long)mp->f_bsize * (long long)mp->f_bfree);

			disk_ptr->total_inodes=(long long)mp->f_files;
			disk_ptr->free_inodes=(long long)mp->f_ffree;
			/* Freebsd doesn't have a "available" inodes */
			disk_ptr->used_inodes=disk_ptr->total_inodes-disk_ptr->free_inodes;
#endif
#if defined(LINUX) || defined(CYGWIN)
			if((disk_ptr->device_name=copy_string(disk_ptr->device_name, mp->mnt_fsname))==NULL){
				return NULL;
			}
				
			if((disk_ptr->fs_type=copy_string(disk_ptr->fs_type, mp->mnt_type))==NULL){	
				return NULL;
			}

			if((disk_ptr->mnt_point=copy_string(disk_ptr->mnt_point, mp->mnt_dir))==NULL){
				return NULL;
			}
			disk_ptr->size = (long long)fs.f_bsize * (long long)fs.f_blocks;
			disk_ptr->avail = (long long)fs.f_bsize * (long long)fs.f_bavail;
			disk_ptr->used = (disk_ptr->size) - ((long long)fs.f_bsize * (long long)fs.f_bfree);

			disk_ptr->total_inodes=(long long)fs.f_files;
			disk_ptr->free_inodes=(long long)fs.f_ffree;
			/* Linux doesn't have a "available" inodes */
			disk_ptr->used_inodes=disk_ptr->total_inodes-disk_ptr->free_inodes;
#endif

#ifdef SOLARIS
			/* Memory leak in event of realloc failing */
			/* Maybe make this char[bigenough] and do strncpy's and put a null in the end? 
			 * Downside is its a bit hungry for a lot of mounts, as MNT_MAX_SIZE would prob 
			 * be upwards of a k each 
			 */
			if((disk_ptr->device_name=copy_string(disk_ptr->device_name, mp.mnt_special))==NULL){
				return NULL;
			}

			if((disk_ptr->fs_type=copy_string(disk_ptr->fs_type, mp.mnt_fstype))==NULL){
                                return NULL;
                        }
	
			if((disk_ptr->mnt_point=copy_string(disk_ptr->mnt_point, mp.mnt_mountp))==NULL){
                                return NULL;
                        }
			
			disk_ptr->size = (long long)fs.f_frsize * (long long)fs.f_blocks;
			disk_ptr->avail = (long long)fs.f_frsize * (long long)fs.f_bavail;
			disk_ptr->used = (disk_ptr->size) - ((long long)fs.f_frsize * (long long)fs.f_bfree);
		
			disk_ptr->total_inodes=(long long)fs.f_files;
			disk_ptr->used_inodes=disk_ptr->total_inodes - (long long)fs.f_ffree;
			disk_ptr->free_inodes=(long long)fs.f_favail;
#endif
			num_disks++;
		}
	}

	*entries=num_disks;	

	/* If this fails, there is very little i can do about it, so
           I'll ignore it :) */
#if defined(LINUX) || defined(CYGWIN)
	endmntent(f);
#endif
#if defined(SOLARIS)
	fclose(f);
#endif

	return disk_stats;

}
void diskio_stat_init(int start, int end, diskio_stat_t *diskio_stats){

	for(diskio_stats+=start; start<end; start++){
		diskio_stats->disk_name=NULL;
		
		diskio_stats++;
	}
}

diskio_stat_t *diskio_stat_malloc(int needed_entries, int *cur_entries, diskio_stat_t *diskio_stats){

        if(diskio_stats==NULL){

                if((diskio_stats=malloc(needed_entries * sizeof(diskio_stat_t)))==NULL){
                        return NULL;
                }
                diskio_stat_init(0, needed_entries, diskio_stats);
                *cur_entries=needed_entries;

                return diskio_stats;
        }


        if(*cur_entries<needed_entries){
                diskio_stats=realloc(diskio_stats, (sizeof(diskio_stat_t)*needed_entries));
                if(diskio_stats==NULL){
                        return NULL;
                }
                diskio_stat_init(*cur_entries, needed_entries, diskio_stats);
                *cur_entries=needed_entries;
        }

        return diskio_stats;
}

static diskio_stat_t *diskio_stats=NULL;	
static int num_diskio=0;	

#ifdef LINUX
typedef struct {
	int major;
	int minor;
} partition;
#endif

diskio_stat_t *get_diskio_stats(int *entries){

	static int sizeof_diskio_stats=0;
#ifndef LINUX
	diskio_stat_t *diskio_stats_ptr;
#endif

#ifdef SOLARIS
        kstat_ctl_t *kc;
        kstat_t *ksp;
	kstat_io_t kios;
#endif
#ifdef LINUX
	FILE *f;
	char *line_ptr;
	int major, minor;
	int has_pp_stats = 1;
	static partition *parts = NULL;
	static int alloc_parts = 0;
	int i, n;
	time_t now;
	const char *format;
#endif
#if defined(FREEBSD) || defined(DFBSD)
	static struct statinfo stats;
	static int stats_init = 0;
	int counter;
	struct device_selection *dev_sel = NULL;
	int n_selected, n_selections;
	long sel_gen;
	struct devstat *dev_ptr;
#endif
#ifdef NETBSD
	struct disk_sysctl *stats;
#endif
#ifdef OPENBSD
	int diskcount;
	char *disknames, *name, *bufpp;
	char **dk_name;
	struct diskstats *stats;
#endif
#ifdef NETBSD
#define MIBSIZE 3
#endif
#ifdef OPENBSD
#define MIBSIZE 2
#endif
#if defined(NETBSD) || defined(OPENBSD)
	int num_disks, i;
	int mib[MIBSIZE];
	size_t size;
#endif

	num_diskio=0;

#ifdef OPENBSD
	mib[0] = CTL_HW;
	mib[1] = HW_DISKCOUNT;

	size = sizeof(diskcount);
	if (sysctl(mib, MIBSIZE, &diskcount, &size, NULL, 0) < 0) {
		return NULL;
	}

	mib[0] = CTL_HW;
	mib[1] = HW_DISKNAMES;

	if (sysctl(mib, MIBSIZE, NULL, &size, NULL, 0) < 0) {
		return NULL;
	}

	disknames = malloc(size);
	if (disknames == NULL) {
		return NULL;
	}

	if (sysctl(mib, MIBSIZE, disknames, &size, NULL, 0) < 0) {
		return NULL;
	}

	dk_name = calloc(diskcount, sizeof(char *));
	bufpp = disknames;
	for (i = 0; i < diskcount && (name = strsep(&bufpp, ",")) != NULL; i++) {
		dk_name[i] = name;
	}
#endif

#if defined(NETBSD) || defined(OPENBSD)
	mib[0] = CTL_HW;
	mib[1] = HW_DISKSTATS;
#ifdef NETBSD
	mib[2] = sizeof(struct disk_sysctl);
#endif

	if (sysctl(mib, MIBSIZE, NULL, &size, NULL, 0) < 0) {
		return NULL;
	}

#ifdef NETBSD
	num_disks = size / sizeof(struct disk_sysctl);
#else
	num_disks = size / sizeof(struct diskstats);
#endif

	stats = malloc(size);
	if (stats == NULL) {
		return NULL;
	}

	if (sysctl(mib, MIBSIZE, stats, &size, NULL, 0) < 0) {
		return NULL;
	}

	for (i = 0; i < num_disks; i++) {
		u_int64_t rbytes, wbytes;

#ifdef NETBSD
#ifdef HAVE_DK_RBYTES
		rbytes = stats[i].dk_rbytes;
		wbytes = stats[i].dk_wbytes;
#else
		/* Before 1.7, NetBSD merged reads and writes. */
		rbytes = wbytes = stats[i].dk_bytes;
#endif
#else
		rbytes = wbytes = stats[i].ds_bytes;
#endif

		/* Don't keep stats for disks that have never been used. */
		if (rbytes == 0 && wbytes == 0) {
			continue;
		}

		diskio_stats = diskio_stat_malloc(num_diskio + 1,
		                                  &sizeof_diskio_stats,
		                                  diskio_stats);
		if (diskio_stats == NULL) {
			return NULL;
		}
		diskio_stats_ptr = diskio_stats + num_diskio;
		
		diskio_stats_ptr->read_bytes = rbytes;
		diskio_stats_ptr->write_bytes = wbytes;
		if (diskio_stats_ptr->disk_name != NULL) {
			free(diskio_stats_ptr->disk_name);
		}
#ifdef NETBSD
		diskio_stats_ptr->disk_name = strdup(stats[i].dk_name);
#else
		diskio_stats_ptr->disk_name = strdup(dk_name[i]);
#endif
		diskio_stats_ptr->systime = time(NULL);
	
		num_diskio++;	
	}

	free(stats);
#ifdef OPENBSD
	free(dk_name);
	free(disknames);
#endif
#endif

#if defined(FREEBSD) || defined(DFBSD)
	if (!stats_init) {
		stats.dinfo=malloc(sizeof(struct devinfo));
		if(stats.dinfo==NULL) return NULL;
		bzero(stats.dinfo, sizeof(struct devinfo));
		stats_init = 1;
	}
#ifdef FREEBSD5
	if ((devstat_getdevs(NULL, &stats)) < 0) return NULL;
	/* Not aware of a get all devices, so i said 999. If we ever
	 * find a machine with more than 999 disks, then i'll change
	 * this number :)
	 */
	if (devstat_selectdevs(&dev_sel, &n_selected, &n_selections, &sel_gen, stats.dinfo->generation, stats.dinfo->devices, stats.dinfo->numdevs, NULL, 0, NULL, 0, DS_SELECT_ONLY, 999, 1) < 0) return NULL;
#else
	if ((getdevs(&stats)) < 0) return NULL;
	/* Not aware of a get all devices, so i said 999. If we ever
	 * find a machine with more than 999 disks, then i'll change
	 * this number :)
	 */
	if (selectdevs(&dev_sel, &n_selected, &n_selections, &sel_gen, stats.dinfo->generation, stats.dinfo->devices, stats.dinfo->numdevs, NULL, 0, NULL, 0, DS_SELECT_ONLY, 999, 1) < 0) return NULL;
#endif

	for(counter=0;counter<stats.dinfo->numdevs;counter++){
		dev_ptr=&stats.dinfo->devices[dev_sel[counter].position];

		/* Throw away devices that have done nothing, ever.. Eg "odd" 
		 * devices.. like mem, proc.. and also doesn't report floppy
		 * drives etc unless they are doing stuff :)
		 */
#ifdef FREEBSD5
		if((dev_ptr->bytes[DEVSTAT_READ]==0) && (dev_ptr->bytes[DEVSTAT_WRITE]==0)) continue;
#else
		if((dev_ptr->bytes_read==0) && (dev_ptr->bytes_written==0)) continue;
#endif
		if((diskio_stats=diskio_stat_malloc(num_diskio+1, &sizeof_diskio_stats, diskio_stats))==NULL){
			return NULL;
		}
		diskio_stats_ptr=diskio_stats+num_diskio;

#ifdef FREEBSD5		
		diskio_stats_ptr->read_bytes=dev_ptr->bytes[DEVSTAT_READ];
		diskio_stats_ptr->write_bytes=dev_ptr->bytes[DEVSTAT_WRITE];
#else
		diskio_stats_ptr->read_bytes=dev_ptr->bytes_read;
		diskio_stats_ptr->write_bytes=dev_ptr->bytes_written;
#endif
		if(diskio_stats_ptr->disk_name!=NULL) free(diskio_stats_ptr->disk_name);
		asprintf((&diskio_stats_ptr->disk_name), "%s%d", dev_ptr->device_name, dev_ptr->unit_number);
		diskio_stats_ptr->systime=time(NULL);

		num_diskio++;
	}
	free(dev_sel);

#endif
#ifdef SOLARIS
	if ((kc = kstat_open()) == NULL) {
                return NULL;
        }

	for (ksp = kc->kc_chain; ksp; ksp = ksp->ks_next) {
                if (!strcmp(ksp->ks_class, "disk")) {

			if(ksp->ks_type != KSTAT_TYPE_IO) continue;
			/* We dont want metadevices appearins as num_diskio */
			if(strcmp(ksp->ks_module, "md")==0) continue;
                        if((kstat_read(kc, ksp, &kios))==-1){	
			}
			
			if((diskio_stats=diskio_stat_malloc(num_diskio+1, &sizeof_diskio_stats, diskio_stats))==NULL){
				kstat_close(kc);
				return NULL;
			}
			diskio_stats_ptr=diskio_stats+num_diskio;
			
			diskio_stats_ptr->read_bytes=kios.nread;
			
			diskio_stats_ptr->write_bytes=kios.nwritten;

			if(diskio_stats_ptr->disk_name!=NULL) free(diskio_stats_ptr->disk_name);

			diskio_stats_ptr->disk_name=strdup((char *) get_svr_from_bsd(ksp->ks_name));
			diskio_stats_ptr->systime=time(NULL);
			num_diskio++;
		}
	}

	kstat_close(kc);
#endif

#ifdef LINUX
	num_diskio = 0;
	n = 0;

	/* Read /proc/partitions to find what devices exist. Recent 2.4 kernels
	   have statistics in here too, so we can use those directly.
	   2.6 kernels have /proc/diskstats instead with almost (but not quite)
	   the same format. */

	f = fopen("/proc/diskstats", "r");
	format = " %d %d %19s %*d %*d %lld %*d %*d %*d %lld";
	if (f == NULL) {
		f = fopen("/proc/partitions", "r");
		format = " %d %d %*d %19s %*d %*d %lld %*d %*d %*d %lld";
	}
	if (f == NULL) goto out;
	now = time(NULL);

	while ((line_ptr = f_read_line(f, "")) != NULL) {
		char name[20];
		char *s;
		long long rsect, wsect;

		int nr = sscanf(line_ptr, format,
			&major, &minor, name, &rsect, &wsect);
		if (nr < 3) continue;

		/* Skip device names ending in numbers, since they're
		   partitions. */
		s = name;
		while (*s != '\0') s++;
		--s;
		if (*s >= '0' && *s <= '9') continue;

		if (nr < 5) {
			has_pp_stats = 0;
			rsect = 0;
			wsect = 0;
		}

		diskio_stats = diskio_stat_malloc(n + 1, &sizeof_diskio_stats,
			diskio_stats);
		if (diskio_stats == NULL) goto out;
		if (n >= alloc_parts) {
			alloc_parts += 16;
			parts = realloc(parts, alloc_parts * sizeof *parts);
			if (parts == NULL) {
				alloc_parts = 0;
				goto out;
			}
		}

		if (diskio_stats[n].disk_name != NULL)
			free(diskio_stats[n].disk_name);
		diskio_stats[n].disk_name = strdup(name);
		diskio_stats[n].read_bytes = rsect * 512;
		diskio_stats[n].write_bytes = wsect * 512;
		diskio_stats[n].systime = now;
		parts[n].major = major;
		parts[n].minor = minor;

		n++;
	}

	fclose(f);
	f = NULL;

	if (!has_pp_stats) {
		/* This is an older kernel where /proc/partitions doesn't
		   contain stats. Read what we can from /proc/stat instead, and
		   fill in the appropriate bits of the list allocated above. */

		f = fopen("/proc/stat", "r");
		if (f == NULL) goto out;
		now = time(NULL);
	
		line_ptr = f_read_line(f, "disk_io:");
		if (line_ptr == NULL) goto out;
	
		while((line_ptr=strchr(line_ptr, ' '))!=NULL){
			long long rsect, wsect;

			if (*++line_ptr == '\0') break;
	
			if((sscanf(line_ptr,
				"(%d,%d):(%*d, %*d, %lld, %*d, %lld)",
				&major, &minor, &rsect, &wsect)) != 4) {
					continue;
			}

			/* Find the corresponding device from earlier.
			   Just to add to the fun, "minor" is actually the disk
			   number, not the device minor, so we need to figure
			   out the real minor number based on the major!
			   This list is not exhaustive; if you're running
			   an older kernel you probably don't have fancy
			   I2O hardware anyway... */
			switch (major) {
			case 3:
			case 21:
			case 22:
			case 33:
			case 34:
			case 36:
			case 56:
			case 57:
			case 88:
			case 89:
			case 90:
			case 91:
				minor *= 64;
				break;
			case 9:
			case 43:
				break;
			default:
				minor *= 16;
				break;
			}
			for (i = 0; i < n; i++) {
				if (major == parts[i].major
					&& minor == parts[i].minor)
					break;
			}
			if (i == n) continue;

			/* We read the number of blocks. Blocks are stored in
			   512 bytes */
			diskio_stats[i].read_bytes = rsect * 512;
			diskio_stats[i].write_bytes = wsect * 512;
			diskio_stats[i].systime = now;
		}
	}

	num_diskio = n;
out:
	if (f != NULL) fclose(f);
#endif

#ifdef CYGWIN
	return NULL;
#endif

	*entries=num_diskio;

	return diskio_stats;
}

diskio_stat_t *get_diskio_stats_diff(int *entries){
	static diskio_stat_t *diff = NULL;
	static int diff_count = 0;
	diskio_stat_t *src, *dest;
	int i, j, new_count;

	if (diskio_stats == NULL) {
		/* No previous stats, so we can't calculate a difference. */
		return get_diskio_stats(entries);
	}

	/* Resize the results array to match the previous stats. */
	diff = diskio_stat_malloc(num_diskio, &diff_count, diff);
	if (diff == NULL) {
		return NULL;
	}

	/* Copy the previous stats into the result. */
	for (i = 0; i < diff_count; i++) {
		src = &diskio_stats[i];
		dest = &diff[i];

		if (dest->disk_name != NULL) {
			free(dest->disk_name);
		}
		dest->disk_name = strdup(src->disk_name);
		dest->read_bytes = src->read_bytes;
		dest->write_bytes = src->write_bytes;
		dest->systime = src->systime;
	}

	/* Get a new set of stats. */
	if (get_diskio_stats(&new_count) == NULL) {
		return NULL;
	}

	/* For each previous stat... */
	for (i = 0; i < diff_count; i++) {
		dest = &diff[i];

		/* ... find the corresponding new stat ... */
		for (j = 0; j < new_count; j++) {
			/* Try the new stat in the same position first,
			   since that's most likely to be it. */
			src = &diskio_stats[(i + j) % new_count];
			if (strcmp(src->disk_name, dest->disk_name) == 0) {
				break;
			}
		}
		if (j == new_count) {
			/* No match found. */
			continue;
		}

		/* ... and subtract the previous stat from it to get the
		   difference. */
		dest->read_bytes = src->read_bytes - dest->read_bytes;
		dest->write_bytes = src->write_bytes - dest->write_bytes;
		dest->systime = src->systime - dest->systime;
	}

	*entries = diff_count;
	return diff;
}

