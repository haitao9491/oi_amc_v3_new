#ifndef __H_NM_GLB_H_
#define __H_NM_GLB_H_

#pragma pack(1)
struct nm_lkaddr_info {
	unsigned char rack;
	unsigned char shelf;
	unsigned char slot;
	unsigned char subslot;
};

struct nmboard_info {
	unsigned char rack;
	unsigned char shelf;
	unsigned char slot;
	unsigned char subslot;
	unsigned int  comp;
	unsigned char bdtype;
	unsigned char stat;
};
#pragma pack()

#if defined(__cplusplus)
extern "C" {
#endif

void *nms_glb_open(void);
int nms_glb_insert_bdinfo(void *hd, char *ip, int port, struct nmboard_info *bdi);
int nms_glb_get_bdinfo(void *hd, char *ip, int port, struct nmboard_info *bdi);
int nms_glb_get_ip_port(void *hd, char *ip, int *port, struct nm_lkaddr_info *lki);
int nms_glb_get_all_bdinfo(void *hd, 
		int (*func)(char *ip, int port, struct nmboard_info bdi, void *arg), void *arg);
void *nms_glb_get_all_bdinfo_pkt(void *hd, int *dlen);
int nms_glb_del_bdinfo(void *hd, char *ip, int port);
int nms_glb_release_bdinfo(void *hd);
int nms_glb_close(void *hd);
int nm_glb_code_bdinfo(unsigned char *data, struct nmboard_info bdi);
int nm_glb_decode_bdinfo(void *data, struct nmboard_info *bdi);
int nm_glb_decode_lkaddr_info(void *data, struct nm_lkaddr_info *lki);

#if defined(__cplusplus)
}
#endif

#endif
