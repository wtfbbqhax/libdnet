/*
 * fw-ipchains.c
 *
 * Copyright (c) 2001 Dug Song <dugsong@monkey.org>
 *
 * $Id$
 */

#include "config.h"

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/icmp.h>
#include <linux/if.h>
#ifdef HAVE_LINUX_IP_FW_H
#include <linux/ip_fw.h>
#elif defined(HAVE_LINUX_IP_FWCHAINS_H)
#include <linux/ip_fwchains.h>
#elif defined(HAVE_LINUX_NETFILTER_IPV4_IPCHAINS_CORE_H)
#include <linux/netfilter_ipv4/ipchains_core.h>
#endif

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dnet.h"

#define PROC_IPCHAINS_FILE	"/proc/net/ip_fwchains"

struct fw_handle {
	int	fd;
};

static void
fr_to_fwc(struct fw_rule *fr, struct ip_fwchange *fwc)
{
	memset(fwc, 0, sizeof(*fwc));

	strlcpy(fwc->fwc_rule.ipfw.fw_vianame, fr->device, IFNAMSIZ);
	
	if (fr->op == FW_OP_ALLOW)
		strlcpy(fwc->fwc_rule.label, IP_FW_LABEL_ACCEPT, 
		    sizeof(fwc->fwc_rule.label));
	else
		strlcpy(fwc->fwc_rule.label, IP_FW_LABEL_BLOCK,
		    sizeof(fwc->fwc_rule.label));

	if (fr->direction == FW_DIR_IN)
		strlcpy(fwc->fwc_label, IP_FW_LABEL_INPUT,
		    sizeof(fwc->fwc_label));
	else
		strlcpy(fwc->fwc_label, IP_FW_LABEL_OUTPUT,
		    sizeof(fwc->fwc_label));
	
	fwc->fwc_rule.ipfw.fw_proto = fr->proto;
	fwc->fwc_rule.ipfw.fw_src.s_addr = fr->src.addr_ip;
	fwc->fwc_rule.ipfw.fw_dst.s_addr = fr->dst.addr_ip;
	addr_btom(fr->src.addr_bits, &fwc->fwc_rule.ipfw.fw_smsk.s_addr,
	    IP_ADDR_LEN);
	addr_btom(fr->dst.addr_bits, &fwc->fwc_rule.ipfw.fw_dmsk.s_addr,
	    IP_ADDR_LEN);

	/* XXX - ICMP? */
	fwc->fwc_rule.ipfw.fw_spts[0] = fr->sport[0];
	fwc->fwc_rule.ipfw.fw_spts[1] = fr->sport[1];
	fwc->fwc_rule.ipfw.fw_dpts[0] = fr->dport[0];
	fwc->fwc_rule.ipfw.fw_dpts[1] = fr->dport[1];
}

static void
fwc_to_fr(struct ip_fwchange *fwc, struct fw_rule *fr)
{
	memset(fr, 0, sizeof(*fr));

	strlcpy(fr->device, fwc->fwc_rule.ipfw.fw_vianame, sizeof(fr->device));

	if (strcmp(fwc->fwc_rule.label, IP_FW_LABEL_ACCEPT) == 0)
		fr->op = FW_OP_ALLOW;
	else
		fr->op = FW_OP_BLOCK;

	if (strcmp(fwc->fwc_label, IP_FW_LABEL_INPUT) == 0)
		fr->direction = FW_DIR_IN;
	else
		fr->direction = FW_DIR_OUT;

	fr->proto = fwc->fwc_rule.ipfw.fw_proto;
	fr->src.addr_type = fr->dst.addr_type = ADDR_TYPE_IP;
	fr->src.addr_ip = fwc->fwc_rule.ipfw.fw_src.s_addr;
	fr->dst.addr_ip = fwc->fwc_rule.ipfw.fw_dst.s_addr;
	addr_mtob(&fwc->fwc_rule.ipfw.fw_smsk.s_addr, IP_ADDR_LEN,
	    &fr->src.addr_bits);
	addr_mtob(&fwc->fwc_rule.ipfw.fw_dmsk.s_addr, IP_ADDR_LEN,
	    &fr->dst.addr_bits);

	/* XXX - ICMP? */
	fr->sport[0] = fwc->fwc_rule.ipfw.fw_spts[0];
	fr->sport[1] = fwc->fwc_rule.ipfw.fw_spts[1];
	fr->dport[0] = fwc->fwc_rule.ipfw.fw_dpts[0];
	fr->dport[1] = fwc->fwc_rule.ipfw.fw_dpts[1];
}

fw_t *
fw_open(void)
{
	fw_t *fw;

	if ((fw = calloc(1, sizeof(*fw))) == NULL)
		return (NULL);

	if ((fw->fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
		free(fw);
		return (NULL);
	}
	return (fw);
}

int
fw_add(fw_t *fw, struct fw_rule *rule)
{
	struct ip_fwchange fwc;

	fr_to_fwc(rule, &fwc);
	
	return (setsockopt(fw->fd, IPPROTO_IP, IP_FW_APPEND,
	    &fwc, sizeof(fwc)));
}

int
fw_delete(fw_t *fw, struct fw_rule *rule)
{
	struct ip_fwchange fwc;

	fr_to_fwc(rule, &fwc);
	
	return (setsockopt(fw->fd, IPPROTO_IP, IP_FW_DELETE,
	    &fwc, sizeof(fwc)));
}

int
fw_loop(fw_t *fw, fw_handler callback, void *arg)
{
	FILE *fp;
	struct ip_fwchange fwc;
	struct fw_rule fr;
	char buf[BUFSIZ];
	u_int phi, plo, bhi, blo, tand, txor;
	int ret;
	
	if ((fp = fopen(PROC_IPCHAINS_FILE, "r")) == NULL)
		return (-1);

	while (fgets(buf, sizeof(buf), fp) != NULL) {
		if (sscanf(buf,
		    "%8s %X/%X->%X/%X %s %hX %hX %hu %u %u %u %u "
		    "%hu-%hu %hu-%hu A%X X%X %hX %u %hu %s\n",
		    fwc.fwc_label,
		    &fwc.fwc_rule.ipfw.fw_src.s_addr,
		    &fwc.fwc_rule.ipfw.fw_smsk.s_addr,
		    &fwc.fwc_rule.ipfw.fw_dst.s_addr,
		    &fwc.fwc_rule.ipfw.fw_dmsk.s_addr,
		    fwc.fwc_rule.ipfw.fw_vianame,
		    &fwc.fwc_rule.ipfw.fw_flg,
		    &fwc.fwc_rule.ipfw.fw_invflg,
		    &fwc.fwc_rule.ipfw.fw_proto,
		    &phi, &plo, &bhi, &blo,
		    &fwc.fwc_rule.ipfw.fw_spts[0],
		    &fwc.fwc_rule.ipfw.fw_spts[1],
		    &fwc.fwc_rule.ipfw.fw_dpts[0],
		    &fwc.fwc_rule.ipfw.fw_dpts[1],
		    &tand, &txor,
		    &fwc.fwc_rule.ipfw.fw_redirpt,
		    &fwc.fwc_rule.ipfw.fw_mark,
		    &fwc.fwc_rule.ipfw.fw_outputsize,
		    fwc.fwc_rule.label) != 23)
			break;

		if (strcmp(fwc.fwc_rule.label, IP_FW_LABEL_ACCEPT) != 0 &&
		    strcmp(fwc.fwc_rule.label, IP_FW_LABEL_BLOCK) != 0 &&
		    strcmp(fwc.fwc_rule.label, IP_FW_LABEL_REJECT) != 0)
			continue;
		if (strcmp(fwc.fwc_label, IP_FW_LABEL_INPUT) != 0 &&
		    strcmp(fwc.fwc_label, IP_FW_LABEL_OUTPUT) != 0)
			continue;
		if (strcmp(fwc.fwc_rule.label, "-") == 0)
			(fwc.fwc_rule.label)[0] = '\0';
		if (strcmp(fwc.fwc_rule.ipfw.fw_vianame, "-") == 0)
			(fwc.fwc_rule.ipfw.fw_vianame)[0] = '\0';
		fwc.fwc_rule.ipfw.fw_src.s_addr =
		    htonl(fwc.fwc_rule.ipfw.fw_src.s_addr);
		fwc.fwc_rule.ipfw.fw_dst.s_addr =
		    htonl(fwc.fwc_rule.ipfw.fw_dst.s_addr);
		fwc.fwc_rule.ipfw.fw_smsk.s_addr =
		    htonl(fwc.fwc_rule.ipfw.fw_smsk.s_addr);
		fwc.fwc_rule.ipfw.fw_dmsk.s_addr =
		    htonl(fwc.fwc_rule.ipfw.fw_dmsk.s_addr);
		
		fwc_to_fr(&fwc, &fr);
		
		if ((ret = callback(&fr, arg)) != 0) {
			fclose(fp);
			return (ret);
		}
	}
	fclose(fp);
	
	return (0);
}

int
fw_close(fw_t *fw)
{
	assert(fw != NULL);

	if (close(fw->fd) < 0)
		return (-1);
	
	free(fw);
	return (0);
}
