

#ifndef _OCFS2_HEARTBEAT_H
#define _OCFS2_HEARTBEAT_H

struct o2hb_disk_heartbeat_block {
	__le64 hb_seq;
	__u8  hb_node;
	__u8  hb_pad1[3];
	__le32 hb_cksum;
	__le64 hb_generation;
	__le32 hb_dead_ms;
};

#endif /* _OCFS2_HEARTBEAT_H */
