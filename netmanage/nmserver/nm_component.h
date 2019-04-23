#ifndef __H_NM_COMPONENT_H_
#define __H_NM_COMPONENT_H_


#if defined(__cplusplus)
extern "C" {
#endif

void *nm_component_open(void);
int nm_component_open_comp(void *hd, unsigned int flag);
void *nm_component_get_handle(void *hd, int flag);
int nm_component_close_comp(void *hd, int flag);
unsigned int nm_component_get_comp(void *glb, char *ip, int port);
int nm_component_close(void *hd);
void *nm_component_lprocess(void *hd, void *glb, char *ip, int port, void *data);

#if defined(__cplusplus)
}
#endif

#endif
