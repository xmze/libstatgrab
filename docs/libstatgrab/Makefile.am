# Makefile for libstatgrab/docs/libstatgrab
# http://www.i-scream.org/libstatgrab/
# $Id$

if MANPAGES
man_MANS =	$(statgrab_MANS) $(sg_cpu_MANS) $(sg_disk_MANS) $(sg_error_MANS) \
		$(sg_fs_MANS) $(sg_host_MANS) $(sg_load_MANS) $(sg_mem_MANS) \
		$(sg_netif_MANS) $(sg_netio_MANS) $(sg_page_MANS) $(sg_proc_MANS) \
		$(sg_user_MANS) $(sg_vect_MANS) $(sg_init_MANS) $(sg_mutex_MANS) \
		$(sg_intro_MANS) $(sg_glob_MANS) $(sg_seterr_MANS)

statgrab_MANS =	statgrab.3 libstatgrab.3

sg_intro_MANS =	sg_intro.3 sg_internal-intro.3

sg_cpu_MANS =	sg_get_cpu_stats.3 sg_get_cpu_stats_r.3 \
		sg_get_cpu_stats_diff.3 sg_get_cpu_stats_diff_between.3 \
		sg_get_cpu_percents.3 sg_get_cpu_percents_of.3 \
		sg_get_cpu_percents_r.3 sg_free_cpu_stats.3

sg_disk_MANS =	sg_get_disk_io_stats.3 sg_get_disk_io_stats_r.3 \
		sg_get_disk_io_stats_diff.3 sg_get_disk_io_stats_diff_between.3 \
		sg_free_disk_io_stats.3 \
		sg_disk_io_compare_name.3 sg_disk_io_compare_traffic.3

sg_error_MANS =	sg_get_error.3 sg_get_error_arg.3 sg_get_error_errno.3 \
		sg_get_error_details.3 sg_strperror.3 sg_str_error.3

sg_fs_MANS =	sg_get_fs_stats.3 sg_get_fs_stats_r.3 \
		sg_get_fs_stats_diff.3 sg_get_fs_stats_diff_between.3 \
		sg_free_fs_stats.3 \
		sg_get_valid_filesystems.3 sg_set_valid_filesystems.3 \
		sg_fs_compare_device_name.3 sg_fs_compare_mnt_point.3

sg_host_MANS =	sg_get_host_info.3 sg_get_host_info_r.3 sg_free_host_info.3

sg_load_MANS =	sg_get_load_stats.3 sg_get_load_stats_r.3 sg_free_load_stats.3

sg_mem_MANS =	sg_get_mem_stats.3 sg_get_mem_stats_r.3 sg_free_mem_stats.3 \
		sg_get_swap_stats.3 sg_get_swap_stats_r.3 sg_free_swap_stats.3

sg_netif_MANS =	sg_get_network_iface_stats.3 sg_get_network_iface_stats_r.3 \
		sg_free_network_iface_stats.3 sg_network_iface_compare_name.3

sg_netio_MANS =	sg_get_network_io_stats.3 sg_get_network_io_stats_r.3 \
		sg_get_network_io_stats_diff.3 sg_get_network_io_stats_diff_between.3 \
		sg_free_network_io_stats.3 sg_network_io_compare_name.3

sg_page_MANS =	sg_get_page_stats.3 sg_get_page_stats_r.3 \
		sg_get_page_stats_diff.3 sg_get_page_stats_diff_between.3 \
		sg_free_page_stats.3

sg_proc_MANS =	sg_get_process_stats.3 sg_get_process_stats_r.3 \
		sg_get_process_count.3 sg_get_process_count_of.3 \
		sg_get_process_count_r.3 sg_free_process_count.3 \
		sg_process_compare_cpu.3 sg_process_compare_gid.3 \
		sg_process_compare_name.3 sg_process_compare_pid.3 \
		sg_process_compare_res.3 sg_process_compare_size.3 \
		sg_process_compare_time.3 sg_process_compare_uid.3

sg_user_MANS =	sg_get_user_stats.3 sg_get_user_stats_r.3 \
		sg_free_user_stats.3

sg_vect_MANS =	sg_vector_create.3 sg_vector_free.3 \
		sg_vector_clear.3 sg_vector_resize.3 \
		sg_vector_clone.3 sg_vector_clone_into.3 \
		sg_vector_compute_diff.3 \
		sg_get_nelements.3 sg_free_stats_buf.3 \
		sg_prove_vector.3

sg_init_MANS =	sg_init.3 sg_snapshot.3 sg_shutdown.3 sg_drop_privileges.3

sg_mutex_MANS =	sg_lock_mutex.3 sg_unlock_mutex.3

sg_glob_MANS =	sg_comp_init.3 sg_comp_destroy.3 sg_comp_get_tls.3 \
		sg_global_lock.3 sg_global_unlock.3

sg_seterr_MANS=	sg_set_error.3 sg_set_error_with_errno.3 sg_clear_error.3 \
		sg_set_error_with_errno_code.3

sg_tools_MANS =	sg_update_string.3

EXTRA_DIST = $(man_MANS)

MAINTAINERCLEANFILES= $(man_MANS)

.NOTPARALLEL :

.xml.3:
	$(DOCBOOK2MAN) -N --symlinks --string-param \
		header-3="`git log -1 --format='%ad' --date=short $<`" \
		$<
endif
