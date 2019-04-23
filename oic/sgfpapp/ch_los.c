/*
 * (C) Copyright 2018
 * liye <ye.li@raycores.com>
 *
 * ch_los.c - A description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sgfplib.h"

void *__lgwr__handle = NULL;

int main()
{
	struct fpga_board_runinfo_ex port_states;
    static void* ghd = NULL;
    int los = -1;
    int i;

	ghd = sgfplib_open(0);
    if (ghd == NULL) {
        printf("Failed to sgfplib_open.");
        return -1;
    }
	memset(&port_states, 0, sizeof(struct fpga_board_runinfo_ex));
	if (sgfplib_get_fpga_bd_runinfo_ex(ghd, &port_states) != 0) {
        printf("Failed to sgfplib_get_fpga_bd_runinfo_ex.");
		return -1;
    }
    sgfplib_close(ghd);
    
	for (i = 0; i < FPGA_OI_PORTNUM; i++) {
        los &= port_states.ports[i].los;
    }
    printf("%d", los);

    return 0;
}
