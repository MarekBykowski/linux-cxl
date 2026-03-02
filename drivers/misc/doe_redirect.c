#include <linux/module.h>
#include <linux/pci.h>
#include <linux/rcupdate.h>
#include <linux/jump_label.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/pci-doe.h> /*struct pci_doe_task*/

DEFINE_STATIC_KEY_TRUE(doe_redirect_key);

/* TODO: wrap up all the variables into a single structure */

struct doe_redirect {
	struct pci_doe_mb *from;
	struct pci_doe_mb *to;
};

static struct doe_redirect __rcu *redir;

int (*redirect_hook)(struct pci_doe_mb *, struct pci_doe_task *);
static int __doe_redirect_submit(struct pci_doe_mb *mb,
				 struct pci_doe_task *task);

static struct dentry *dbg_dir;
static bool redirect_enabled;

#define DEV_A "0000:01:00.0"
#define DEV_B "0000:02:00.0"

static int doe_redirect_submit(struct pci_doe_mb *mb,
                               struct pci_doe_task *task)
{
        if (static_branch_likely(&doe_redirect_key))
		return __doe_redirect_submit(mb, task);

	/* If redirect disabled, fall back */
        return __pci_doe_submit_task(mb, task);
}

/* The heart of redirct */
static int __doe_redirect_submit(struct pci_doe_mb *mb,
                               struct pci_doe_task *task)
{
        struct doe_redirect *r;
        struct pci_doe_mb *target = mb;

	/*
	 * rcu lock guarantees that even though a writer replaces `redir`
	 * it stays valid during the read section
	 */
        rcu_read_lock();
        r = rcu_dereference(redir);

        if (r && mb == r->from)
                target = r->to;

        rcu_read_unlock();

	pr_debug("mb: DOE: Redirct mb from %s to %s\n",
		 pci_name(r->from->pdev), pci_name(r->to->pdev));

	/* After redirection, call original task submit */
        return __pci_doe_submit_task(target, task);
}

static void enable_redirect(void)
{
	static_branch_enable(&doe_redirect_key);
	redirect_enabled = true;
}

static void disable_redirect(void)
{
	static_branch_disable(&doe_redirect_key);
	synchronize_rcu();
	redirect_enabled = false;
}

/* ================================
 * Lookup helper
 * ================================ */
static struct pci_doe_mb *find_doe_mb(const char *bdf)
{
	struct pci_dev *pdev;

	pdev = pci_get_domain_bus_and_slot(0,
		simple_strtoul(bdf + 5, NULL, 16),
		PCI_DEVFN(
		simple_strtoul(bdf + 8, NULL, 16),
		simple_strtoul(bdf + 11, NULL, 16)));

	if (!pdev)
		return NULL;

	/* This assumes DOE already initialized */
	return pdev->doe_mb;
}

static ssize_t enable_write(struct file *file,
			    const char __user *buf,
			    size_t count, loff_t *ppos)
{
	char buf[8];
	int val;

	if (count > sizeof(kbuf) - 1)
		return -EINVAL;

	if (copy_from_user(kbuf, buf, count))
		return -EFAULT;

	buf[count] = '\0';
	if (kstrtoint(kbuf, 0, &val))
		return -EINVAL;

	if (val)
		enable_redirect();
	else
		disable_redirect();

	return count;
}

static ssize_t enable_read(struct file *f, char __user *user_buf,
			   size_t count, loff_t *ppos)
{
	char buf[4];
	int len;

	len = snprintf(buf, sizeof(buf), "%d\n", redirect_enabled ? 1 : 0);
	return simple_read_from_buffer(user_buf, len, ppos, buf, len);
}

static const struct file_operations enable_fops = {
	.write = enable_write,
	.read  = enable_read,
};

/* ================================
 * Module Init
 * ================================ */

static int __init doe_redirect_init(void)
{
	struct pci_doe_mb *a, *b;
	struct doe_redirect *r;

	pr_info("DOE redirect init\n");

	a = find_doe_mb(DEV_A);
	b = find_doe_mb(DEV_B);

	if (!a || !b) {
		pr_err("DOE devices not found\n");
		return -ENODEV;
	}

	r = kzalloc(sizeof(*r), GFP_KERNEL);
	if (!r)
		return -ENOMEM;

	r->from = a;
	r->to   = b;

	rcu_assign_pointer(redir, r);

	dbg_dir = debugfs_create_dir("doe_redirect", NULL);
	debugfs_create_file("enable", 0644, dbg_dir, NULL, &enable_fops);

	redirect_hook = doe_redirect_submit;

	pr_info("DOE redirect ready (echo 1 to enable)\n");
	return 0;
}

/* ================================
 * Module Exit
 * ================================ */

static void __exit doe_redirect_exit(void)
{
	struct doe_redirect *r;

	disable_redirect();

	r = rcu_dereference_protected(redir, 1);
	kfree(r);

	debugfs_remove_recursive(dbg_dir);

	pr_info("DOE redirect unloaded\n");
}

module_init(doe_redirect_init);
module_exit(doe_redirect_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("DOE MITM Example");
MODULE_DESCRIPTION("DOE A->B redirect using debugfs + RCU + static key");
