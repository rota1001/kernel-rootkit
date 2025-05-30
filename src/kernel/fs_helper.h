#pragma once
#include <linux/limits.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/string.h>
#include <net/inet_sock.h>
#include <net/net_namespace.h>
#include "internel.h"

/**
 * proc_find_init - Get the `proc_dir_entry` structure of /proc
 *
 * Notice that this should be called before any other functions below
 */
void proc_find_init(void);

#define get_member_ptr(base, offset, type) \
    ((type *) (((char *) (base)) + (offset)))


/**
 * proc_get_name - Get the name of the node
 */
char *proc_get_name(struct proc_dir_entry *node);

/**
 * proc_find_by_path - Find the `proc_dir_entry` by path
 */
struct proc_dir_entry *proc_find_by_path(const char *path);

/**
 * proc_get_ops_by_path - Get the `proc_ops` by path
 */
struct proc_ops *proc_get_proc_ops_by_path(const char *path);

/**
 * proc_get_ops_by_path - Get the `proc_operations` by pat
 */
struct seq_operations *proc_get_seq_ops_by_path(const char *path);

/**
 * List the files and directories with printk.
 *
 * This is for debug purpose.
 */
void proc_list_dir(struct proc_dir_entry *entry);
