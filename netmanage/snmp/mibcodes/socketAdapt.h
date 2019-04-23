/*
 * socketAdapt.h
 */
#ifndef SOCKETADAPT_H
#define SOCKETADAPT_H

#include "nmpkt.h"
/* function declarations */
int socketAdapt_send_cmd_req(uint8_t module, uint8_t cmd, uint8_t *data, int dlen);
int init_socketAdapt(void);
void exit_socketAdapt(void);

#endif /* SOCKETADAPT_H */
