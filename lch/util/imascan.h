/*
 *
 * imascan.h - A brief description goes here.
 *
 */

#ifndef _HEAD_IMASCAN_97CCE340_52CC76EF_79D3D5CF_H
#define _HEAD_IMASCAN_97CCE340_52CC76EF_79D3D5CF_H

typedef struct ima_icp_struct
{
	/* 1-5  ATM cell header  Octet 1 = 0000 0000, Octet 2 = 0000 0000,
	 *                       Octet 3 = 0000 0000, Octet 4 = 0000 1011,
	 *                       Octet 5 = 0110 0100 (valid HEC)*/
	unsigned char header[5];

	/* 6 OAM Label        Bits 7-0: IMA Version Value
	 * 00000001: IMA Version 1.0
	 * 00000011: IMA Version 1.1 */
	unsigned char ver;
	/* 7 A  Cell ID and Link ID
	 * Bit 7: IMA OAM Cell Type (1: ICP cell)
	 * Bits 6-5: Unused and set to 0
	 * Bits 4-0: Logical ID for Tx IMA link range (0...31)*/
	unsigned char cell_link_id;
	/* 8 A IMA Frame Sequence Number IMA Frame Sequence Number
 	 * from 0 to 255 and cycling.*/
	unsigned char ifsn;
	/* 9 A ICP Cell Offset Range (0... M-1): indicates position of
	 * ICP cell within the IMA frame.*/
	unsigned char icp_off;
	/* 10 A Link Stuff Indication Bits 7-3: Unused and set to 0
	 * Bits 2-0: Link Stuffing Indication (LSI)
	 * 111 = No imminent stuff event,
	 * 100 = Stuff event in 4 ICP cell locations (optional),
	 * 011 = Stuff event in 3 ICP cell locations (optional),
	 * 010 = Stuff event in 2 ICP cell locations (optional),
	 * 001 = Stuff event at the next ICP cell location (mandatory),
	 * 000 = This is one out of the 2 ICP cells comprising the stuff
	 * event (mandatory).*/
	unsigned char link_stuff_ind;
	/* 11 B Status & Control Change Indication
	 * Bits 7-0: Status change indication: 0 to 255 and cycling
	 * (count to be incremented every change of octets 12-49)*/
	unsigned char status_ctrl_ind;
	/* 12 B IMA ID Bits 7-0: IMA ID*/
	unsigned char ima_id;
	/* 13 B Group Status and Control
	 * Bits 7-4: Group State
	 * 0000 = Start-up,
	 * 0001 = Start-up-Ack,
	 * 0010 = Config-Aborted - Unsupported M,
	 * 0011 = Config-Aborted - Incompatible Group Symmetry,
	 * 0100 = Config-Aborted - Unsupported IMA Version,
	 * 0101, 0110 = Reserved for other Config-Aborted reasons
	 *              in a future version of theIMA specification,
	 * 0111 = Config-Aborted - Other reasons,
	 * 1000 = Insufficient-Links,
	 * 1001 = Blocked,
	 * 1010 = Operational,
	 * Others: Reserved for later use in a future version of
	 *         the IMA specification.
	 * Bits 3-2: Group Symmetry Mode
	 * 00 = Symmetrical configuration and operation,
	 * 01 = Symmetrical configuration and asymmetrical operation (optional),
	 * 10 = Asymmetrical configuration and asymmetrical operation (optional)
	 * 11 = Reserved
	 * Bits 1-0: IMA Frame Length
	 * (00: M=32, 01: M=64, 10: M=128, 11: M=256)*/
	unsigned char grp_status_ctrl;
	/* 14 B Transmit Timing Information
	 * Bits 7-6: Unused and set to 0
	 * Bit 5: Transmit Clock Mode: (0: ITC mode, 1: CTC mode)
	 * Bits 4-0: Tx LID of the timing reference (0 to 31)*/
	unsigned char tx_timing_info;
	/* 15 B Tx Test Control
	 * Bits 7-6: Unused and set to 0
	 * Bit 5: Test Link Command (0: inactive, l: active)
	 * Bits 4-0: Tx LID of test link (0 to 31)*/
	unsigned char tx_test_ctrl;
	/* 16 B Tx Test Pattern
	 * Bits 7-0: Tx Test Pattern (value from 0 to 255)*/
	unsigned char tx_test_pattern;
	/* 17 B Rx test Pattern
	 * Bits 7-0: Rx Test Pattern (value from 0 to 255)*/
	unsigned char rx_test_pattern;
	/* 18-49 C Link 0-Link31 Information
	 * Bits 7-5: Transmit State (see Table 3 on page 32)
	 * 000 Not In Group
	 * 001 Unusable No reason given
	 * 010 Unusable Fault (vendor specific)
	 * 011 Unusable Mis-connected
	 * 100 Unusable Inhibited (vendor specific)
	 * 101 Unusable Failed (not currently defined)
	 * 110 Usable
	 * 111 Active
	 * Bits 4-2: Receive State (see Table 3 on page 32)
	 * 000 Not In Group
	 * 001 Unusable No reason given
	 * 010 Unusable Fault (vendor specific)
	 * 011 Unusable Mis-connected
	 * 100 Unusable Inhibited (vendor specific)
	 * 101 Unusable Failed (not currently defined)
	 * 110 Usable
	 * 111 Active
	 * Bits 1-0: Rx Defect Indicators (see Table 3 on page 32)
	 * 00 No defect
	 * 01 Physical Link Defect (e.g.LOS,OOF/LOF, LCD)
	 * 10 LIF
	 * 11 LODS*/
	unsigned char link_status_ctrl[32];
	/* 50 D Unused Set to 0x6A as defined in
	 * ITU-T Recommendation I.432[30] for unused bytes.*/
	unsigned char unused;
	/* 51 E End-to-end channel
	 * Proprietary channel (set to 0 if unused).*/
	unsigned char end_to_end_chan;
	/*52-53 CRC Error Control
	 * Bits 15-10: Reserved field for future use default
	 * value coded all zero
	 * Bits 9-0: CRC-10 as specified in ITU-T Recommendation I.610[31]*/
	unsigned char crc_err_control[2];
} IMA_ICP;

#if defined(__cplusplus)
extern "C" {
#endif

extern DLL_APP void *imascan_open(int max_channel, int max_icpnum);
extern DLL_APP void *imascan_proc(void *handle, unsigned char *pktcell);
extern DLL_APP int icpinfo(void *handle, unsigned char *pktcell);
extern DLL_APP void imascan_close(void *handle);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_IMASCAN_97CCE340_52CC76EF_79D3D5CF_H */
