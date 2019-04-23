/*
 *
 * pktdecode.c - A brief description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "pktdecode.h"

int pkt_decode(struct ps_protocol_handle *p, struct ps_decoding_result *result,
		unsigned char *data, int len, int *trailerlen)
{
	pkt_hdr *ph;
	struct fld_value fv;
	unsigned int s, ns;

	if (pkt_verify(data, 0) < 0)
		return -1;

	ph = (pkt_hdr *)data;
	pkthdr_get_ts(ph, &s, &ns);

	ps_result_push_value_g(result, fv, FLD_TYPE_UINT, ui,
			PVID_PKT_TYPE_SUBTYPE,
			(pkthdr_get_type(ph)<<8) | pkthdr_get_subtype(ph), -1);
	ps_result_push_value_g(result, fv, FLD_TYPE_UINT, ui,
			PVID_PKT_PROTOCOL, pkthdr_get_protocol(ph), -1);
	ps_result_push_value_g(result, fv, FLD_TYPE_UCHAR, uc,
			PVID_PKT_SC, pkthdr_get_sc(ph), -1);
	ps_result_push_value_g(result, fv, FLD_TYPE_USHORT, us,
			PVID_PKT_DEVICE, pkthdr_get_device(ph), -1);
	ps_result_push_value_g(result, fv, FLD_TYPE_USHORT, us,
			PVID_PKT_CHANNEL, pkthdr_get_channel(ph), -1);
	ps_result_push_value_g(result, fv, FLD_TYPE_UINT, ui,
			PVID_PKT_TS_S, s, -1);
	ps_result_push_value_g(result, fv, FLD_TYPE_UINT, ui,
			PVID_PKT_TS_NS, ns, -1);

	return sizeof(pkt_hdr);
}
