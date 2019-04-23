#ifndef __H_NM_E1PHY_H_
#define __H_NM_E1PHY_H_

#include "nm_glb.h"

#if defined(__cplusplus)
extern "C" {
#endif

void *nm_e1phy_open(void);
int nm_e1phy_add_data(void *hd, int phycnt, int pcnt, void *data);
void *nm_e1phy_get_data(void *hd, int *phycnt, int *pcnt);
void *nm_e1phy_get_data_pkt(void *hd, struct nm_lkaddr_info *lki, int *dlen);
int nm_e1phy_release_data(void *hd);
int nm_e1phy_close(void *hd);

#if defined(__cplusplus)
}
#endif

#endif
