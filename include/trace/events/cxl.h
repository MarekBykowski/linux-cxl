/* SPDX-License-Identifier: GPL-2.0 */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM cxl

#if !defined(_CXL_EVENTS_H) || defined(TRACE_HEADER_MULTI_READ)
#define _CXL_EVENTS_H

#include <linux/tracepoint.h>

#define CXL_HEADERLOG_SIZE		SZ_512
#define CXL_HEADERLOG_SIZE_U32		SZ_512 / sizeof(u32)

#define CXL_RAS_UC_CACHE_DATA_PARITY	BIT(0)
#define CXL_RAS_UC_CACHE_ADDR_PARITY	BIT(1)
#define CXL_RAS_UC_CACHE_BE_PARITY	BIT(2)
#define CXL_RAS_UC_CACHE_DATA_ECC	BIT(3)
#define CXL_RAS_UC_MEM_DATA_PARITY	BIT(4)
#define CXL_RAS_UC_MEM_ADDR_PARITY	BIT(5)
#define CXL_RAS_UC_MEM_BE_PARITY	BIT(6)
#define CXL_RAS_UC_MEM_DATA_ECC		BIT(7)
#define CXL_RAS_UC_REINIT_THRESH	BIT(8)
#define CXL_RAS_UC_RSVD_ENCODE		BIT(9)
#define CXL_RAS_UC_POISON		BIT(10)
#define CXL_RAS_UC_RECV_OVERFLOW	BIT(11)
#define CXL_RAS_UC_INTERNAL_ERR		BIT(14)
#define CXL_RAS_UC_IDE_TX_ERR		BIT(15)
#define CXL_RAS_UC_IDE_RX_ERR		BIT(16)

#define show_uc_errs(status)	__print_flags(status, " | ",		  \
	{ CXL_RAS_UC_CACHE_DATA_PARITY, "Cache Data Parity Error" },	  \
	{ CXL_RAS_UC_CACHE_ADDR_PARITY, "Cache Address Parity Error" },	  \
	{ CXL_RAS_UC_CACHE_BE_PARITY, "Cache Byte Enable Parity Error" }, \
	{ CXL_RAS_UC_CACHE_DATA_ECC, "Cache Data ECC Error" },		  \
	{ CXL_RAS_UC_MEM_DATA_PARITY, "Memory Data Parity Error" },	  \
	{ CXL_RAS_UC_MEM_ADDR_PARITY, "Memory Address Parity Error" },	  \
	{ CXL_RAS_UC_MEM_BE_PARITY, "Memory Byte Enable Parity Error" },  \
	{ CXL_RAS_UC_MEM_DATA_ECC, "Memory Data ECC Error" },		  \
	{ CXL_RAS_UC_REINIT_THRESH, "REINIT Threshold Hit" },		  \
	{ CXL_RAS_UC_RSVD_ENCODE, "Received Unrecognized Encoding" },	  \
	{ CXL_RAS_UC_POISON, "Received Poison From Peer" },		  \
	{ CXL_RAS_UC_RECV_OVERFLOW, "Receiver Overflow" },		  \
	{ CXL_RAS_UC_INTERNAL_ERR, "Component Specific Error" },	  \
	{ CXL_RAS_UC_IDE_TX_ERR, "IDE Tx Error" },			  \
	{ CXL_RAS_UC_IDE_RX_ERR, "IDE Rx Error" }			  \
)

TRACE_EVENT(cxl_aer_uncorrectable_error,
	TP_PROTO(const struct device *dev, u32 status, u32 fe, u32 *hl),
	TP_ARGS(dev, status, fe, hl),
	TP_STRUCT__entry(
		__string(dev_name, dev_name(dev))
		__field(u32, status)
		__field(u32, first_error)
		__array(u32, header_log, CXL_HEADERLOG_SIZE_U32)
	),
	TP_fast_assign(
		__assign_str(dev_name, dev_name(dev));
		__entry->status = status;
		__entry->first_error = fe;
		/*
		 * Embed the 512B headerlog data for user app retrieval and
		 * parsing, but no need to print this in the trace buffer.
		 */
		memcpy(__entry->header_log, hl, CXL_HEADERLOG_SIZE);
	),
	TP_printk("%s: status: '%s' first_error: '%s'",
		  __get_str(dev_name),
		  show_uc_errs(__entry->status),
		  show_uc_errs(__entry->first_error)
	)
);

#define CXL_RAS_CE_CACHE_DATA_ECC	BIT(0)
#define CXL_RAS_CE_MEM_DATA_ECC		BIT(1)
#define CXL_RAS_CE_CRC_THRESH		BIT(2)
#define CLX_RAS_CE_RETRY_THRESH		BIT(3)
#define CXL_RAS_CE_CACHE_POISON		BIT(4)
#define CXL_RAS_CE_MEM_POISON		BIT(5)
#define CXL_RAS_CE_PHYS_LAYER_ERR	BIT(6)

#define show_ce_errs(status)	__print_flags(status, " | ",			\
	{ CXL_RAS_CE_CACHE_DATA_ECC, "Cache Data ECC Error" },			\
	{ CXL_RAS_CE_MEM_DATA_ECC, "Memory Data ECC Error" },			\
	{ CXL_RAS_CE_CRC_THRESH, "CRC Threshold Hit" },				\
	{ CLX_RAS_CE_RETRY_THRESH, "Retry Threshold" },				\
	{ CXL_RAS_CE_CACHE_POISON, "Received Cache Poison From Peer" },		\
	{ CXL_RAS_CE_MEM_POISON, "Received Memory Poison From Peer" },		\
	{ CXL_RAS_CE_PHYS_LAYER_ERR, "Received Error From Physical Layer" }	\
)

TRACE_EVENT(cxl_aer_correctable_error,
	TP_PROTO(const struct device *dev, u32 status),
	TP_ARGS(dev, status),
	TP_STRUCT__entry(
		__string(dev_name, dev_name(dev))
		__field(u32, status)
	),
	TP_fast_assign(
		__assign_str(dev_name, dev_name(dev));
		__entry->status = status;
	),
	TP_printk("%s: status: '%s'",
		  __get_str(dev_name), show_ce_errs(__entry->status)
	)
);

#include <linux/pci-doe.h>

struct cxl_mbox_cmd;
struct pci_doe_task;
struct pci_doe_protocol;

TRACE_EVENT(pci_config_direct_write,
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
	TP_printk("bdf=%x:%x.%x addr=%x -> %x",
		__entry->bus,
		PCI_SLOT(__entry->devfn),
		PCI_FUNC(__entry->devfn),
		__entry->reg,
		__entry->value
	)
);

TRACE_EVENT(pci_config_direct_read,
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
	/* bdf addr val */
	TP_printk("bdf=%x:%x.%x addr=%x <- %x",
		__entry->bus,
		PCI_SLOT(__entry->devfn),
		PCI_FUNC(__entry->devfn),
		__entry->reg,
		__entry->value
	)
);

TRACE_EVENT(pci_config_mm_write,
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
	TP_printk("bdf=%x:%x.%x addr=%x mmio=VA:%px->PA:%lx -> %x",
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

TRACE_EVENT(pci_config_mm_read,
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
	TP_printk("bdf=%x:%x.%x addr=%x mmio=VA:%px->PA:%lx <- %x",
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
#endif /* _CXL_EVENTS_H */

/* This part must be outside protection */
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE cxl
#include <trace/define_trace.h>
