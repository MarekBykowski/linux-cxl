// SPDX-License-Identifier: GPL-2.0
/*
 * Intel Vendor Specific Extended Capabilities auxiliary bus driver
 *
 * Copyright (c) 2021, Intel Corporation.
 * All Rights Reserved.
 *
 * Author: David E. Box <david.e.box@linux.intel.com>
 *
 * This driver discovers and creates auxiliary devices for Intel defined PCIe
 * "Vendor Specific" and "Designated Vendor Specific" Extended Capabilities,
 * VSEC and DVSEC respectively. The driver supports features on specific PCIe
 * endpoints that exist primarily to expose them.
 */

#define DEBUG

#include <linux/auxiliary_bus.h>
#include <linux/bits.h>
#include <linux/kernel.h>
#include <linux/idr.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/types.h>
#include <linux/proc_fs.h>

#include "cxlpci.h"
#include "cxlmem.h"

#define CREATE_TRACE_POINTS
#include <trace/events/cxl.h>

static bool intel_walk_dvsec(struct pci_dev *pdev, unsigned long quirks)
{
	u16 p = 0;
	bool p_found = false;
	struct device *dev = &pdev->dev;

	do {
		u16 v, id;
		u16 cap, ctrl;
		struct cxl_endpoint_dvsec_info info = { 0 };
		int ranges = 0, i, hdm_count;

		p = pci_find_next_ext_capability(pdev, p, PCI_EXT_CAP_ID_DVSEC);
		if (!p)
			break;

		p_found = true;

		pci_read_config_word(pdev, p + PCI_DVSEC_HEADER1, &v);
		pci_read_config_word(pdev, p + PCI_DVSEC_HEADER2, &id);

		if ((PCI_DVSEC_VENDOR_ID_CXL == v) && (CXL_DVSEC_PCIE_DEVICE == id)) {
			pci_read_config_word(pdev, p + CXL_DVSEC_CAP_OFFSET, &cap);
			pci_read_config_word(pdev, p + CXL_DVSEC_CTRL_OFFSET, &ctrl);

			/*
			 * It is not allowed by spec for MEM.capable to be set and have 0 legacy
			 * HDM decoders (values > 2 are also undefined as of CXL 2.0). As this
			 * driver is for a spec defined class code which must be CXL.mem
			 * capable, there is no point in continuing to enable CXL.mem.
			 */
			hdm_count = FIELD_GET(CXL_DVSEC_HDM_COUNT_MASK, cap);
			if (!hdm_count || hdm_count > 2)
				continue;

			for (i = 0; i<hdm_count; i++) {
				u64 base, size;
				u32 temp;

				pci_read_config_dword(
					pdev, p + CXL_DVSEC_RANGE_SIZE_HIGH(i), &temp);
				size = (u64)temp << 32;

				pci_read_config_dword(
					pdev, p + CXL_DVSEC_RANGE_SIZE_LOW(i), &temp);
				size |= temp & CXL_DVSEC_MEM_SIZE_LOW_MASK;

				pci_read_config_dword(
					pdev, p + CXL_DVSEC_RANGE_BASE_HIGH(i), &temp);
				base = (u64)temp << 32;

				pci_read_config_dword(
					pdev, p + CXL_DVSEC_RANGE_BASE_LOW(i), &temp);
				base |= temp & CXL_DVSEC_MEM_BASE_LOW_MASK;

				info.dvsec_range[i] = (struct range) {
					.start = base,
					.end = base + size - 1
				};

				if (size)
					ranges++;
			}
		}

		dev_info(dev, "\tdvsec at %x: vendorID=%x ID=%x:\n",
			 p, v, id);

		if ((PCI_DVSEC_VENDOR_ID_CXL == v) && (CXL_DVSEC_PCIE_DEVICE == id)) {
			dev_info(dev, "\t\tcap %x\n", cap);
			dev_info(dev, "\t\tctrl %x\n", ctrl);
			for (i = 0; i<ranges; i++)
				dev_info(dev, "\t\trange[%d]: start-end %llx-%llx\n",
					 i,
					 info.dvsec_range[i].start,
					 info.dvsec_range[i].end);
		}
	} while (true);

	if (!p_found) {
		dev_warn(dev, "\tDevice DVSEC not present\n");
		return EINVAL;
	}

	return 0;
}

static bool intel_walk_doe(struct pci_dev *pdev, unsigned long quirks)
{
	u16 p = 0;
	u32 header, cap, ctrl, status;
	bool p_found = false;
	struct device *dev = &pdev->dev;

	while ((p = pci_find_next_ext_capability(pdev, p, PCI_EXT_CAP_ID_DOE))) {
		p_found = true;
		pci_read_config_dword(pdev, p + PCI_DOE_CAP, &header);
		pci_read_config_dword(pdev, p + PCI_DOE_CAP, &cap);
		pci_read_config_dword(pdev, p + PCI_DOE_CTRL, &ctrl);
		pci_read_config_dword(pdev, p + PCI_DOE_STATUS, &status);
		dev_info(dev, "\tdoe at %x\n", p);
		dev_info(dev, "\t\theader %x\n", header);
		dev_info(dev, "\t\tcap %x\n", cap);
		dev_info(dev, "\t\tctrl %x\n", ctrl);
		dev_info(dev, "\t\tstatu %x\n", status);
		dev_info(dev, "\t\tnot touching read/write and it will affect dev\n");
	}

	if (!p_found) {
		dev_warn(dev, "\tDevice DOE not present\n");
		return EINVAL;
	}

	return 0;
}

static int intel_vsec_pci_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	int rc;

	rc = pcim_enable_device(pdev);
	if (rc)
		return rc;

	intel_walk_dvsec(pdev, 0);
	intel_walk_doe(pdev, 0);

	return 0;
}

#define CXL_MEMORY_PROGIF       0x10
static const struct pci_device_id intel_vsec_pci_ids[] = {
	{ PCI_DEVICE_CLASS((PCI_CLASS_MEMORY_CXL << 8 | CXL_MEMORY_PROGIF), ~0) },
	{ /*sentinel*/ }
};

static ssize_t dvsec_write(struct file *file, const char __user *buf,
		       size_t count, loff_t *ppos)
{
	char to_tty[3] = {0};
	unsigned int var;
	struct pci_dev *dev = NULL;

	if (copy_from_user(to_tty, buf, count))
	    return -EFAULT;

	if (kstrtou32(to_tty, 10, &var))
	    return -EFAULT;

	pr_info("##### All pci #####\n");
	for_each_pci_dev(dev) {
		dev_info(&dev->dev, "vendor %x: device %x\n",
			 dev->vendor, dev->device);
		intel_vsec_pci_probe(dev, 0);
	}

	pr_info("##### Only CXL subclass #####\n");
	while ((dev = pci_get_class(intel_vsec_pci_ids[0].class, dev)) != NULL) {
		dev_info(&dev->dev, "dev: vendor %x: device %x\n",
			 dev->vendor, dev->device);
		intel_vsec_pci_probe(dev, 0);
	}
	return count;
}

static const struct proc_ops dvsec_ops = {
	.proc_write      = dvsec_write,
};


static int __init proc_init(void)
{
	proc_create("dvsec", S_IWUSR, NULL, &dvsec_ops);
	return 0;
}

device_initcall(proc_init);

MODULE_AUTHOR("Marek Bykowski");
MODULE_DESCRIPTION("Intel Extended Capabilities auxiliary driver");
MODULE_LICENSE("GPL v2");
