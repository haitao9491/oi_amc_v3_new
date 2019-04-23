
#ifndef _SEQ_LST_H_
#define _SEQ_LST_H_

int  seq_lst_init(void);
int  seq_lst_add(unsigned int ipaddr, int port, unsigned short seq);
int  seq_lst_check_timeout(unsigned int *ipaddr, int *port, unsigned short *seq);
int  seq_lst_del(unsigned int ipaddr, int port, unsigned short seq);
void seq_lst_exit(void);
void seq_lst_get_seqno(unsigned short *sn);

#endif
