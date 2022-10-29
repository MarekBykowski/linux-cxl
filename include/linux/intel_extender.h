// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2021 INTEL

#ifndef _INTEL_EXTENDER_H_
#define _INTEL_EXTENDER_H_

#if defined(DEBUG) && 0
#include <asm/stacktrace.h> /* For struct stackframe */

/* args counting based on include/linux/arm-smccc.h by Marc Zyngier */
#define _count_args(_0, _1, _2, _3, _4, _5, _6, _7, x, ...) x
#define count_args(...)						\
	_count_args(__VA_ARGS__, 7, 6, 5, 4, 3, 2, 1, 0)

#define extender_trace_call(frames, ...)	\
	do {		\
		if (count_args(__VA_ARGS__) == 1)	\
			_extender_trace_call_fmt(frames, "");	\
		_extender_trace_call_fmt(frames, __VA_ARGS__);	\
	} while (0)

#define _extender_trace_call_fmt(frames, fmt, ...)	\
	do {	\
		struct stackframe frame;	\
		struct task_struct *tsk = current;	\
		int i;	\
	\
		start_backtrace(&frame,	\
				(unsigned long)__builtin_frame_address(0),	\
				(unsigned long)_THIS_IP_);	\
	\
		pr_debug("extender trace call:\n");	\
		pr_debug("1: %pS " fmt, (void *)frame.pc, ##__VA_ARGS__);	\
		unwind_frame(tsk, &frame);	\
	\
		for (i = 2; i <= frames; i++) {	\
			pr_debug("%d: %pS\n", i, (void *)frame.pc);	\
			unwind_frame(tsk, &frame);	\
		}	\
	} while (0)
ssss
#else
#define extender_trace_call(frames, ...)	do {} while (0)
#endif

#endif /*_INTEL_EXTENDER_H_*/
