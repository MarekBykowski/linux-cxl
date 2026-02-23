/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2026 Marek Bykowski <marek.bykowski@gmail.com>
 */

#ifndef _DOE_REDIRECT_H
#define _DOE_REDIRCT_H

extern int (*redirect_hook)(struct pci_doe_mb *, struct pci_doe_task *);

#if CONFIG_DOE_REDIRECT
#else
int (*redirect_hook)(struct pci_doe_mb *,
		struct pci_doe_task *) = NULL;
#endif

#endif
