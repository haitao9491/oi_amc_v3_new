/*
 *
 * nm_msg.h - A brief description goes here.
 *
 */

#ifndef __NM_MSG_H__
#define __NM_MSG_H__


#if defined(__cplusplus)
extern "C" {
#endif

void *nmmsg_cmd_glb_info(void);
void *nmmsg_cmd_set_sw_an(void *data);
void *nmmsg_cmd_set_sw_pwrdn(void *data);

#if defined(__cplusplus)
}
#endif

#endif
