/*
 * kg2_netlink.c - Linux Driver for Marvell 88DE2750 Digital Video Format Converter
 *
 *                 This file handles Generic Netlink communication, which is used to
 *                 send KG2 configuration request to the user space daemon - kg2d.
 */

#include "kg2_netlink.h"


// Generic netlink operation handlers
static int kg2_genl_get_daemon_pid(struct sk_buff * skb, struct genl_info * info);
static int kg2_genl_set_daemon_pid(struct sk_buff * skb, struct genl_info * info);

// Generic netlink family definition
static struct genl_family kg2_genl_family = {
	.id		= GENL_ID_GENERATE,
	.hdrsize	= 0,
	.name		= KG2_GENL_NAME,
	.version	= KG2_GENL_VERSION,
	.maxattr	= KG2_GENL_ATTR_MAX,
};

// Generic netlink attribute policy
static struct nla_policy kg2_genl_policy[KG2_GENL_ATTR_MAX + 1] = {
	[KG2_GENL_ATTR_PID] = {
		.type		= NLA_U32,
	},
	[KG2_GENL_ATTR_HTOTAL] = {
		.type		= NLA_U16,
	},
	[KG2_GENL_ATTR_HACTIVE] = {
		.type		= NLA_U16,
	},
	[KG2_GENL_ATTR_HFRONTPORCH] = {
		.type		= NLA_U16,
	},
	[KG2_GENL_ATTR_HSYNCWIDTH] = {
		.type		= NLA_U8,
	},
	[KG2_GENL_ATTR_HPOLARITY] = {
		.type		= NLA_U32,
	},
	[KG2_GENL_ATTR_VTOTAL] = {
		.type		= NLA_U16,
	},
	[KG2_GENL_ATTR_VACTIVE] = {
		.type		= NLA_U16,
	},
	[KG2_GENL_ATTR_VFRONTPORCH] = {
		.type		= NLA_U16,
	},
	[KG2_GENL_ATTR_VSYNCWIDTH] = {
		.type		= NLA_U8,
	},
	[KG2_GENL_ATTR_VPOLARITY] = {
		.type		= NLA_U32,
	},
	[KG2_GENL_ATTR_ASPRATIO] = {
		.type		= NLA_U32,
	},
	[KG2_GENL_ATTR_ISPROGRESSIVE] = {
		.type		= NLA_U8,
	},
	[KG2_GENL_ATTR_REFRATE] = {
		.type		= NLA_U16,
	},
};

// Generic netlink operation definition
static struct genl_ops kg2_genl_ops[] = {
	{
		.cmd		= KG2_GENL_CMD_GET_DAEMON_PID,
		.flags		= 0,
		.policy		= kg2_genl_policy,
		.doit		= kg2_genl_get_daemon_pid,
		.dumpit		= NULL,
		.done		= NULL,
	},
	{
		.cmd		= KG2_GENL_CMD_SET_DAEMON_PID,
		.flags		= 0,
		.policy		= kg2_genl_policy,
		.doit		= kg2_genl_set_daemon_pid,
		.dumpit		= NULL,
		.done		= NULL,
	}
};

unsigned int g_daemon_pid = 0;


/*-----------------------------------------------------------------------------
 * Description : Generic Netlink operation handler for KG2 family
 * Parameter   : skb
 *               info
 * Return      : =0 - Success
 *-----------------------------------------------------------------------------
 */

static int kg2_genl_get_daemon_pid(struct sk_buff * skb, struct genl_info * info)
{
	int ret = 0;
	void * hdr;
	struct sk_buff * msg;

	printk(KERN_INFO "kg2: Send daemon PID to user: %u\n", info->snd_pid);

	if (g_daemon_pid == 0)
		return -EINVAL;

	// Allocate message
	msg = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);

	if (msg == NULL)
		return -ENOMEM;

	// Set message header
	hdr = genlmsg_put(msg, info->snd_pid, info->snd_seq, &kg2_genl_family,
	                  0, KG2_GENL_CMD_SET_DAEMON_PID);

	if (hdr == NULL)
	{
		ret = -EMSGSIZE;
		goto err;
	}

	// Set message payload
	ret = nla_put_u32(msg, KG2_GENL_ATTR_PID, g_daemon_pid);

	if (ret)
	{
		ret = -ENOBUFS;
		goto err;
	}

	// Finalize message
	genlmsg_end(msg, hdr);

	// Reply message
	return genlmsg_reply(msg, info);
 err:
	nlmsg_free(msg);

	return ret;
}

/*-----------------------------------------------------------------------------
 * Description : Generic Netlink operation handler for KG2 family
 * Parameter   : skb
 *               info
 * Return      : =0 - Success
 *-----------------------------------------------------------------------------
 */

static int kg2_genl_set_daemon_pid(struct sk_buff * skb, struct genl_info * info)
{
	printk(KERN_INFO "kg2: Receive daemon PID: %u\n", info->snd_pid);

	g_daemon_pid = info->snd_pid;

	return 0;
}

/*-----------------------------------------------------------------------------
 * Description : Check if the daemon PID is available
 * Parameter   : N/A
 * Return      : =true - Available 
 *             : =false - Not available
 *-----------------------------------------------------------------------------
 */

bool kg2_genl_daemon_pid_available(void)
{
	if (g_daemon_pid > 0)
		return true;

	return false;
}

/*-----------------------------------------------------------------------------
 * Description : Generic Netlink operation handler for KG2 family
 * Parameter   : timing
 * Return      : =0 - Success
 *-----------------------------------------------------------------------------
 */

int kg2_genl_set_input_timing(AVC_CMD_TIMING_PARAM * timing)
{
	int ret = 0;
	void * hdr;
	struct sk_buff * msg;

	msg = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);

	if (msg == NULL)
	{
		return -ENOMEM;
	}

	// Create Generic Netlink message header
	hdr = genlmsg_put(msg, g_daemon_pid, 0, &kg2_genl_family, NLM_F_REQUEST, KG2_GENL_CMD_SET_INPUT_TIMING);

	if (hdr == NULL)
	{
		ret = -EMSGSIZE;
		goto err;
	}

	// Add a KG2_GENL_ATTR_MSG attribute
	ret = nla_put_u16(msg, KG2_GENL_ATTR_HTOTAL, timing->HTotal);

	if (ret)
	{
		ret = -ENOBUFS;
		goto err;
	}

	ret = nla_put_u16(msg, KG2_GENL_ATTR_HACTIVE, timing->HActive);

	if (ret)
	{
		ret = -ENOBUFS;
		goto err;
	}

	ret = nla_put_u16(msg, KG2_GENL_ATTR_HFRONTPORCH, timing->HFrontPorch);

	if (ret)
	{
		ret = -ENOBUFS;
		goto err;
	}

	ret = nla_put_u8(msg, KG2_GENL_ATTR_HSYNCWIDTH, timing->HSyncWidth);

	if (ret)
	{
		ret = -ENOBUFS;
		goto err;
	}

	ret = nla_put_u32(msg, KG2_GENL_ATTR_HPOLARITY, timing->HPolarity);

	if (ret)
	{
		ret = -ENOBUFS;
		goto err;
	}

	ret = nla_put_u16(msg, KG2_GENL_ATTR_VTOTAL, timing->VTotal);

	if (ret)
	{
		ret = -ENOBUFS;
		goto err;
	}

	ret = nla_put_u16(msg, KG2_GENL_ATTR_VACTIVE, timing->VActive);

	if (ret)
	{
		ret = -ENOBUFS;
		goto err;
	}

	ret = nla_put_u16(msg, KG2_GENL_ATTR_VFRONTPORCH, timing->VFrontPorch);

	if (ret)
	{
		ret = -ENOBUFS;
		goto err;
	}

	ret = nla_put_u8(msg, KG2_GENL_ATTR_VSYNCWIDTH, timing->VSyncWidth);

	if (ret)
	{
		ret = -ENOBUFS;
		goto err;
	}

	ret = nla_put_u32(msg, KG2_GENL_ATTR_VPOLARITY, timing->VPolarity);

	if (ret)
	{
		ret = -ENOBUFS;
		goto err;
	}

	ret = nla_put_u32(msg, KG2_GENL_ATTR_ASPRATIO, timing->AspRatio);

	if (ret)
	{
		ret = -ENOBUFS;
		goto err;
	}

	ret = nla_put_u8(msg, KG2_GENL_ATTR_ISPROGRESSIVE, timing->IsProgressive);

	if (ret)
	{
		ret = -ENOBUFS;
		goto err;
	}

	ret = nla_put_u16(msg, KG2_GENL_ATTR_REFRATE, timing->RefRate);

	if (ret)
	{
		ret = -ENOBUFS;
		goto err;
	}

	// Finalize the message
	genlmsg_end(msg, hdr);

	// Send the message
	ret = genlmsg_unicast(&init_net, msg, g_daemon_pid);

	return ret;
 err:
	nlmsg_free(msg);

	return ret;
}

/*-----------------------------------------------------------------------------
 * Description : Generic Netlink operation handler for KG2 family
 * Parameter   : timing
 * Return      : =0 - Success
 *-----------------------------------------------------------------------------
 */

int kg2_genl_set_output_timing(AVC_CMD_TIMING_PARAM * timing)
{
	int ret = 0;
	void * hdr;
	struct sk_buff * msg;

	msg = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);

	if (msg == NULL)
	{
		return -ENOMEM;
	}

	// Create Generic Netlink message header
	hdr = genlmsg_put(msg, g_daemon_pid, 0, &kg2_genl_family, NLM_F_REQUEST, KG2_GENL_CMD_SET_OUTPUT_TIMING);

	if (hdr == NULL)
	{
		ret = -EMSGSIZE;
		goto err;
	}

	// Add a KG2_GENL_ATTR_MSG attribute
	ret = nla_put_u16(msg, KG2_GENL_ATTR_HTOTAL, timing->HTotal);

	if (ret)
	{
		ret = -ENOBUFS;
		goto err;
	}

	ret = nla_put_u16(msg, KG2_GENL_ATTR_HACTIVE, timing->HActive);

	if (ret)
	{
		ret = -ENOBUFS;
		goto err;
	}

	ret = nla_put_u16(msg, KG2_GENL_ATTR_HFRONTPORCH, timing->HFrontPorch);

	if (ret)
	{
		ret = -ENOBUFS;
		goto err;
	}

	ret = nla_put_u8(msg, KG2_GENL_ATTR_HSYNCWIDTH, timing->HSyncWidth);

	if (ret)
	{
		ret = -ENOBUFS;
		goto err;
	}

	ret = nla_put_u32(msg, KG2_GENL_ATTR_HPOLARITY, timing->HPolarity);

	if (ret)
	{
		ret = -ENOBUFS;
		goto err;
	}

	ret = nla_put_u16(msg, KG2_GENL_ATTR_VTOTAL, timing->VTotal);

	if (ret)
	{
		ret = -ENOBUFS;
		goto err;
	}

	ret = nla_put_u16(msg, KG2_GENL_ATTR_VACTIVE, timing->VActive);

	if (ret)
	{
		ret = -ENOBUFS;
		goto err;
	}

	ret = nla_put_u16(msg, KG2_GENL_ATTR_VFRONTPORCH, timing->VFrontPorch);

	if (ret)
	{
		ret = -ENOBUFS;
		goto err;
	}

	ret = nla_put_u8(msg, KG2_GENL_ATTR_VSYNCWIDTH, timing->VSyncWidth);

	if (ret)
	{
		ret = -ENOBUFS;
		goto err;
	}

	ret = nla_put_u32(msg, KG2_GENL_ATTR_VPOLARITY, timing->VPolarity);

	if (ret)
	{
		ret = -ENOBUFS;
		goto err;
	}

	ret = nla_put_u32(msg, KG2_GENL_ATTR_ASPRATIO, timing->AspRatio);

	if (ret)
	{
		ret = -ENOBUFS;
		goto err;
	}

	ret = nla_put_u8(msg, KG2_GENL_ATTR_ISPROGRESSIVE, timing->IsProgressive);

	if (ret)
	{
		ret = -ENOBUFS;
		goto err;
	}

	ret = nla_put_u16(msg, KG2_GENL_ATTR_REFRATE, timing->RefRate);

	if (ret)
	{
		ret = -ENOBUFS;
		goto err;
	}

	// Finalize the message
	genlmsg_end(msg, hdr);

	// Send the message
	ret = genlmsg_unicast(&init_net, msg, g_daemon_pid);

	return ret;
 err:
	nlmsg_free(msg);

	return ret;
}

/*-----------------------------------------------------------------------------
 * Description : Generic Netlink operation handler for KG2 family
 * Parameter   : N/A
 * Return      : =0 - Success
 *-----------------------------------------------------------------------------
 */

int kg2_genl_system_is_resumed(void)
{
	int ret = 0;
	void * hdr;
	struct sk_buff * msg;

	msg = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);

	if (msg == NULL)
	{
		return -ENOMEM;
	}

	// Create Generic Netlink message header
	hdr = genlmsg_put(msg, g_daemon_pid, 0, &kg2_genl_family, NLM_F_REQUEST, KG2_GENL_EVT_SYSTEM_IS_RESUMED);

	if (hdr == NULL)
	{
		ret = -EMSGSIZE;
		goto err;
	}

	// Finalize the message
	genlmsg_end(msg, hdr);

	// Send the message
	ret = genlmsg_unicast(&init_net, msg, g_daemon_pid);

	return ret;
 err:
	nlmsg_free(msg);

	return ret;
}

/*-----------------------------------------------------------------------------
 * Description : Register KG2 to Generic Netlink subsystem
 * Parameter   : N/A
 * Return      : =0 - Success
 *               <0 - Fail
 *-----------------------------------------------------------------------------
 */

int kg2_genl_register(void)
{
	return genl_register_family_with_ops(&kg2_genl_family,
	                                     kg2_genl_ops,
	                                     ARRAY_SIZE(kg2_genl_ops));
}

/*-----------------------------------------------------------------------------
 * Description : Unregister KG2 from Generic Netlink subsystem
 * Parameter   : N/A
 * Return      : =0 - Success
 *               <0 - Fail
 *-----------------------------------------------------------------------------
 */

int kg2_genl_unregister(void)
{
	return genl_unregister_family(&kg2_genl_family);
}

