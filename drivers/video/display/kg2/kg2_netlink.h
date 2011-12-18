/*
 * kg2_netlink.h - Linux Driver for Marvell 88DE2750 Digital Video Format Converter
 *
 *                 This file handles Generic Netlink communication, which is used to
 *                 send KG2 configuration request to the user space daemon - kg2d.
 */

#ifndef _KG2_NETLINK_H
#define _KG2_NETLINK_H

#include <net/genetlink.h>
#include <video/kg2.h>


#define KG2_GENL_NAME		"KG2"
#define KG2_GENL_VERSION	1

// Generic netlink attributes
enum kg2_genl_attributes {
	KG2_GENL_ATTR_UNSPEC,
	KG2_GENL_ATTR_PID,
	KG2_GENL_ATTR_HTOTAL,
	KG2_GENL_ATTR_HACTIVE,
	KG2_GENL_ATTR_HFRONTPORCH,
	KG2_GENL_ATTR_HSYNCWIDTH,
	KG2_GENL_ATTR_HPOLARITY,
	KG2_GENL_ATTR_VTOTAL,
	KG2_GENL_ATTR_VACTIVE,
	KG2_GENL_ATTR_VFRONTPORCH,
	KG2_GENL_ATTR_VSYNCWIDTH,
	KG2_GENL_ATTR_VPOLARITY,
	KG2_GENL_ATTR_ASPRATIO,
	KG2_GENL_ATTR_ISPROGRESSIVE,
	KG2_GENL_ATTR_REFRATE,
	__KG2_GENL_ATTR_MAX,
};
#define KG2_GENL_ATTR_MAX	(__KG2_GENL_ATTR_MAX - 1)

// Generic netlink commands
enum kg2_genl_commands {
	KG2_GENL_CMD_UNSPEC,
	KG2_GENL_CMD_SET_DAEMON_PID,
	KG2_GENL_CMD_SET_INPUT_TIMING,
	KG2_GENL_CMD_SET_OUTPUT_TIMING,
	KG2_GENL_EVT_SYSTEM_IS_RESUMED,
	KG2_GENL_CMD_GET_DAEMON_PID,
	KG2_GENL_CMD_GET_INPUT_TIMING,
	KG2_GENL_CMD_GET_OUTPUT_TIMING,
	__KG2_GENL_CMD_MAX,
};
#define KG2_GENL_CMD_MAX	(__KG2_GENL_CMD_MAX - 1)

/* Generic netlink functions */
bool kg2_genl_daemon_pid_available(void);
int kg2_genl_set_input_timing(AVC_CMD_TIMING_PARAM * timing);
int kg2_genl_set_output_timing(AVC_CMD_TIMING_PARAM * timing);
int kg2_genl_system_is_resumed(void);
int kg2_genl_register(void);
int kg2_genl_unregister(void);


#endif /* _KG2_NETLINK_H */

