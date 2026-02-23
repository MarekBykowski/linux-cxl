


#include <linux/module.h>
#include <linux/pci.h>
#include <linux/rcupdate.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/xarray.h>
#include <linux/pci-doe.h>
#include <cxlmem.h>

#define DEV_A "0000:01:00.0"
#define DEV_B "0000:02:00.0"

extern struct pci_doe_mb *resolve_existing_mb(struct pci_dev *pdev);

extern int (*redirect_hook)(struct pci_doe_mb *,
                            struct pci_doe_task *);

extern int __pci_doe_submit_task(struct pci_doe_mb *,
                                 struct pci_doe_task *);

struct cxl_dev_state; /* forward decalraton */

/* ============================================= */

struct redirect_ctx {
        struct pci_dev *pdev_a;
        struct pci_dev *pdev_b;
        struct pci_doe_mb *mb_a;
        struct pci_doe_mb *mb_b;
};

static struct redirect_ctx *ctx;
static struct dentry *dbg_dir;
static bool redirect_enabled;

/* ============================================= */

static struct pci_dev *get_pdev_from_bdf(const char *bdf)
{
        unsigned int dom, bus, dev, fn;

        if (sscanf(bdf, "%04x:%02x:%02x.%1x",
                   &dom, &bus, &dev, &fn) != 4)
                return NULL;

        return pci_get_domain_bus_and_slot(dom, bus,
                                           PCI_DEVFN(dev, fn));
}


/* ============================================= */
/* Resolve existing mailbox via CXL xarray       */
/* ============================================= */

struct pci_doe_mb *
resolve_existing_mb(struct pci_dev *pdev)
{
        struct cxl_dev_state *cxlds;
        struct pci_doe_mb *mb;
        unsigned long index;

        cxlds = pci_get_drvdata(pdev);
        if (!cxlds)
                return NULL;

        xa_for_each(&cxlds->doe_mbs, index, mb)
                return mb; /* usually only one */

        return NULL;
}
/* ============================================= */
/* Redirect hook                                 */
/* ============================================= */

static int doe_redirect_submit(struct pci_doe_mb *mb,
                               struct pci_doe_task *task)
{
        if (!redirect_enabled)
                return __pci_doe_submit_task(mb, task);

        if (mb == ctx->mb_a)
                return __pci_doe_submit_task(ctx->mb_b, task);

        if (mb == ctx->mb_b)
                return __pci_doe_submit_task(ctx->mb_a, task);

        return __pci_doe_submit_task(mb, task);
}

/* ============================================= */
/* DebugFS control                               */
/* ============================================= */

static ssize_t enable_write(struct file *f,
                            const char __user *buf,
                            size_t len, loff_t *ppos)
{
        char kbuf[8];
        int val;

        if (len > sizeof(kbuf) - 1)
                return -EINVAL;

        if (copy_from_user(kbuf, buf, len))
                return -EFAULT;

        kbuf[len] = 0;

        if (kstrtoint(kbuf, 0, &val))
                return -EINVAL;

        redirect_enabled = !!val;

        pr_info("DOE redirect %s\n",
                redirect_enabled ? "enabled" : "disabled");

        return len;
}

static ssize_t enable_read(struct file *f,
                           char __user *buf,
                           size_t len, loff_t *ppos)
{
        char tmp[4];
        int r;

        r = snprintf(tmp, sizeof(tmp), "%d\n",
                     redirect_enabled ? 1 : 0);

        return simple_read_from_buffer(buf, len,
                                       ppos, tmp, r);
}

static const struct file_operations enable_fops = {
        .owner = THIS_MODULE,
        .write = enable_write,
        .read  = enable_read,
};

/* ============================================= */
/* Module init                                   */
/* ============================================= */

static int __init doe_redirect_init(void)
{
        pr_info("DOE redirect init\n");

        ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
        if (!ctx)
                return -ENOMEM;

        ctx->pdev_a = get_pdev_from_bdf(DEV_A);
        ctx->pdev_b = get_pdev_from_bdf(DEV_B);

        if (!ctx->pdev_a || !ctx->pdev_b)
                return -ENODEV;

        ctx->mb_a = resolve_existing_mb(ctx->pdev_a);
        ctx->mb_b = resolve_existing_mb(ctx->pdev_b);

        if (!ctx->mb_a || !ctx->mb_b) {
                pr_err("Failed to resolve DOE mailboxes\n");
                return -ENODEV;
        }

        rcu_assign_pointer(redirect_hook,
                           doe_redirect_submit);

        dbg_dir = debugfs_create_dir("doe_redirect", NULL);
        debugfs_create_file("enable", 0644,
                            dbg_dir, NULL,
                            &enable_fops);

        pr_info("DOE redirect ready (echo 1 to enable)\n");
        return 0;
}

/* ============================================= */
/* Module exit                                   */
/* ============================================= */

static void __exit doe_redirect_exit(void)
{
        rcu_assign_pointer(redirect_hook, NULL);
        synchronize_rcu();

        debugfs_remove_recursive(dbg_dir);

        if (ctx) {
                if (ctx->pdev_a)
                        pci_dev_put(ctx->pdev_a);
                if (ctx->pdev_b)
                        pci_dev_put(ctx->pdev_b);
                kfree(ctx);
        }

        pr_info("DOE redirect unloaded\n");
}

module_init(doe_redirect_init);
module_exit(doe_redirect_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Marek");
MODULE_DESCRIPTION("Strict DOE redirect without first-transfer leak");
