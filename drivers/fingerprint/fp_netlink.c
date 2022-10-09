/*
 * netlink interface
 *
 * Copyright (c) 2021 vivo
 */
#define pr_fmt(fmt)		"[FP_KERN] " KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/module.h>
#include <linux/timer.h>
#include <linux/time.h>
#include <linux/types.h>
#include <net/sock.h>
#include <net/netlink.h>

#define NETLINK_TEST 30
#define MAX_MSGSIZE 256

static int pid = -1;
struct sock *fp_nl_sk;

void fp_sendmsg(void *message, int message_len)
{
	struct sk_buff *skb_1;
	struct nlmsghdr *nlh;
	int len = NLMSG_SPACE(MAX_MSGSIZE);
	int ret = 0;

	if (!message || !fp_nl_sk || !pid) {
		return;
	}

	if (message_len > MAX_MSGSIZE) {
		pr_err("%s, message_len(%d) is bigger than maxsize(%d)", __func__, message_len, MAX_MSGSIZE);
		return;
	}

	skb_1 = alloc_skb(len, GFP_KERNEL);
	if (!skb_1) {
		pr_err("%s, alloc_skb error", __func__);
		return;
	}
	nlh = nlmsg_put(skb_1, 0, 0, 0, MAX_MSGSIZE, 0);

	NETLINK_CB(skb_1).portid = 0;
	NETLINK_CB(skb_1).dst_group = 0;

	memcpy(NLMSG_DATA(nlh), message, message_len);

	pr_info("%s, msg send", __func__);
	ret = netlink_unicast(fp_nl_sk, skb_1, pid, MSG_DONTWAIT);
	if (!ret) {
		/*kfree_skb(skb_1); */
		pr_err("%s, send msg from kernel to usespace failed ret 0x%x", __func__, ret);
	}
}

void fp_nl_data_ready(struct sk_buff *__skb)
{
	struct sk_buff *skb;
	struct nlmsghdr *nlh;
	char str[256];

	skb = skb_get(__skb);
	if (skb->len >= NLMSG_SPACE(0)) {
		nlh = nlmsg_hdr(skb);
		memcpy(str, NLMSG_DATA(nlh), sizeof(str));
		pid = nlh->nlmsg_pid;
		pr_err("%s, resv msg from usespace str:%s pid:%d", __func__, str, pid);
		kfree_skb(skb);
	}

}

int fp_netlink_init(void)
{
	struct netlink_kernel_cfg netlink_cfg;

	memset(&netlink_cfg, 0, sizeof(struct netlink_kernel_cfg));

	netlink_cfg.groups = 0;
	netlink_cfg.flags = 0;
	netlink_cfg.input = fp_nl_data_ready;
	netlink_cfg.cb_mutex = NULL;

	fp_nl_sk = netlink_kernel_create(&init_net, NETLINK_TEST,
			&netlink_cfg);

	if (!fp_nl_sk) {
		pr_err("%s, create netlink socket error", __func__);
		return 1;
	}

	return 0;
}

void fp_netlink_exit(void)
{
	if (fp_nl_sk != NULL) {
		netlink_kernel_release(fp_nl_sk);
		fp_nl_sk = NULL;
	}

	pr_info("%s, self module exited", __func__);
}
