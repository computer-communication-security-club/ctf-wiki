// Эмзэг kernel драйвер (/dev/vuln). ioctl-оор stack буферт өгөгдөл хадгалах/унших ба
// хэмжээ (size) шалгахгүйгээс kernel stack overflow (STORE) ба leak (FETCH) үүснэ.
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>

#define BUF_SIZE 0x100

struct req {
    unsigned long size;   // хэрэглэгчийн өгөх хэмжээ (шалгагддаггүй!)
    char *data;          // хэрэглэгчийн буфер
};

#define CMD_STORE _IOW('k', 1, struct req)
#define CMD_FETCH _IOR('k', 2, struct req)

static long vuln_ioctl(struct file *f, unsigned int cmd, unsigned long arg) {
    char buf[BUF_SIZE];
    char *p = buf;
    struct req r;

    __asm__ volatile("" : "+r"(p));   // буферийн хэмжээг компиляторт нуух (FORTIFY)
    if (copy_from_user(&r, (void __user *)arg, sizeof(r)))
        return -EFAULT;

    switch (cmd) {
    case CMD_STORE:
        if (copy_from_user(p, r.data, r.size))   // size шалгахгүй: STACK OVERFLOW
            return -EFAULT;
        break;
    case CMD_FETCH:
        if (copy_to_user(r.data, p, r.size))      // size шалгахгүй: stack leak
            return -EFAULT;
        break;
    default:
        return -EINVAL;
    }
    return 0;
}

static const struct file_operations vuln_fops = {
    .owner          = THIS_MODULE,
    .unlocked_ioctl = vuln_ioctl,
};

static struct miscdevice vuln_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = "vuln",
    .fops  = &vuln_fops,
    .mode  = 0666,
};

static int __init vuln_init(void) { return misc_register(&vuln_dev); }
static void __exit vuln_exit(void) { misc_deregister(&vuln_dev); }
module_init(vuln_init);
module_exit(vuln_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("intentionally vulnerable driver");
