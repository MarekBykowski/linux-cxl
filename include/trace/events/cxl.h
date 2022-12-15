/* SPDX-License-Identifier: GPL-2.0 */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM cxl

#if !defined(_TRACE_CXL_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_CXL_H

#include <linux/tracepoint.h>
#include <linux/pci-doe.h>

struct cxl_mbox_cmd;
struct pci_doe_task;
struct pci_doe_protocol;

TRACE_EVENT(cxl_cam_write,
	TP_PROTO(unsigned int bus, unsigned int devfn, int reg, unsigned int value),
	TP_ARGS(bus, devfn, reg, value),
	TP_STRUCT__entry(
		__field(unsigned int,	bus)
		__field(unsigned int,	devfn)
		__field(int,		reg)
		__field(unsigned int,	value)
	),
	TP_fast_assign(
		__entry->bus = bus;
		__entry->devfn = devfn;
		__entry->reg = reg;
		__entry->value = value;
	),
	/* b:fn am w val */
	TP_printk("%x:%x:%x reg=%x access=cam -> %x",
		__entry->bus,
		PCI_SLOT(__entry->devfn),
		PCI_FUNC(__entry->devfn),
		__entry->reg,
		__entry->value
	)
);

TRACE_EVENT(cxl_cam_read,
	TP_PROTO(unsigned int bus, unsigned int devfn, int reg, unsigned int value),
	TP_ARGS(bus, devfn, reg, value),
	TP_STRUCT__entry(
		__field(unsigned int,	bus)
		__field(unsigned int,	devfn)
		__field(int,		reg)
		__field(unsigned int,	value)
	),
	TP_fast_assign(
		__entry->bus = bus;
		__entry->devfn = devfn;
		__entry->reg = reg;
		__entry->value = value;
	),
	/* b:fn am w val */
	TP_printk("%x:%x:%x reg=%x access=cam <- %x",
		__entry->bus,
		PCI_SLOT(__entry->devfn),
		PCI_FUNC(__entry->devfn),
		__entry->reg,
		__entry->value
	)
);

TRACE_EVENT(cxl_ecam_write,
	TP_PROTO(unsigned int bus, unsigned int devfn, int reg, void __iomem* addr, unsigned int value),
	TP_ARGS(bus, devfn, reg, addr, value),
	TP_STRUCT__entry(
		__field(unsigned int,	bus)
		__field(unsigned int,	devfn)
		__field(int,		reg)
		__field(void __iomem*,	addr)
		__field(unsigned int,	value)
	),
	TP_fast_assign(
		__entry->bus = bus;
		__entry->devfn = devfn;
		__entry->reg = reg;
		__entry->addr = addr;
		__entry->value = value;
	),
	TP_printk("%x:%x:%x reg=%x access=ecam mmio=VA:%px->PA:%lx -> %x",
		__entry->bus,
		PCI_SLOT(__entry->devfn),
		PCI_FUNC(__entry->devfn),
		__entry->reg,
		__entry->addr,
		is_vmalloc_addr(__entry->addr) ?
			vmalloc_to_pfn(__entry->addr) + ((unsigned long)__entry->addr & ~PAGE_MASK) :
			(__pa(__entry->addr) >> PAGE_SHIFT) + ((unsigned long)__entry->addr & ~PAGE_MASK),
		__entry->value
	)
);

TRACE_EVENT(cxl_ecam_read,
	TP_PROTO(unsigned int bus, unsigned int devfn, int reg, void __iomem* addr, unsigned int value),
	TP_ARGS(bus, devfn, reg, addr, value),
	TP_STRUCT__entry(
		__field(unsigned int,	bus)
		__field(unsigned int,	devfn)
		__field(int,		reg)
		__field(void __iomem*,	addr)
		__field(unsigned int,	value)
	),
	TP_fast_assign(
		__entry->bus = bus;
		__entry->devfn = devfn;
		__entry->reg = reg;
		__entry->addr = addr;
		__entry->value = value;
	),
	/* vmalloc_to_pfn */
	TP_printk("%x:%x:%x reg=%x access=ecam mmio=VA:%px->PA:%lx <- %x",
		__entry->bus,
		PCI_SLOT(__entry->devfn),
		PCI_FUNC(__entry->devfn),
		__entry->reg,
		__entry->addr,
		is_vmalloc_addr(__entry->addr) ?
			vmalloc_to_pfn(__entry->addr) + ((unsigned long)__entry->addr & ~PAGE_MASK) :
			(__pa(__entry->addr) >> PAGE_SHIFT) + ((unsigned long)__entry->addr & ~PAGE_MASK),
		__entry->value
	)
);

TRACE_EVENT(cxl_mbox_send_cmd_obsolete,
	TP_PROTO(u16 opcode, void *payload_in, void *payload_out, size_t size_in, size_t size_out, u16 return_code),
	TP_ARGS(opcode, payload_in, payload_out, size_in, size_out, return_code),
	TP_STRUCT__entry(
		__field(u16,	opcode)
		__field(void *,	payload_in)
		__field(void *,	payload_out)
		__field(size_t,	size_in)
		__field(size_t,	size_out)
		__field(u16,	return_code)
	),
	TP_fast_assign(
		__entry->opcode = opcode;
		__entry->payload_in = payload_in;
		__entry->payload_out = payload_out;
		__entry->size_in = size_in;
		__entry->size_out = size_out;
		__entry->return_code = return_code;
	),
	TP_printk("opcode=%x payload_in=%px payload_out=%px size_in=%zu size_out=%zu return_code=%x",
		__entry->opcode,
		__entry->payload_in,
		__entry->payload_out,
		__entry->size_in,
		__entry->size_out,
		__entry->return_code
	)
);

TRACE_EVENT(cxl_mbox_send_cmd,
	TP_PROTO(struct device *dev, struct cxl_mbox_cmd *mcmd),
	TP_ARGS(dev, mcmd),
	TP_STRUCT__entry(
		__string(dev_name, dev_name(dev))
		__field(u16,	opcode)
		__field(void *,	payload_in)
		__field(void *,	payload_out)
		__field(size_t,	size_in)
		__field(size_t,	size_out)
		__field(u16,	return_code)
	),
	TP_fast_assign(
		__assign_str(dev_name, dev_name(dev));
		__entry->opcode = mcmd->opcode;
		__entry->payload_in = mcmd->payload_in;
		__entry->payload_out = mcmd->payload_out;
		__entry->size_in = mcmd->size_in;
		__entry->size_out = mcmd->size_out;
		__entry->return_code = mcmd->return_code;
	),
	TP_printk("%s opcode=%x payload_in=%px payload_out=%px size_in=%zu size_out=%zu return_code=%x",
		__get_str(dev_name),
		__entry->opcode,
		__entry->payload_in,
		__entry->payload_out,
		__entry->size_in,
		__entry->size_out,
		__entry->return_code
	)
);

TRACE_EVENT(cxl_doe_sumbit_task,
	TP_PROTO(struct pci_doe_task *task),
	TP_ARGS(task),
	TP_STRUCT__entry(
		__string(dev_name, dev_name(&task->doe_mb->pdev->dev))
		__field(u16, vid)
		__field(u8, type)
		__field(void *,	request_pl)
		__field(size_t,	request_pl_sz)
		__field(size_t,	size_out)
		__field(void *,	response_pl)
		__field(size_t,	response_pl_sz)
		__field(int,	rv)
	),
	TP_fast_assign(
		__assign_str(dev_name, dev_name(&task->doe_mb->pdev->dev));
		__entry->vid = (task->prot).vid;
		__entry->type = (task->prot).type;
		__entry->request_pl = task->request_pl;
		__entry->request_pl_sz = task->request_pl_sz;
		__entry->response_pl = task->response_pl;
		__entry->response_pl_sz = task->response_pl_sz;
		__entry->rv = task->rv;
	),
	TP_printk("%s doe_vid=%x doe_type=%x request_pl=%px request_pl_sz=%zu response_pl=%px response_pl_sz=%zu rv=%d",
		__get_str(dev_name),
		__entry->vid,
		__entry->type,
		__entry->request_pl,
		__entry->request_pl_sz,
		__entry->response_pl,
		__entry->response_pl_sz,
		__entry->rv
	)
);

#endif /* _TRACE_CXL_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
