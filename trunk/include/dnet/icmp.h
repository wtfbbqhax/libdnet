/*
 * icmp.h
 *
 * Internet Control Message Protocol (RFC 792).
 *
 * Copyright (c) 2000 Dug Song <dugsong@monkey.org>
 *
 * $Id$
 */

#ifndef DNET_ICMP_H
#define DNET_ICMP_H

#define ICMP_HDR_LEN	4
#define ICMP_LEN_MIN	8		/* minimum ICMP msg size, with hdr */

struct icmp_hdr {
	u_char		icmp_type;	/* type of message, see below */
	u_char		icmp_code;	/* type sub code */
	u_short		icmp_cksum;	/* ones complement cksum of struct */
};

struct icmp_msg_echo {
	u_int32_t	icmp_id;
	u_int32_t	icmp_seq;
	/* optional data follows */
};

struct icmp_msg_unreach_frag {
	u_int16_t	icmp_void;	/* must be zero */
	u_int16_t	icmp_nextmtu;	/* MTU of next-hop network */
	/* IP header + 8 bytes of the original datagram follows */
};

struct icmp_msg_quote {
	u_int32_t	icmp_void;	/* must be zero */
#define icmp_gwaddr	icmp_void	/* router IP address to use */
#define icmp_pptr	icmp_void	/* pointer to offending octet field */
	/* IP header + 8 bytes of the original datagram follows */
};

/* RFC 1256 */
struct icmp_msg_rtradv {
	u_char		icmp_num_addrs;	/* number of address / pref pairs */
	u_char		icmp_wpa;	/* words / address - always 2 */
	u_short		icmp_lifetime;	/* route lifetime in seconds */
	/* variable number of icmp_rtradv_data structures follows */
};

struct icmp_rtradv_data {
	u_int32_t	icmp_gwaddr;	/* router IP address */
	u_int32_t	icmp_pref;	/* preference (usually zero) */
#define ICMP_RTR_PREF_NODEFAULT		0x80000000
};

struct icmp_msg_timestamp {
	u_int32_t	icmp_id;
	u_int32_t	icmp_seq;
	u_int32_t	icmp_ts_orig;
	u_int32_t	icmp_ts_rcv;
	u_int32_t	icmp_ts_tx;
};

struct icmp_msg_mask {
	u_int32_t	icmp_id;
	u_int32_t	icmp_seq;
	u_int32_t	icmp_mask;
};

union icmp_msg {
	struct icmp_msg_echo		icmp_echo;
	struct icmp_msg_quote		icmp_unreach;
	struct icmp_msg_unreach_frag	icmp_unreach_frag;
	struct icmp_msg_quote		icmp_quench;
	struct icmp_msg_quote		icmp_redirect;
	struct icmp_msg_quote		icmp_rtr;
	struct icmp_msg_rtradv		icmp_rtr_adv;
	struct icmp_msg_quote		icmp_time_exceed;
	struct icmp_msg_quote		icmp_param_prob;
	struct icmp_msg_timestamp	icmp_timestamp;
	struct icmp_msg_mask		icmp_mask;
};

/*
 * Definition of type and code field values.
 */
#define	ICMP_ECHOREPLY		0		/* echo reply */
#define	ICMP_UNREACH		3		/* dest unreachable, codes: */
#define		ICMP_UNREACH_NET		0	/* bad net */
#define		ICMP_UNREACH_HOST		1	/* bad host */
#define		ICMP_UNREACH_PROTOCOL		2	/* bad protocol */
#define		ICMP_UNREACH_PORT		3	/* bad port */
#define		ICMP_UNREACH_NEEDFRAG		4	/* IP_DF caused drop */
#define		ICMP_UNREACH_SRCFAIL		5	/* src route failed */
#define		ICMP_UNREACH_NET_UNKNOWN	6	/* unknown net */
#define		ICMP_UNREACH_HOST_UNKNOWN	7	/* unknown host */
#define		ICMP_UNREACH_ISOLATED		8	/* src host isolated */
#define		ICMP_UNREACH_NET_PROHIB		9	/* for crypto devs */
#define		ICMP_UNREACH_HOST_PROHIB	10	/* ditto */
#define		ICMP_UNREACH_TOSNET		11	/* bad tos for net */
#define		ICMP_UNREACH_TOSHOST		12	/* bad tos for host */
#define		ICMP_UNREACH_FILTER_PROHIB	13	/* prohibited access */
#define		ICMP_UNREACH_HOST_PRECEDENCE	14	/* precedence error */
#define		ICMP_UNREACH_PRECEDENCE_CUTOFF	15	/* precedence cutoff */
#define	ICMP_SOURCEQUENCH	4		/* packet lost, slow down */
#define	ICMP_REDIRECT		5		/* shorter route, codes: */
#define		ICMP_REDIRECT_NET		0	/* for network */
#define		ICMP_REDIRECT_HOST		1	/* for host */
#define		ICMP_REDIRECT_TOSNET		2	/* for tos and net */
#define		ICMP_REDIRECT_TOSHOST		3	/* for tos and host */
#define	ICMP_ECHO		8		/* echo service */
#define	ICMP_ROUTERADVERT	9		/* router advertisement */
#define	ICMP_ROUTERSOLICIT	10		/* router solicitation */
#define	ICMP_TIMXCEED		11		/* time exceeded, code: */
#define		ICMP_TIMXCEED_INTRANS		0	/* ttl==0 in transit */
#define		ICMP_TIMXCEED_REASS		1	/* ttl==0 in reass */
#define	ICMP_PARAMPROB		12		/* ip header bad */
#define		ICMP_PARAMPROB_OPTABSENT	1	/* req. opt. absent */
#define	ICMP_TSTAMP		13		/* timestamp request */
#define	ICMP_TSTAMPREPLY	14		/* timestamp reply */
#define	ICMP_IREQ		15		/* information request */
#define	ICMP_IREQREPLY	16		/* information reply */
#define	ICMP_MASKREQ	17		/* address mask request */
#define	ICMP_MASKREPLY	18		/* address mask reply */

#define	ICMP_MAXTYPE	18

#define	ICMP_INFOTYPE(type) \
	((type) == ICMP_ECHOREPLY || (type) == ICMP_ECHO || \
	(type) == ICMP_ROUTERADVERT || (type) == ICMP_ROUTERSOLICIT || \
	(type) == ICMP_TSTAMP || (type) == ICMP_TSTAMPREPLY || \
	(type) == ICMP_IREQ || (type) == ICMP_IREQREPLY || \
	(type) == ICMP_MASKREQ || (type) == ICMP_MASKREPLY)

#endif /* DNET_ICMP_H */