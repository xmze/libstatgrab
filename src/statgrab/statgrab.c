/* 
 * i-scream central monitoring system
 * http://www.i-scream.org
 * Copyright (C) 2000-2003 i-scream
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <statgrab.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>

typedef enum {
	LONG_LONG = 0,
	TIME_T,
	FLOAT,
	DOUBLE,
	STRING,
	INT
} stat_type;

typedef enum {
	DISPLAY_LINUX = 0,
	DISPLAY_BSD,
	DISPLAY_MRTG,
	DISPLAY_PLAIN
} display_mode_type;

typedef enum {
	REPEAT_NONE = 0,
	REPEAT_ONCE,
	REPEAT_FOREVER
} repeat_mode_type;

typedef struct {
	char *name;
	stat_type type;
	void *stat;
} stat;

stat *stats = NULL;
int num_stats = 0;
int alloc_stats = 0;
#define INCREMENT_STATS 64

display_mode_type display_mode = DISPLAY_LINUX;
repeat_mode_type repeat_mode = REPEAT_NONE;
int repeat_time = 1;
int use_cpu_percent = 0;

/* Exit with an error message. */
void die(const char *s) {
	fprintf(stderr, "fatal: %s\n", s);
	exit(1);
}

/* Remove all the recorded stats. */
void clear_stats() {
	int i;

	for (i = 0; i < num_stats; i++)
		free(stats[i].name);
	free(stats);
	stats = NULL;
	num_stats = 0;
	alloc_stats = 0;
}

/* Add a stat. The varargs make up the name, joined with dots; the name is
   terminated with a NULL. */
void add_stat(stat_type type, void *stat, ...) {
	va_list ap;
	int len = 0;
	char *name, *p;

	/* Figure out how long the name will be, including dots and trailing
	   \0. */
	va_start(ap, stat);
	while (1) {
		const char *part = va_arg(ap, const char *);
		if (part == NULL)
			break;
		len += 1 + strlen(part);
	}
	va_end(ap);

	/* Paste the name together. */
	name = malloc(len);
	if (name == NULL)
		die("out of memory");
	p = name;
	va_start(ap, stat);
	while (1) {
		const char *part = va_arg(ap, const char *);
		int partlen;
		if (part == NULL)
			break;
		partlen = strlen(part);
		memcpy(p, part, partlen);
		p += partlen;
		*p++ = '.';
	}
	va_end(ap);
	*--p = '\0';

	/* Replace spaces with underscores. */
	for (p = name; *p != '\0'; p++) {
		if (*p == ' ')
			*p = '_';
	}

	/* Stretch the stats array if necessary. */
	if (num_stats >= alloc_stats) {
		alloc_stats += INCREMENT_STATS;
		stats = realloc(stats, alloc_stats * sizeof *stats);
		if (stats == NULL)
			die("out of memory");
	}

	stats[num_stats].name = name;
	stats[num_stats].type = type;
	stats[num_stats].stat = stat;
	++num_stats;
}

/* Compare two stats by name, for sorting purposes. */
int stats_compare(const void *a, const void *b) {
	return strcmp(((stat *)a)->name, ((stat *)b)->name);
}

/* Clear and rebuild the stats array. */
void get_stats(int use_diffs) {
	cpu_states_t *cpu_s;
	cpu_percent_t *cpu_p;
	mem_stat_t *mem;
	load_stat_t *load;
	user_stat_t *user;
	swap_stat_t *swap;
	general_stat_t *gen;
	disk_stat_t *disk;
	diskio_stat_t *diskio;
	process_stat_t *proc;
	network_stat_t *net;
	page_stat_t *page;
	static int zero = 0;
	int n, i;

	clear_stats();

	/* Constants, for use with MRTG mode. */
	add_stat(INT, &zero, "const", "0", NULL);

	/* FIXME when only fetching some stats, it'd be more efficient to only
	   do the libstatgrab calls needed, rather than fetching everything. */

	if (use_cpu_percent) {
		cpu_p = cpu_percent_usage();
		if (cpu_p != NULL) {
			add_stat(FLOAT, &cpu_p->user,
			         "cpu", "user", NULL);
			add_stat(FLOAT, &cpu_p->kernel,
			         "cpu", "kernel", NULL);
			add_stat(FLOAT, &cpu_p->idle,
			         "cpu", "idle", NULL);
			add_stat(FLOAT, &cpu_p->iowait,
			         "cpu", "iowait", NULL);
			add_stat(FLOAT, &cpu_p->swap,
			         "cpu", "swap", NULL);
			add_stat(FLOAT, &cpu_p->nice,
			         "cpu", "nice", NULL);
			add_stat(TIME_T, &cpu_s->systime,
			         "cpu", "time_taken", NULL);
		}
	} else {
		cpu_s = use_diffs ? get_cpu_diff() : get_cpu_totals();
		if (cpu_s != NULL) {
			add_stat(LONG_LONG, &cpu_s->user,
			         "cpu", "user", NULL);
			add_stat(LONG_LONG, &cpu_s->kernel,
			         "cpu", "kernel", NULL);
			add_stat(LONG_LONG, &cpu_s->idle,
			         "cpu", "idle", NULL);
			add_stat(LONG_LONG, &cpu_s->iowait,
			         "cpu", "iowait", NULL);
			add_stat(LONG_LONG, &cpu_s->swap,
			         "cpu", "swap", NULL);
			add_stat(LONG_LONG, &cpu_s->nice,
			         "cpu", "nice", NULL);
			add_stat(LONG_LONG, &cpu_s->total,
			         "cpu", "total", NULL);
			add_stat(TIME_T, &cpu_s->systime,
			         "cpu", "systime", NULL);
		}
	}

	mem = get_memory_stats();
	if (mem != NULL) {
		add_stat(LONG_LONG, &mem->total, "mem", "total", NULL);
		add_stat(LONG_LONG, &mem->free, "mem", "free", NULL);
		add_stat(LONG_LONG, &mem->used, "mem", "used", NULL);
		add_stat(LONG_LONG, &mem->cache, "mem", "cache", NULL);
	}

	load = get_load_stats();
	if (load != NULL) {
		add_stat(DOUBLE, &load->min1, "load", "min1", NULL);
		add_stat(DOUBLE, &load->min5, "load", "min5", NULL);
		add_stat(DOUBLE, &load->min15, "load", "min15", NULL);
	}

	user = get_user_stats();
	if (user != NULL) {
		add_stat(INT, &user->num_entries, "user", "num", NULL);
		add_stat(STRING, &user->name_list, "user", "names", NULL);
	}

	swap = get_swap_stats();
	if (swap != NULL) {
		add_stat(LONG_LONG, &swap->total, "swap", "total", NULL);
		add_stat(LONG_LONG, &swap->used, "swap", "used", NULL);
		add_stat(LONG_LONG, &swap->free, "swap", "free", NULL);
	}

	gen = get_general_stats();
	if (gen != NULL) {
		add_stat(STRING, &gen->os_name,
		         "general", "os_name", NULL);
		add_stat(STRING, &gen->os_release,
		         "general", "os_release", NULL);
		add_stat(STRING, &gen->os_version,
		         "general", "os_version", NULL);
		add_stat(STRING, &gen->platform, "general", "platform", NULL);
		add_stat(STRING, &gen->hostname, "general", "hostname", NULL);
		add_stat(TIME_T, &gen->uptime, "general", "uptime", NULL);
	}

	disk = get_disk_stats(&n);
	if (disk != NULL) {
		for (i = 0; i < n; i++) {
			/* FIXME it'd be nicer if libstatgrab did this */
			const char *name = disk[i].device_name,
			           *p = strrchr(name, '/');
			if (p != NULL)
				name = p + 1;
			if (*name == '\0')
				name = "root";
	
			add_stat(STRING, &disk[i].device_name,
			         "fs", name, "device_name", NULL);
			add_stat(STRING, &disk[i].fs_type,
			         "fs", name, "fs_type", NULL);
			add_stat(STRING, &disk[i].mnt_point,
			         "fs", name, "mnt_point", NULL);
			add_stat(LONG_LONG, &disk[i].size,
			         "fs", name, "size", NULL);
			add_stat(LONG_LONG, &disk[i].used,
			         "fs", name, "used", NULL);
			add_stat(LONG_LONG, &disk[i].avail,
			         "fs", name, "avail", NULL);
			add_stat(LONG_LONG, &disk[i].total_inodes,
			         "fs", name, "total_inodes", NULL);
			add_stat(LONG_LONG, &disk[i].used_inodes,
			         "fs", name, "used_inodes", NULL);
			add_stat(LONG_LONG, &disk[i].free_inodes,
			         "fs", name, "free_inodes", NULL);
		}
	}

	diskio = use_diffs ? get_diskio_stats_diff(&n) : get_diskio_stats(&n);
	if (diskio != NULL) {
		for (i = 0; i < n; i++) {
			const char *name = diskio[i].disk_name;
	
			add_stat(STRING, &diskio[i].disk_name,
			         "disk", name, "disk_name", NULL);
			add_stat(LONG_LONG, &diskio[i].read_bytes,
			         "disk", name, "read_bytes", NULL);
			add_stat(LONG_LONG, &diskio[i].write_bytes,
			         "disk", name, "write_bytes", NULL);
			add_stat(TIME_T, &diskio[i].systime,
			         "disk", name, "systime", NULL);
		}
	}

	proc = get_process_stats();
	if (proc != NULL) {
		add_stat(INT, &proc->total, "proc", "total", NULL);
		add_stat(INT, &proc->running, "proc", "running", NULL);
		add_stat(INT, &proc->sleeping, "proc", "sleeping", NULL);
		add_stat(INT, &proc->stopped, "proc", "stopped", NULL);
		add_stat(INT, &proc->zombie, "proc", "zombie", NULL);
	}

	net = use_diffs ? get_network_stats_diff(&n) : get_network_stats(&n);
	if (net != NULL) {
		for (i = 0; i < n; i++) {
			const char *name = net[i].interface_name;
	
			add_stat(STRING, &net[i].interface_name,
			         "net", name, "interface_name", NULL);
			add_stat(LONG_LONG, &net[i].tx,
			         "net", name, "tx", NULL);
			add_stat(LONG_LONG, &net[i].rx,
			         "net", name, "rx", NULL);
			add_stat(TIME_T, &net[i].systime,
			         "net", name, "systime", NULL);
		}
	}

	page = use_diffs ? get_page_stats_diff() : get_page_stats();
	if (page != NULL) {
		add_stat(LONG_LONG, &page->pages_pagein, "page", "in", NULL);
		add_stat(LONG_LONG, &page->pages_pageout, "page", "out", NULL);
		add_stat(LONG_LONG, &page->systime, "page", "systime", NULL);
	}

	qsort(stats, num_stats, sizeof *stats, stats_compare);
}

/* Print the value of a stat. */
void print_stat_value(const stat *s) {
	void *v = s->stat;

	switch (s->type) {
	case LONG_LONG:
		printf("%lld", *(long long *)v);
		break;
	case TIME_T:
		/* FIXME option for formatted time? */
		printf("%ld", *(time_t *)v);
		break;
	case FLOAT:
		printf("%f", *(float *)v);
		break;
	case DOUBLE:
		printf("%f", *(double *)v);
		break;
	case STRING:
		/* FIXME escaping? */
		printf("%s", *(char **)v);
		break;
	case INT:
		printf("%d", *(int *)v);
		break;
	}
}

/* Print the name and value of a stat. */
void print_stat(const stat *s) {
	switch (display_mode) {
	case DISPLAY_LINUX:
		printf("%s = ", s->name);
		break;
	case DISPLAY_BSD:
		printf("%s: ", s->name);
		break;
	case DISPLAY_MRTG:
	case DISPLAY_PLAIN:
		break;
	}
	print_stat_value(s);
	printf("\n");
}

/* Print stats as specified on the provided command line. */
void print_stats(int argc, char **argv) {
	int i;

	if (argc == optind) {
		/* Print all stats. */
		for (i = 0; i < num_stats; i++)
			print_stat(&stats[i]);
	} else {
		/* Print selected stats. */
		for (i = optind; i < argc; i++) {
			stat key;
			const stat *s;

			key.name = argv[i];
			s = (const stat *)bsearch(&key, stats, num_stats,
			                          sizeof *stats,
			                          stats_compare);
			if (s != NULL) {
				print_stat(s);
			}
		}
	}
}

void usage() {
	printf("Usage: statgrab [OPTION]... [STAT]...\n"
	       "Display system statistics (all statistics by default).\n"
	       "\n");
	printf("  -l         Linux sysctl-style output (default)\n"
	       "  -b         BSD sysctl-style output\n"
	       "  -m         MRTG-compatible output\n"
	       "  -u         Plain output (only show values)\n"
	       "  -n         Display cumulative stats once (default)\n"
	       "  -s         Display stat differences repeatedly\n"
	       "  -o         Display stat differences once\n"
	       "  -t DELAY   When repeating, wait DELAY seconds between updates (default 1)\n"
	       "  -p         Display CPU usage as percentages rather than absolute values\n"
	       "\n");
	printf("Report bugs to <bugs@i-scream.org>.\n");
	exit(1);
}

int main(int argc, char **argv) {
	opterr = 0;
	while (1) {
		int c = getopt(argc, argv, "lbmunsot:p");
		if (c == -1)
			break;
		switch (c) {
		case 'l':
			display_mode = DISPLAY_LINUX;
			break;
		case 'b':
			display_mode = DISPLAY_BSD;
			break;
		case 'm':
			display_mode = DISPLAY_MRTG;
			break;
		case 'u':
			display_mode = DISPLAY_PLAIN;
			break;
		case 'n':
			repeat_mode = REPEAT_NONE;
			break;
		case 's':
			repeat_mode = REPEAT_FOREVER;
			break;
		case 'o':
			repeat_mode = REPEAT_ONCE;
			break;
		case 't':
			repeat_time = atoi(optarg);
			break;
		case 'p':
			use_cpu_percent = 1;
			break;
		default:
			usage();
		}
	}

	if (display_mode == DISPLAY_MRTG) {
		if ((argc - optind) != 2)
			die("mrtg mode: must specify exactly two stats");
		if (repeat_mode != REPEAT_NONE)
			die("mrtg mode: cannot repeat display");
	}

	switch (repeat_mode) {
	case REPEAT_NONE:
		get_stats(0);
		print_stats(argc, argv);
		break;
	case REPEAT_ONCE:
		get_stats(1);
		sleep(repeat_time);
		get_stats(1);
		print_stats(argc, argv);
		break;
	case REPEAT_FOREVER:
		while (1) {
			get_stats(1);
			print_stats(argc, argv);
			printf("\n");
			sleep(repeat_time);
		}
	}

	if (display_mode == DISPLAY_MRTG) {
		printf("\n");
		printf("statgrab\n");
	}

	return 0;
}

