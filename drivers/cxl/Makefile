# SPDX-License-Identifier: GPL-2.0
obj-y += core/
obj-$(CONFIG_CXL_PCI) += cxl_pci.o
obj-$(CONFIG_CXL_MEM) += cxl_mem.o
obj-$(CONFIG_CXL_ACPI) += cxl_acpi.o
obj-$(CONFIG_CXL_PMEM) += cxl_pmem.o
obj-$(CONFIG_CXL_PORT) += cxl_port.o
obj-y += cxl_dvsec.o

cxl_mem-y := mem.o
cxl_pci-y := pci.o
cxl_acpi-y := acpi.o
cxl_pmem-y := pmem.o
cxl_port-y := port.o
cxl_dvsec-y := dvsec.o
