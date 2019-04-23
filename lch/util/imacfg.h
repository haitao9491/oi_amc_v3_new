/*
 *
 * imacfg.h - A brief description goes here.
 *
 */

#ifndef _HEAD_IMACFG_97CCE340_52CC76EF_79D3D5CF_H
#define _HEAD_IMACFG_97CCE340_52CC76EF_79D3D5CF_H

#define MAX_LNK_NUM			256
#define MAX_GRP_NUM 			128
#define IMA_MAX_LINKS_OF_IMA_GRP        32
#define LINK_NOT_GRP	      		255
#define LINK_IN_TC		      	254

/*----------------------------------------------------------------------------*/
/* IMA TX Link Configuration: sIMA84_CFG_IMA_TX_LINK                          */
/*----------------------------------------------------------------------------*/
typedef struct cfg_ima_tx_link
{
    unsigned char    lid;      /* LID assigned to this physical link          */
    unsigned char    icpOffset;/* ICP cell offset in IMA frame
                            NOTE: icpOffset MUST NOT be zero
                            (1 through 255; the IMA device will automatically
                            adjust the icpOffset value based on the number of
                            cells per frame M, so always use an icpOffset value
                            as if M is 256)                                   */
    unsigned short    phyLink; /* 0-83 for S/UNI-IMA-84, 0-7 for S/UNI-IMA-8  */
} CFG_IMA_TX_LINK;

/*----------------------------------------------------------------------------*/
/* IMA Group Configuration: CFG_IMA_GRP                             	      */
/*----------------------------------------------------------------------------*/
typedef struct cfg_ima_grp
{
    unsigned char    clockMd;     /* one of:
                                             IMA84_IMA_GRP_CLK_MD_ITC,
                                             IMA84_IMA_GRP_CLK_MD_CTC         */
    unsigned char    symmetryMd;  /* one of:
                                             IMA84_IMA_GRP_SYM,
                                             IMA84_IMA_GRP_SYM_CFG_ASYM_OP,
                                             IMA84_IMA_GRP_ASYM               */
    unsigned char    imaVersion;  /* IMA84_VERSION_10 or
                                           IMA84_VERSION_11                   */
    unsigned char    imaOamLabel; /* one of :
                                           IMA84_OAM_LABEL_10,
                                           IMA84_OAM_LABEL_11 or
                                           user defined
																		      */
    unsigned char    rxImaIdCfgEn;/* 0 - recover IMA ID from incoming
                                                ICP cells
                                           1 - specify IMA ID to check for in
                                                incoming ICP                  */
    unsigned char    rxImaIdCfg;  /* IMA ID to check in incoming ICP
                                            cells (ignored if rxImaIdCfgEn=0)
                                            (0-255)                           */
    unsigned char    txImaId;     /* IMA id used in outgoing ICP cells
                                            (0-255)                           */
    unsigned short   txVphy;        /* 7-bit VPHY id on TX Any-PHY/UTOPIA
	                                         interface with the following
											 valid values:
                                             0-83 (for Any-PHY)
                                             0-30 (for UTOPIA MPHY)           */
    unsigned short   rxVphy;        /* 16-bit VPHY id on RX Any-PHY/UTOPIA
											 interface with the 7 LSBs
											 restricted to the following:
                                             0-83 (for Any-PHY/UTOPIA SPHY)
                                             0-30 (for UTOPIA MPHY)           */
    unsigned char    rxM;         /* Rx frame length acceptable range
                                             bit 3: M=256 (0-no; 1-yes)
                                             bit 2: M=128 (0-no; 1-yes)
                                             bit 1: M= 64 (0-no; 1-yes)
                                             bit 1: M= 32 (0-no; 1-yes)       */
    unsigned char    pRx;         /* Min num active Rx links required
                                           (0-31)                             */
    unsigned char    txM;         /* Transmit IMA frame length
	                                         IMA84_IMA_M_32  (0)
	                                         IMA84_IMA_M_64  (1)
	                                         IMA84_IMA_M_128 (2)
	                                         IMA84_IMA_M_256 (3)              */
    unsigned char    pTx;         /* Min num active Tx links required
                                           (0-31)                             */
    unsigned short   maxDelay;    /* Max DCB buf depth (0 - gblMaxDelay);
	                                       see maxDcbDepth field in DIV       */
    unsigned short   delayGb;     /* Delay guardband (0 to maxDelay-1)        */
    unsigned short   delayAddEn;  /* Allow delay addition to accommodate
                                           a new link                         */
	unsigned char    reserved;    /* reserved for future use                  */
	unsigned char    stuffMd;	  /* one of:
	                                        IMA84_IMA_GRP_STUFF_MD_ITC,
	                                        IMA84_IMA_GRP_STUFF_MD_CTC        */
	unsigned char    stuffAdvMd;  /* one of:
	                                        IMA84_IMA_GRP_STUFF_ADV_1_AHEAD,
	                                        IMA84_IMA_GRP_STUFF_ADV_4_AHEAD   */
	unsigned short   txTrl; 	  /* physical link id of transmit TRL
                                           (0-83)                             */
    unsigned short   numRxLinks;  /* Num valid entries in rxLinks array */
    unsigned short   numTxLinks;  /* Num valid entries in txLinks array */
    unsigned short   rxLinks[IMA_MAX_LINKS_OF_IMA_GRP];
                                        /* Array of rx physical links         */
    CFG_IMA_TX_LINK txLinks[IMA_MAX_LINKS_OF_IMA_GRP];
                                        /* Array of tx phylink cfg info       */
} CFG_IMA_GRP;

typedef struct ima_cfg {
	unsigned char grpNumAll;
	unsigned char groupNum;
	unsigned char tcNum;
	unsigned char linkInGrp[MAX_LNK_NUM];
	unsigned char grpMode[MAX_GRP_NUM + MAX_LNK_NUM];
	unsigned char grpId[MAX_GRP_NUM + MAX_LNK_NUM];
	CFG_IMA_GRP grpCfg[MAX_GRP_NUM + MAX_LNK_NUM];
} IMA_CFG;

#if defined(__cplusplus)
extern "C" {
#endif

extern int imacfg_load(unsigned long hd, void *cfg);
extern void imacfg_dump(FILE *fp, void *cfg);
extern char *imacfg_print(IMA_CFG *cfg);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_IMACFG_97CCE340_52CC76EF_79D3D5CF_H */
