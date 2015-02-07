

#ifndef _ASM_X86_UV_UV_BAU_H
#define _ASM_X86_UV_UV_BAU_H

#include <linux/bitmap.h>
#define BITSPERBYTE 8


#define UV_ITEMS_PER_DESCRIPTOR		8
#define MAX_BAU_CONCURRENT		3
#define UV_CPUS_PER_ACT_STATUS		32
#define UV_ACT_STATUS_MASK		0x3
#define UV_ACT_STATUS_SIZE		2
#define UV_ADP_SIZE			32
#define UV_DISTRIBUTION_SIZE		256
#define UV_SW_ACK_NPENDING		8
#define UV_NET_ENDPOINT_INTD		0x38
#define UV_DESC_BASE_PNODE_SHIFT	49
#define UV_PAYLOADQ_PNODE_SHIFT		49
#define UV_PTC_BASENAME			"sgi_uv/ptc_statistics"
#define uv_physnodeaddr(x)		((__pa((unsigned long)(x)) & uv_mmask))
#define UV_ENABLE_INTD_SOFT_ACK_MODE_SHIFT 15
#define UV_INTD_SOFT_ACK_TIMEOUT_PERIOD_SHIFT 16
#define UV_INTD_SOFT_ACK_TIMEOUT_PERIOD 0x000000000bUL

#define DESC_STATUS_IDLE		0
#define DESC_STATUS_ACTIVE		1
#define DESC_STATUS_DESTINATION_TIMEOUT	2
#define DESC_STATUS_SOURCE_TIMEOUT	3

#define SOURCE_TIMEOUT_LIMIT		20
#define DESTINATION_TIMEOUT_LIMIT	20

#define THROTTLE_DELAY			10
#define TIMEOUT_DELAY			10
#define BIOS_TO				1000
/* BIOS is assumed to set the destination timeout to 1003520 nanoseconds */

#define PLUGSB4RESET 100
#define TIMEOUTSB4RESET 100

#define DEST_Q_SIZE			20
#define DEST_NUM_RESOURCES		8
#define MAX_CPUS_PER_NODE		32
#define FLUSH_RETRY_PLUGGED		1
#define FLUSH_RETRY_TIMEOUT		2
#define FLUSH_GIVEUP			3
#define FLUSH_COMPLETE			4

struct bau_target_uvhubmask {
	unsigned long bits[BITS_TO_LONGS(UV_DISTRIBUTION_SIZE)];
};

struct bau_local_cpumask {
	unsigned long bits;
};


struct bau_msg_payload {
	unsigned long address;		/* signifies a page or all TLB's
						of the cpu */
	/* 64 bits */
	unsigned short sending_cpu;	/* filled in by sender */
	/* 16 bits */
	unsigned short acknowledge_count;/* filled in by destination */
	/* 16 bits */
	unsigned int reserved1:32;	/* not usable */
};


struct bau_msg_header {
	unsigned int dest_subnodeid:6;	/* must be 0x10, for the LB */
	/* bits 5:0 */
	unsigned int base_dest_nodeid:15; /* nasid (pnode<<1) of */
	/* bits 20:6 */			  /* first bit in uvhub map */
	unsigned int command:8;	/* message type */
	/* bits 28:21 */
				/* 0x38: SN3net EndPoint Message */
	unsigned int rsvd_1:3;	/* must be zero */
	/* bits 31:29 */
				/* int will align on 32 bits */
	unsigned int rsvd_2:9;	/* must be zero */
	/* bits 40:32 */
				/* Suppl_A is 56-41 */
	unsigned int sequence:16;/* message sequence number */
	/* bits 56:41 */	/* becomes bytes 16-17 of msg */
				/* Address field (96:57) is never used as an
				   address (these are address bits 42:3) */

	unsigned int rsvd_3:1;	/* must be zero */
	/* bit 57 */
				/* address bits 27:4 are payload */
	/* these next 24  (58-81) bits become bytes 12-14 of msg */

	/* bits 65:58 land in byte 12 */
	unsigned int replied_to:1;/* sent as 0 by the source to byte 12 */
	/* bit 58 */
	unsigned int msg_type:3; /* software type of the message*/
	/* bits 61:59 */
	unsigned int canceled:1; /* message canceled, resource to be freed*/
	/* bit 62 */
	unsigned int payload_1a:1;/* not currently used */
	/* bit 63 */
	unsigned int payload_1b:2;/* not currently used */
	/* bits 65:64 */

	/* bits 73:66 land in byte 13 */
	unsigned int payload_1ca:6;/* not currently used */
	/* bits 71:66 */
	unsigned int payload_1c:2;/* not currently used */
	/* bits 73:72 */

	/* bits 81:74 land in byte 14 */
	unsigned int payload_1d:6;/* not currently used */
	/* bits 79:74 */
	unsigned int payload_1e:2;/* not currently used */
	/* bits 81:80 */

	unsigned int rsvd_4:7;	/* must be zero */
	/* bits 88:82 */
	unsigned int sw_ack_flag:1;/* software acknowledge flag */
	/* bit 89 */
				/* INTD trasactions at destination are to
				   wait for software acknowledge */
	unsigned int rsvd_5:6;	/* must be zero */
	/* bits 95:90 */
	unsigned int rsvd_6:5;	/* must be zero */
	/* bits 100:96 */
	unsigned int int_both:1;/* if 1, interrupt both sockets on the uvhub */
	/* bit 101*/
	unsigned int fairness:3;/* usually zero */
	/* bits 104:102 */
	unsigned int multilevel:1;	/* multi-level multicast format */
	/* bit 105 */
				/* 0 for TLB: endpoint multi-unicast messages */
	unsigned int chaining:1;/* next descriptor is part of this activation*/
	/* bit 106 */
	unsigned int rsvd_7:21;	/* must be zero */
	/* bits 127:107 */
};

/* see msg_type: */
#define MSG_NOOP 0
#define MSG_REGULAR 1
#define MSG_RETRY 2

struct bau_desc {
	struct bau_target_uvhubmask distribution;
	/*
	 * message template, consisting of header and payload:
	 */
	struct bau_msg_header header;
	struct bau_msg_payload payload;
};

struct bau_payload_queue_entry {
	unsigned long address;		/* signifies a page or all TLB's
						of the cpu */
	/* 64 bits, bytes 0-7 */

	unsigned short sending_cpu;	/* cpu that sent the message */
	/* 16 bits, bytes 8-9 */

	unsigned short acknowledge_count; /* filled in by destination */
	/* 16 bits, bytes 10-11 */

	/* these next 3 bytes come from bits 58-81 of the message header */
	unsigned short replied_to:1;    /* sent as 0 by the source */
	unsigned short msg_type:3;      /* software message type */
	unsigned short canceled:1;      /* sent as 0 by the source */
	unsigned short unused1:3;       /* not currently using */
	/* byte 12 */

	unsigned char unused2a;		/* not currently using */
	/* byte 13 */
	unsigned char unused2;		/* not currently using */
	/* byte 14 */

	unsigned char sw_ack_vector;	/* filled in by the hardware */
	/* byte 15 (bits 127:120) */

	unsigned short sequence;	/* message sequence number */
	/* bytes 16-17 */
	unsigned char unused4[2];	/* not currently using bytes 18-19 */
	/* bytes 18-19 */

	int number_of_cpus;		/* filled in at destination */
	/* 32 bits, bytes 20-23 (aligned) */

	unsigned char unused5[8];       /* not using */
	/* bytes 24-31 */
};

struct bau_control {
	struct bau_desc *descriptor_base;
	struct bau_payload_queue_entry *va_queue_first;
	struct bau_payload_queue_entry *va_queue_last;
	struct bau_payload_queue_entry *bau_msg_head;
	struct bau_control *uvhub_master;
	struct bau_control *socket_master;
	unsigned long timeout_interval;
	atomic_t active_descriptor_count;
	int max_concurrent;
	int max_concurrent_constant;
	int retry_message_scans;
	int plugged_tries;
	int timeout_tries;
	int ipi_attempts;
	int conseccompletes;
	short cpu;
	short uvhub_cpu;
	short uvhub;
	short cpus_in_socket;
	short cpus_in_uvhub;
	unsigned short message_number;
	unsigned short uvhub_quiesce;
	short socket_acknowledge_count[DEST_Q_SIZE];
	cycles_t send_message;
	spinlock_t masks_lock;
	spinlock_t uvhub_lock;
	spinlock_t queue_lock;
};

struct ptc_stats {
	/* sender statistics */
	unsigned long s_giveup; /* number of fall backs to IPI-style flushes */
	unsigned long s_requestor; /* number of shootdown requests */
	unsigned long s_stimeout; /* source side timeouts */
	unsigned long s_dtimeout; /* destination side timeouts */
	unsigned long s_time; /* time spent in sending side */
	unsigned long s_retriesok; /* successful retries */
	unsigned long s_ntargcpu; /* number of cpus targeted */
	unsigned long s_ntarguvhub; /* number of uvhubs targeted */
	unsigned long s_ntarguvhub16; /* number of times >= 16 target hubs */
	unsigned long s_ntarguvhub8; /* number of times >= 8 target hubs */
	unsigned long s_ntarguvhub4; /* number of times >= 4 target hubs */
	unsigned long s_ntarguvhub2; /* number of times >= 2 target hubs */
	unsigned long s_ntarguvhub1; /* number of times == 1 target hub */
	unsigned long s_resets_plug; /* ipi-style resets from plug state */
	unsigned long s_resets_timeout; /* ipi-style resets from timeouts */
	unsigned long s_busy; /* status stayed busy past s/w timer */
	unsigned long s_throttles; /* waits in throttle */
	unsigned long s_retry_messages; /* retry broadcasts */
	/* destination statistics */
	unsigned long d_alltlb; /* times all tlb's on this cpu were flushed */
	unsigned long d_onetlb; /* times just one tlb on this cpu was flushed */
	unsigned long d_multmsg; /* interrupts with multiple messages */
	unsigned long d_nomsg; /* interrupts with no message */
	unsigned long d_time; /* time spent on destination side */
	unsigned long d_requestee; /* number of messages processed */
	unsigned long d_retries; /* number of retry messages processed */
	unsigned long d_canceled; /* number of messages canceled by retries */
	unsigned long d_nocanceled; /* retries that found nothing to cancel */
	unsigned long d_resets; /* number of ipi-style requests processed */
	unsigned long d_rcanceled; /* number of messages canceled by resets */
};

static inline int bau_uvhub_isset(int uvhub, struct bau_target_uvhubmask *dstp)
{
	return constant_test_bit(uvhub, &dstp->bits[0]);
}
static inline void bau_uvhub_set(int uvhub, struct bau_target_uvhubmask *dstp)
{
	__set_bit(uvhub, &dstp->bits[0]);
}
static inline void bau_uvhubs_clear(struct bau_target_uvhubmask *dstp,
				    int nbits)
{
	bitmap_zero(&dstp->bits[0], nbits);
}
static inline int bau_uvhub_weight(struct bau_target_uvhubmask *dstp)
{
	return bitmap_weight((unsigned long *)&dstp->bits[0],
				UV_DISTRIBUTION_SIZE);
}

static inline void bau_cpubits_clear(struct bau_local_cpumask *dstp, int nbits)
{
	bitmap_zero(&dstp->bits, nbits);
}

#define cpubit_isset(cpu, bau_local_cpumask) \
	test_bit((cpu), (bau_local_cpumask).bits)

extern void uv_bau_message_intr1(void);
extern void uv_bau_timeout_intr1(void);

struct atomic_short {
	short counter;
};

static inline int atomic_read_short(const struct atomic_short *v)
{
	return v->counter;
}

static inline int atomic_add_short_return(short i, struct atomic_short *v)
{
	short __i = i;
	asm volatile(LOCK_PREFIX "xaddw %0, %1"
			: "+r" (i), "+m" (v->counter)
			: : "memory");
	return i + __i;
}

#endif /* _ASM_X86_UV_UV_BAU_H */
