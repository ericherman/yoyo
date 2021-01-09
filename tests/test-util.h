/* SPDX-License-Identifier: GPL-3.0-or-later */
/* yoyo: a program to launch and re-launch another program if it hangs */
/* Copyright (C) 2020, 2021 Eric Herman <eric@freesa.org> */

#ifndef TEST_UTIL_H
#define TEST_UTIL_H 1

unsigned run_named_test(const char *name, unsigned (*func)(void));

#define run_test(func) run_named_test(#func, func)

int failures_to_status(const char *name, unsigned failures);

#endif /* #ifndef TEST_UTIL_H */
