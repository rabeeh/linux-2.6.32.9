/*
 * user.c - the user space idle governor
 *
 * Copyright (C) 2006-2007 Tawfik Bayouk <tawfik@marvell.com>
 *
 * This code is licenced under the GPL.
 */

#include <linux/kernel.h>
#include <linux/cpuidle.h>
#include <linux/pm_qos_params.h>
#include <linux/time.h>
#include <linux/ktime.h>
#include <linux/hrtimer.h>
#include <linux/tick.h>

extern unsigned cpuidle_user_state;


/**
 * user_select - selects the next idle state to enter
 * @dev: the CPU
 */
static int user_select(struct cpuidle_device *dev)
{
	return cpuidle_user_state;
}

/**
 * user_reflect - attempts to guess what happened after entry
 * @dev: the CPU
 *
 * NOTE: it's important to be fast here because this operation will add to
 *       the overall exit latency.
 */
static void user_reflect(struct cpuidle_device *dev)
{

}

/**
 * user_enable_device - scans a CPU's states and does setup
 * @dev: the CPU
 */
static int user_enable_device(struct cpuidle_device *dev)
{
	return 0;
}

static struct cpuidle_governor user_governor = {
	.name =		"user",
	.rating =	1,
	.enable =	user_enable_device,
	.select =	user_select,
	.reflect =	user_reflect,
	.owner =	THIS_MODULE,
};

/**
 * init_user - initializes the governor
 */
static int __init init_user(void)
{
	return cpuidle_register_governor(&user_governor);
}

/**
 * exit_user - exits the governor
 */
static void __exit exit_user(void)
{
	cpuidle_unregister_governor(&user_governor);
}

MODULE_LICENSE("GPL");
module_init(init_user);
module_exit(exit_user);
