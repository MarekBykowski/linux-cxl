#!/bin/bash

# Build the kernel with CONFIG_GDB_SCRIPTS enabled, but leave CONFIG_DEBUG_INFO_REDUCED off.
# Add “nokaslr” to the kernel command line
exec sh -c 'gdb -ex "target extended-remote :10000" -ex "b boot_cpu_init" -ex "c" -ex "lx-symbols" vmlinux'
