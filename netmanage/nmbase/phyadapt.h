#ifndef PHY_ADAPT_H_
#define PHY_ADAPT_H_

/* SDH alarms and error counters */
#define SDH_ALARM_LOS    0x00000001
#define SDH_ALARM_LOF    0x00000002  /* A1, A2 */
#define SDH_ALARM_RS_TIM 0x00000004  /* J0 */
#define SDH_ALARM_SF     0x00000008  /* B2 */
#define SDH_ALARM_SD     0x00000010  /* B2 */
#define SDH_ALARM_MS_AIS 0x00000020  /* K2 */
#define SDH_ALARM_DCRU_ERROR     0x00010000
#define SDH_ALARM_JAT_FIFO_ERROR 0x00020000
#define SDH_ALARM_JAT_DLL_ERROR  0x00040000

typedef struct {
	unsigned int alarm;
	unsigned int rs_bip;  /* B1 */
	unsigned int ms_bip;  /* B2 */
	unsigned int ms_rei;  /* M1 */
} nm_sl_status;

#define SDH_ALARM_AU_AIS 0x00000100
#define SDH_ALARM_AU_LOP 0x00000200

typedef struct {
	unsigned int alarm;
} nm_au_status;

#define SDH_ALARM_HP_TIM  0x00000001
#define SDH_ALARM_HP_TIU  0x00000002
#define SDH_ALARM_HP_ERDI 0x00000004
#define SDH_ALARM_HP_RDI  0x00000008
#define SDH_ALARM_HP_PDI  0x00000010
#define SDH_ALARM_HP_UNEQ 0x00000020
#define SDH_ALARM_HP_PLM  0x00000040
#define SDH_ALARM_HP_PLU  0x00000080
#define SDH_ALARM_HP_SF   0x00000100
#define SDH_ALARM_HP_SD   0x00000200

typedef struct {
	unsigned int alarm;
} nm_hp_status;

#define SDH_ALARM_LP_TAIS 0x00000001
#define SDH_ALARM_LP_LOP  0x00000002
#define SDH_ALARM_LP_RDI  0x00000004
#define SDH_ALARM_LP_ERDI 0x00000008
#define SDH_ALARM_LP_PSLU 0x00000010
#define SDH_ALARM_LP_PSLM 0x00000020
#define SDH_ALARM_LP_UNEQ 0x00000040
#define SDH_ALARM_LP_PDI  0x00000080
#define SDH_ALARM_LP_TIU  0x00000100
#define SDH_ALARM_LP_TIM  0x00000200
#define SDH_ALARM_LP_SF   0x00000400
#define SDH_ALARM_LP_SD   0x00000800
#define SDH_ALARM_LP_LOM  0x00001000
#define SDH_ALARM_LP_AIS  0x00010000
#define SDH_ALARM_LP_RED  0x00020000
#define SDH_ALARM_LP_AISD 0x00040000
#define SDH_ALARM_LP_RAI  0x00080000
#define SDH_ALARM_LP_RXELST_SLP  0x00100000
#define SDH_ALARM_LP_TXELST_SLP  0x00200000

typedef struct {
	unsigned int alarm;
	unsigned int pje, nje;
	unsigned int bip, rei;
} nm_lp_status;

typedef struct {
	nm_sl_status   sl[4];       /* Section/Line (Regenerator/Multiplexer) STM1# */
	nm_au_status   au[4*3];     /* SPE (AU) Pointer STM1#                   */
	nm_hp_status   hp[4*3];     /* High-Order Path STM1# TUG3#             */
	nm_lp_status   lp[4*3*7*3]; /* Low-Order Path STM1# TUG3# TUG2# TU12# */
} nm_sdh_status;

typedef struct nm_sdh_status {
	nm_sl_status   *slptr;   /* ptr to sl */
	nm_au_status   *auptr;   /* ptr to au */
	nm_hp_status   *hpptr;   /* ptr to hp  */
	nm_lp_status   *lpptr;   /* ptr to lp */
} nm_sdh_status_t;


/* Receive side */
#define ALARM_LOS   0x00000001
#define ALARM_RED   0x00000001
#define ALARM_LOF   0x00000002
#define ALARM_AIS   0x00000004 /* E1, AIS        */
#define ALARM_BLUE  0x00000004 /* T1, Blue Alarm */
#define ALARM_RAI   0x00000008
#define ALARM_YEL   0x00000008

#define ALARM_CASMFLOSS 0x00000010
#define ALARM_CRCMFLOSS 0x00000020
#define ALARM_CASMFRAI  0x00000040

#define ALARM_BERTSYNC 0x00001000
#define ALARM_BERT_RA1 0x00002000
#define ALARM_BERT_RA0 0x00004000

#define ALARM_OEQ   0x00010000
#define ALARM_UEQ   0x00020000
#define ALARM_JALT  0x00040000
#define ALARM_RESF  0x00080000
#define ALARM_RESEM 0x00100000
#define ALARM_RSLIP 0x00200000

#define ALARM_RSL_MASK  0xf0000000
#define ALARM_RSL_SHIFT 28

/* Transmit side */
#define ALARM_TOC   0x00000001 /* Open circuited  */
#define ALARM_TSC   0x00000002 /* Short circuited */

/* Dependency */
#define ALARM_LT_CASMFLOSS  ALARM_CASMFRAI
#define ALARM_LT_LOF       (ALARM_CASMFLOSS | ALARM_LT_CASMFLOSS | ALARM_CRCMFLOSS | ALARM_RAI)
#define ALARM_LT_AIS       (ALARM_LOF | ALARM_LT_LOF)
#define ALARM_LT_LOS       (ALARM_AIS | ALARM_LT_AIS)

typedef struct {
	unsigned int r;
	unsigned int t;
} nm_port_alarm;

typedef struct {
	unsigned int lcvc;
	unsigned int pcvc;
	unsigned int fosc;
	unsigned int ebc;
} nm_port_stat;

#define NP_AMCE1_MAX_PHY 2
#define NP_PHY_MAX_PORT  8
#define NP_EIPB_MAX_PHY 16

typedef struct {
        nm_port_alarm alarm[NP_AMCE1_MAX_PHY][NP_PHY_MAX_PORT];
        nm_port_stat  stat[NP_AMCE1_MAX_PHY][NP_PHY_MAX_PORT];
} nm_amce1_status;

typedef struct {
        nm_port_alarm alarm[NP_EIPB_MAX_PHY][NP_PHY_MAX_PORT];
        nm_port_stat  stat[NP_EIPB_MAX_PHY][NP_PHY_MAX_PORT];
} nm_eipb_status;

typedef struct nm_e1_status{
	nm_port_alarm *alarm;
	nm_port_stat  *stat;
} nm_e1_status_t;

#endif
