/*
 *
 * imascan.c - A brief description goes here.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "os.h"
#include "aplog.h"
#include "pkt.h"
#include "imacfg.h"
#include "imascan.h"

#ifdef WIN32
#pragma warning ( disable : 4996 )
#endif

#define MAX_ICPNUM	100

static int ima_m[4] = {32, 64, 128, 256};

typedef struct ima_phy_link_struct
{
	/* Physical link number: 0-62 */
	unsigned char phy_nr;
	/* ICP cell recieved from physical link */
	unsigned char rcv_nr;
	/* Pointer to ICP cell recieved from physical link */
	IMA_ICP*      p_icp;

} IMA_PHY_LINK;

typedef struct ima_grp_struct
{
	/* Start index of this IMA Group */
	unsigned int index;
	/* Total link number in this IMA Group */
	unsigned int link_num;
	/* IMA config is ok after verification */
	unsigned int ok;
} IMA_GRP;

typedef struct ima_probe {
	int icpcount;
	int maxicp;
	IMA_PHY_LINK *imaplink;
} IMA_PROBE;

/******************************************************************************
**
** DESCRIPTION:	This module check if the ATM cell is ICP cell.
**
*******************************************************************************/
static IMA_ICP *is_icp_cell(unsigned char *pdata)
{
	IMA_ICP *picp = NULL;

	if (pdata == NULL)
		return picp;

	/* 1-5  ATM cell header  Octet 1 = 0000 0000, Octet 2 = 0000 0000,
	 *                       Octet 3 = 0000 0000, Octet 4 = 0000 1011,
	 *                       Octet 5 = 0110 0100 (valid HEC)*/
	if (pdata[0] == 0 &&
		pdata[1] == 0 &&
		pdata[2] == 0 &&
		pdata[3] == 0x0b &&
		pdata[4] == 0x64 &&
		(pdata[5] == 0x01 || pdata[5] == 0x03) &&
        /* 7 A  Cell ID and Link ID
		 * Bit 7: IMA OAM Cell Type (1: ICP cell) */
		(pdata[6] & 0x80) == 0x80)
	{
		picp = (IMA_ICP *)malloc(sizeof(IMA_ICP));
		if (picp)
			memcpy(picp, pdata, sizeof(IMA_ICP));
	}

	return picp;
}

/******************************************************************************
**
** DESCRIPTION:	This module check if the Link and Group status is stable.
**
*******************************************************************************/
static int is_link_stable(IMA_ICP *p_icp1, IMA_ICP *p_icp2)
{

	if (p_icp1 == NULL || p_icp2 == NULL)
		return 0;

	/*For a specific link, if the Status  and Control Indication of two ICP
	 * cell is same, then we think the Link and Group status is stable */
	if ((p_icp1->cell_link_id == p_icp2->cell_link_id) &&
		(p_icp1->status_ctrl_ind == p_icp2->status_ctrl_ind))
		return 1;

	return 0;
}


/******************************************************************************
**
** DESCRIPTION:	This module is the comparison routine for qsort.
**
** INPUTS:   IMA_PHY_LINK* p_link_1
**           IMA_PHY_LINK* p_link_2
**
** OUTPUTS:   negative integer if p_link_1->p_icp->ver < p_link_2->p_icp->ver
**            0                if p_link_1->p_icp->ver = p_link_2->p_icp->ver
**            positive integer if p_link_1->p_icp->ver > p_link_2->p_icp->ver
**            For those physical link recieving no ICP Cell, that is the  p_icp
**            is NULL, move them to the end
**
*******************************************************************************/
static int sort_link_for_ima_ver(const void *p1, const void *p2)
{
	IMA_PHY_LINK *p_link_1 = (IMA_PHY_LINK *)p1;
	IMA_PHY_LINK *p_link_2 = (IMA_PHY_LINK *)p2;

	if (p_link_1->p_icp == NULL)
		return 1;
	if (p_link_2->p_icp == NULL)
		return -1;

	return (p_link_1->p_icp->ver - p_link_2->p_icp->ver);
}

/******************************************************************************
**
** DESCRIPTION:	This module is the comparison routine for qsort.
**
** INPUTS:   IMA_PHY_LINK* p_link_1
**           IMA_PHY_LINK* p_link_2
**
** OUTPUTS:   negative int if p_link_1->p_icp->ima_id < p_link_2->p_icp->ima_id
**            0            if p_link_1->p_icp->ima_id = p_link_2->p_icp->ima_id
**            positive int if p_link_1->p_icp->ima_id > p_link_2->p_icp->ima_id
**            For those physical link recieving no ICP Cell, that is the  p_icp
**            is NULL, move them to the end
**
*******************************************************************************/
static int sort_link_for_ima_id(const void *p1, const void *p2)
{
	IMA_PHY_LINK *p_link_1 = (IMA_PHY_LINK *)p1;
	IMA_PHY_LINK *p_link_2 = (IMA_PHY_LINK *)p2;

	return (p_link_1->p_icp->ima_id - p_link_2->p_icp->ima_id);
}

/******************************************************************************
**
** DESCRIPTION:	This module is the comparison routine for qsort.
**
** INPUTS:   IMA_PHY_LINK* p_link_1
**           IMA_PHY_LINK* p_link_2
**
** OUTPUTS:   negative int if status_ctrl_ind of link1 < that of link2
**            0            if status_ctrl_ind of link1 = that of link2
**            positive int if status_ctrl_ind of link1 > that of link2
**            For those physical link recieving no ICP Cell, that is the  p_icp
**            is NULL, move them to the end
**
*******************************************************************************/
static int sort_link_for_status_ctrl_ind(const void *p1, const void *p2)
{
	IMA_PHY_LINK *p_link_1 = (IMA_PHY_LINK *)p1;
	IMA_PHY_LINK *p_link_2 = (IMA_PHY_LINK *)p2;

	return(p_link_1->p_icp->status_ctrl_ind - p_link_2->p_icp->status_ctrl_ind);
}

/******************************************************************************
**
** DESCRIPTION:	This module is the comparison routine for qsort.
**
** INPUTS:   IMA_PHY_LINK* p_link_1
**           IMA_PHY_LINK* p_link_2
**
** OUTPUTS:   negative int if tx_timing_info of link1 < that of link2
**            0            if tx_timing_info of link1 = that of link2
**            positive int if tx_timing_info of link1 > that of link2
**            For those physical link recieving no ICP Cell, that is the  p_icp
**            is NULL, move them to the end
**
*******************************************************************************/
static int sort_link_for_tx_timing_info(const void *p1, const void *p2)
{
	IMA_PHY_LINK *p_link_1 = (IMA_PHY_LINK *)p1;
	IMA_PHY_LINK *p_link_2 = (IMA_PHY_LINK *)p2;

	return (p_link_1->p_icp->tx_timing_info - p_link_2->p_icp->tx_timing_info);
}

/******************************************************************************
**
** DESCRIPTION:	This module is the comparison routine for qsort.
**
** INPUTS:   IMA_PHY_LINK* p_link_1
**           IMA_PHY_LINK* p_link_2
**
** OUTPUTS:   negative int if tx_test_ctrl of link1 < that of link2
**            0            if tx_test_ctrl of link1 = that of link2
**            positive int if tx_test_ctrl of link1 > that of link2
**            For those physical link recieving no ICP Cell, that is the  p_icp
**            is NULL, move them to the end
**
*******************************************************************************/
static int sort_link_for_tx_test_ctrl(const void *p1, const void *p2)
{
	IMA_PHY_LINK *p_link_1 = (IMA_PHY_LINK *)p1;
	IMA_PHY_LINK *p_link_2 = (IMA_PHY_LINK *)p2;

	return (p_link_1->p_icp->tx_test_ctrl - p_link_2->p_icp->tx_test_ctrl);
}

/******************************************************************************
**
** DESCRIPTION:	This module is the comparison routine for qsort.
**
** INPUTS:   IMA_PHY_LINK* p_link_1
**           IMA_PHY_LINK* p_link_2
**
** OUTPUTS:   negative int if tx_test_pattern of link1 < that of link2
**            0            if tx_test_pattern of link1 = that of link2
**            positive int if tx_test_pattern of link1 > that of link2
**            For those physical link recieving no ICP Cell, that is the  p_icp
**            is NULL, move them to the end
**
*******************************************************************************/
static int sort_link_for_tx_test_pattern(const void *p1, const void *p2)
{
	IMA_PHY_LINK *p_link_1 = (IMA_PHY_LINK *)p1;
	IMA_PHY_LINK *p_link_2 = (IMA_PHY_LINK *)p2;

	return(p_link_1->p_icp->tx_test_pattern - p_link_2->p_icp->tx_test_pattern);
}

/******************************************************************************
**
** DESCRIPTION:	This module is the comparison routine for qsort.
**
** INPUTS:   IMA_PHY_LINK* p_link_1
**           IMA_PHY_LINK* p_link_2
**
** OUTPUTS:   negative int if rx_test_pattern of link1 < that of link2
**            0            if rx_test_pattern of link1 = that of link2
**            positive int if rx_test_pattern of link1 > that of link2
**            For those physical link recieving no ICP Cell, that is the  p_icp
**            is NULL, move them to the end
**
*******************************************************************************/
static int sort_link_for_rx_test_pattern(const void *p1, const void *p2)
{
	IMA_PHY_LINK *p_link_1 = (IMA_PHY_LINK *)p1;
	IMA_PHY_LINK *p_link_2 = (IMA_PHY_LINK *)p2;

	return(p_link_1->p_icp->rx_test_pattern - p_link_2->p_icp->rx_test_pattern);
}

/******************************************************************************
**
** DESCRIPTION:	This module is the comparison routine for qsort.
**
** INPUTS:   IMA_PHY_LINK* p_link_1
**           IMA_PHY_LINK* p_link_2
**
** OUTPUTS:   negative int if ifsn of link1 < that of link2
**            0            if ifsn of link1 = that of link2
**            positive int if ifsn of link1 > that of link2
**            For those physical link recieving no ICP Cell, that is the  p_icp
**            is NULL, move them to the end
**
*******************************************************************************/
static int sort_link_for_ifsn(const void *p1, const void *p2)
{
	IMA_PHY_LINK *p_link_1 = (IMA_PHY_LINK *)p1;
	IMA_PHY_LINK *p_link_2 = (IMA_PHY_LINK *)p2;

	return (p_link_1->p_icp->ifsn - p_link_2->p_icp->ifsn);
}

static int sort_link_for_phylink(const void *p1, const void *p2)
{
	IMA_PHY_LINK *p_link_1 = (IMA_PHY_LINK *)p1;
	IMA_PHY_LINK *p_link_2 = (IMA_PHY_LINK *)p2;

	return (p_link_1->phy_nr - p_link_2->phy_nr);
}

/******************************************************************************
**
** DESCRIPTION:	This module is the comparison routine for qsort.
**
** INPUTS:   IMA_PHY_LINK* p_link_1
**           IMA_PHY_LINK* p_link_2
**
** OUTPUTS:   negative int if link_status_ctrl of link1 < that of link2
**            0            if link_status_ctrl of link1 = that of link2
**            positive int if link_status_ctrl of link1 > that of link2
**            For those physical link recieving no ICP Cell, that is the  p_icp
**            is NULL, move them to the end
**
*******************************************************************************/
static int sort_link_for_link_status_ctrl(const void *p1, const void *p2)
{
	int  i, tmp;
	IMA_PHY_LINK *p_link_1 = (IMA_PHY_LINK *)p1;
	IMA_PHY_LINK *p_link_2 = (IMA_PHY_LINK *)p2;

	for (i = 0; i < 32; i++)
	{
		tmp = p_link_1->p_icp->link_status_ctrl[i] -
					p_link_2->p_icp->link_status_ctrl[i];
		if (tmp != 0)
		{
			return tmp;
		}
	}

	return 0;
}

/******************************************************************************
**
** DESCRIPTION:	This module is the comparison routine for qsort.
**
** INPUTS:   IMA_PHY_LINK* p_link_1
**           IMA_PHY_LINK* p_link_2
**
** OUTPUTS:   negative int if grp_status_ctrl of link1 < that of link2
**            0            if grp_status_ctrl of link1 = that of link2
**            positive int if grp_status_ctrl of link1 > that of link2
**            For those physical link recieving no ICP Cell, that is the  p_icp
**            is NULL, move them to the end
**
*******************************************************************************/
static int sort_link_for_grp_status_ctrl(const void *p1, const void *p2)
{
	IMA_PHY_LINK *p_link_1 = (IMA_PHY_LINK *)p1;
	IMA_PHY_LINK *p_link_2 = (IMA_PHY_LINK *)p2;

	return(p_link_1->p_icp->grp_status_ctrl - p_link_2->p_icp->grp_status_ctrl);
}

/******************************************************************************
**
** DESCRIPTION:	This module split each group based on IMA Version.
**
*******************************************************************************/
static int split_for_ima_ver(IMA_PHY_LINK *ima_link,
			 IMA_GRP *ima_grp, unsigned int *grp_num)
{
	int i, j, tmp_val1, tmp_val2, tmp_grp_num, tmp_index;

	if (ima_link == NULL || ima_grp == NULL || grp_num == NULL)
		return -1;

	tmp_grp_num = *grp_num;

	/*Firstly move all the links with ICP cells to the start of the array */
	qsort(ima_link, MAX_LNK_NUM, sizeof(IMA_PHY_LINK), sort_link_for_ima_ver);

	for (i = 0; i < tmp_grp_num; i++)
	{
		tmp_index = ima_grp[i].index;

		qsort(&ima_link[tmp_index], ima_grp[i].link_num,
			sizeof(IMA_PHY_LINK), sort_link_for_ima_ver);

		tmp_val1 = ima_link[tmp_index].p_icp->ver;

		/* Now try to split this group into 2 groups,
		 * the new group will be appended to the end of group array.
		 * The new group will be trying to split again after */
		for (j = tmp_index + 1; j < (tmp_index + (int)ima_grp[i].link_num); j++)
		{
			tmp_val2 = ima_link[j].p_icp->ver;
			/* If the value of tmp_val2 is different to tmp_val1,
			 * this means a new group is being met */
			if (tmp_val2 != tmp_val1)
			{
				/* Create a new group at the ned of the array */
				ima_grp[tmp_grp_num].index = j;
				ima_grp[tmp_grp_num].link_num =
					ima_grp[i].link_num - j + tmp_index;

				/* Update orignal group link number */
				ima_grp[i].link_num = j - tmp_index;

				/* Update total group nmbers */
				tmp_grp_num++;
				break;
			}
		}
		/* Avoid array borderay being crossed */
		if (tmp_grp_num > MAX_GRP_NUM)
		{
			LOGERROR("Err: tmp_grp_num > MAX_GRP_NUM\n");
			return -1;
		}
	}
	/* Update the tmp value to the original output parameter */
	*grp_num = tmp_grp_num;

	return 0;
}

/******************************************************************************
**
** DESCRIPTION:	This module split each group based on IMA ID.
**
*******************************************************************************/
static int split_for_ima_id(IMA_PHY_LINK *ima_link,
			 IMA_GRP *ima_grp, unsigned int *grp_num)
{
	int i, j, tmp_val1, tmp_val2, tmp_index;
	int tmp_grp_num;

	if (ima_link == NULL || ima_grp == NULL || grp_num == NULL)
		return -1;

	tmp_grp_num = *grp_num;

	for (i = 0; i < tmp_grp_num; i++)
	{
		tmp_index = ima_grp[i].index;

		/* Sort the group before spliting it*/
		qsort(&ima_link[tmp_index], ima_grp[i].link_num,
			 sizeof(IMA_PHY_LINK), sort_link_for_ima_id);

		tmp_val1 = ima_link[tmp_index].p_icp->ima_id;

		/* if meet any link whose value is different from the first
		 * one, we meet a new group and then we split this group and
		 * append the new group to the end of the group array */
		for (j = (tmp_index + 1); j < (tmp_index + (int)ima_grp[i].link_num); j++)
		{
			tmp_val2 = ima_link[j].p_icp->ima_id;
			if (tmp_val2 != tmp_val1)
			{
				/* Create a new group at the ned of the array */
				ima_grp[tmp_grp_num].index = j;
				ima_grp[tmp_grp_num].link_num =
						ima_grp[i].link_num - j + tmp_index;

				/* Update orignal group link number */
				ima_grp[i].link_num = j - tmp_index;

				/* Update total group nmbers */
				tmp_grp_num++;
				break;
			}
		}
		if (tmp_grp_num >= MAX_GRP_NUM) {
			LOGERROR("Error, ima_grp_num >= MAX_GRP_NUM");
			return -1;
		}
	}

	/* Update the tmp value to the original output parameter */
	*grp_num = tmp_grp_num;

	return 0;
}

/******************************************************************************
**
** DESCRIPTION:	This module split each group based on IMA Version.
**
*******************************************************************************/
static int split_for_status_ctrl_ind(IMA_PHY_LINK *ima_link,
			 IMA_GRP *ima_grp, unsigned int *grp_num)
{
	int i, j, tmp_val1, tmp_val2, tmp_index;
	int tmp_grp_num;

	if (ima_link == NULL || ima_grp == NULL || grp_num == NULL)
		return -1;

	tmp_grp_num = *grp_num;

	for (i = 0; i < tmp_grp_num; i++)
	{
		tmp_index = ima_grp[i].index;

		/* Sort the group before spliting it*/
		qsort(&ima_link[tmp_index], ima_grp[i].link_num,
			 sizeof(IMA_PHY_LINK), sort_link_for_status_ctrl_ind);

		tmp_val1 = ima_link[tmp_index].p_icp->status_ctrl_ind;

		/* if meet any link whose value is different from the first
		 * one, we meet a new group and then we split this group and
		 * append the new group to the end of the group array */
		for (j = (tmp_index + 1); j < (tmp_index + (int)ima_grp[i].link_num); j++)
		{
			tmp_val2 = ima_link[j].p_icp->status_ctrl_ind;
			if (tmp_val2 != tmp_val1)
			{
				/* Create a new group at the ned of the array */
				ima_grp[tmp_grp_num].index = j;
				ima_grp[tmp_grp_num].link_num =
					ima_grp[i].link_num - j + tmp_index;

				/* Update orignal group link number */
				ima_grp[i].link_num = j - tmp_index;

				/* Update total group nmbers */
				tmp_grp_num++;
				break;
			}
		}
		if (tmp_grp_num >= MAX_GRP_NUM) {
			LOGERROR("Error, ima_grp_num >= MAX_GRP_NUM");
			return -1;
		}
	}
	/* Update the tmp value to the original output parameter */
	*grp_num = tmp_grp_num;

	return 0;
}
/******************************************************************************
**
** DESCRIPTION:	This module split each group based on IMA Group
**              Status & Control.
**
*******************************************************************************/
static int split_for_grp_status_ctrl(IMA_PHY_LINK *ima_link,
			 IMA_GRP *ima_grp, unsigned int *grp_num)
{
	int i, j, tmp_val1, tmp_val2, tmp_index;
	int tmp_grp_num;

	if (ima_link == NULL || ima_grp == NULL || grp_num == NULL)
		return -1;

	tmp_grp_num = *grp_num;

	for (i = 0; i < tmp_grp_num; i++)
	{
		tmp_index = ima_grp[i].index;

		/* Sort the group before spliting it*/
		qsort(&ima_link[tmp_index], ima_grp[i].link_num,
			 sizeof(IMA_PHY_LINK), sort_link_for_grp_status_ctrl);

		tmp_val1 = ima_link[tmp_index].p_icp->grp_status_ctrl;

		/* if meet any link whose value is different from the first
		 * one, we meet a new group and then we split this group and
		 * append the new group to the end of the group array */
		for (j = (tmp_index + 1); j < (tmp_index + (int)ima_grp[i].link_num); j++)
		{
			tmp_val2 = ima_link[j].p_icp->grp_status_ctrl;
			if (tmp_val2 != tmp_val1)
			{
				/* Create a new group at the ned of the array */
				ima_grp[tmp_grp_num].index = j;
				ima_grp[tmp_grp_num].link_num =
					ima_grp[i].link_num - j + tmp_index;

				/* Update orignal group link number */
				ima_grp[i].link_num = j - tmp_index;

				/* Update total group nmbers */
				tmp_grp_num++;
				break;
			}
		}
		if (tmp_grp_num >= MAX_GRP_NUM) {
			LOGERROR("Error, ima_grp_num >= MAX_GRP_NUM");
			return -1;
		}
	}
	/* Update the tmp value to the original output parameter */
	*grp_num = tmp_grp_num;

	return 0;
}
/******************************************************************************
**
** DESCRIPTION:	This module split each group based on IMA
**              Transmit Timing Information.
**
*******************************************************************************/
static int split_for_tx_timing_info(IMA_PHY_LINK *ima_link,
			 IMA_GRP *ima_grp, unsigned int *grp_num)
{
	int i, j, tmp_val1, tmp_val2, tmp_index;
	int tmp_grp_num;

	if (ima_link == NULL || ima_grp == NULL || grp_num == NULL)
		return -1;

	tmp_grp_num = *grp_num;

	for (i = 0; i < tmp_grp_num; i++)
	{
		tmp_index = ima_grp[i].index;

		/* Sort the group before spliting it*/
		qsort(&ima_link[tmp_index], ima_grp[i].link_num,
			 sizeof(IMA_PHY_LINK), sort_link_for_tx_timing_info);

		tmp_val1 = ima_link[tmp_index].p_icp->tx_timing_info;

		/* if meet any link whose value is different from the first
		 * one, we meet a new group and then we split this group and
		 * append the new group to the end of the group array */
		for (j = (tmp_index + 1); j < (tmp_index + (int)ima_grp[i].link_num); j++)
		{
			tmp_val2 = ima_link[j].p_icp->tx_timing_info;
			if (tmp_val2 != tmp_val1)
			{
				/* Create a new group at the ned of the array */
				ima_grp[tmp_grp_num].index = j;
				ima_grp[tmp_grp_num].link_num =
					ima_grp[i].link_num - j + tmp_index;

				/* Update orignal group link number */
				ima_grp[i].link_num = j - tmp_index;

				/* Update total group nmbers */
				tmp_grp_num++;
				break;
			}
		}
		if (tmp_grp_num >= MAX_GRP_NUM) {
			LOGERROR("Error, ima_grp_num >= MAX_GRP_NUM");
			return -1;
		}
	}
	/* Update the tmp value to the original output parameter */
	*grp_num = tmp_grp_num;

	return 0;
}
/******************************************************************************
**
** DESCRIPTION:	This module split each group based on IMA Tx Test Control.
**
*******************************************************************************/
static int split_for_tx_test_ctrl(IMA_PHY_LINK *ima_link,
			 IMA_GRP *ima_grp, unsigned int *grp_num)
{
	int i, j, tmp_val1, tmp_val2, tmp_index;
	int tmp_grp_num;

	if (ima_link == NULL || ima_grp == NULL || grp_num == NULL)
		return -1;

	tmp_grp_num = *grp_num;

	for (i = 0; i < tmp_grp_num; i++)
	{
		tmp_index = ima_grp[i].index;

		/* Sort the group before spliting it*/
		qsort(&ima_link[tmp_index], ima_grp[i].link_num,
			 sizeof(IMA_PHY_LINK), sort_link_for_tx_test_ctrl);

		tmp_val1 = ima_link[tmp_index].p_icp->tx_test_ctrl;

		/* if meet any link whose value is different from the first
		 * one, we meet a new group and then we split this group and
		 * append the new group to the end of the group array */
		for (j = (tmp_index + 1); j < (tmp_index + (int)ima_grp[i].link_num); j++)
		{
			tmp_val2 = ima_link[j].p_icp->tx_test_ctrl;
			if (tmp_val2 != tmp_val1)
			{
				/* Create a new group at the ned of the array */
				ima_grp[tmp_grp_num].index = j;
				ima_grp[tmp_grp_num].link_num =
					ima_grp[i].link_num - j + tmp_index;

				/* Update orignal group link number */
				ima_grp[i].link_num = j - tmp_index;

				/* Update total group nmbers */
				tmp_grp_num++;
				break;
			}
		}
		if (tmp_grp_num >= MAX_GRP_NUM) {
			LOGERROR("Error, ima_grp_num >= MAX_GRP_NUM");
			return -1;
		}
	}
	/* Update the tmp value to the original output parameter */
	*grp_num = tmp_grp_num;

	return 0;
}
/******************************************************************************
**
** DESCRIPTION:	This module split each group based on IMA Tx Test Pattern.
**
*******************************************************************************/
static int split_for_tx_test_pattern(IMA_PHY_LINK *ima_link,
			 IMA_GRP *ima_grp, unsigned int *grp_num)
{
	int i, j, tmp_val1, tmp_val2, tmp_index;
	int tmp_grp_num;

	if (ima_link == NULL || ima_grp == NULL || grp_num == NULL)
		return -1;

	tmp_grp_num = *grp_num;

	for (i = 0; i < tmp_grp_num; i++)
	{
		tmp_index = ima_grp[i].index;

		/* Sort the group before spliting it*/
		qsort(&ima_link[tmp_index], ima_grp[i].link_num,
			 sizeof(IMA_PHY_LINK), sort_link_for_tx_test_pattern);

		tmp_val1 = ima_link[tmp_index].p_icp->tx_test_pattern;

		/* if meet any link whose value is different from the first
		 * one, we meet a new group and then we split this group and
		 * append the new group to the end of the group array */
		for (j = (tmp_index + 1); j < (tmp_index + (int)ima_grp[i].link_num); j++)
		{
			tmp_val2 = ima_link[j].p_icp->tx_test_pattern;
			if (tmp_val2 != tmp_val1)
			{
				/* Create a new group at the ned of the array */
				ima_grp[tmp_grp_num].index = j;
				ima_grp[tmp_grp_num].link_num =
					ima_grp[i].link_num - j + tmp_index;

				/* Update orignal group link number */
				ima_grp[i].link_num = j - tmp_index;

				/* Update total group nmbers */
				tmp_grp_num++;
				break;
			}
		}
		if (tmp_grp_num >= MAX_GRP_NUM) {
			LOGERROR("Error, ima_grp_num >= MAX_GRP_NUM");
			return -1;
		}
	}
	/* Update the tmp value to the original output parameter */
	*grp_num = tmp_grp_num;

	return 0;
}
/******************************************************************************
**
** DESCRIPTION:	This module split each group based on IMA Rx Test Pattern.
**
*******************************************************************************/
static int split_for_rx_test_pattern(IMA_PHY_LINK *ima_link,
			 IMA_GRP *ima_grp, unsigned int *grp_num)
{
	int i, j, tmp_val1, tmp_val2, tmp_index;
	int tmp_grp_num;

	if (ima_link == NULL || ima_grp == NULL || grp_num == NULL)
		return -1;

	tmp_grp_num = *grp_num;

	for (i = 0; i < tmp_grp_num; i++)
	{
		tmp_index = ima_grp[i].index;

		/* Sort the group before spliting it*/
		qsort(&ima_link[tmp_index], ima_grp[i].link_num,
			 sizeof(IMA_PHY_LINK), sort_link_for_rx_test_pattern);

		tmp_val1 = ima_link[tmp_index].p_icp->rx_test_pattern;

		/* if meet any link whose value is different from the first
		 * one, we meet a new group and then we split this group and
		 * append the new group to the end of the group array */
		for (j = (tmp_index + 1); j < (tmp_index + (int)ima_grp[i].link_num); j++)
		{
			tmp_val2 = ima_link[j].p_icp->rx_test_pattern;
			if (tmp_val2 != tmp_val1)
			{
				/* Create a new group at the ned of the array */
				ima_grp[tmp_grp_num].index = j;
				ima_grp[tmp_grp_num].link_num =
					ima_grp[i].link_num - j + tmp_index;

				/* Update orignal group link number */
				ima_grp[i].link_num = j - tmp_index;

				/* Update total group nmbers */
				tmp_grp_num++;
				break;
			}
		}
		if (tmp_grp_num >= MAX_GRP_NUM) {
			LOGERROR("Error, ima_grp_num >= MAX_GRP_NUM");
			return -1;
		}
	}
	/* Update the tmp value to the original output parameter */
	*grp_num = tmp_grp_num;

	return 0;
}

static int is_same_link_status_ctrl(IMA_PHY_LINK *ima_link,
	int index_i, int index_j)
{
	int i, tmp1, tmp2;

	if (ima_link == NULL)
		return 0;

	for (i = 0; i < 32; i++) {
		tmp1 = ima_link[index_i].p_icp->link_status_ctrl[i];
		tmp2 = ima_link[index_i].p_icp->link_status_ctrl[i];
		if (tmp1 != tmp2)
			return 0;
	}

	return 1;
}

/******************************************************************************
**
** DESCRIPTION:	This module split each group based on IMA
** Link 0-Link31 Information.
**
*******************************************************************************/
static int split_for_link_status_ctrl(IMA_PHY_LINK *ima_link,
			 IMA_GRP *ima_grp, unsigned int *grp_num)
{
	int i, j, tmp_index;
	int tmp_grp_num;

	if (ima_link == NULL || ima_grp == NULL || grp_num == NULL)
		return -1;

	tmp_grp_num = *grp_num;

	for (i = 0; i < tmp_grp_num; i++)
	{
		tmp_index = ima_grp[i].index;

		/* Sort the group before spliting it*/
		qsort(&ima_link[tmp_index], ima_grp[i].link_num,
			 sizeof(IMA_PHY_LINK), sort_link_for_link_status_ctrl);

		/* if meet any link whose value is different from the first
		 * one, we meet a new group and then we split this group and
		 * append the new group to the end of the group array */
		for (j=tmp_index + 1; j<tmp_index + (int)ima_grp[i].link_num; j++)
		{
			if (!is_same_link_status_ctrl(ima_link, tmp_index, j))
			{
				/* Create a new group at the ned of the array */
				ima_grp[tmp_grp_num].index = j;
				ima_grp[tmp_grp_num].link_num =
					ima_grp[i].link_num - j + tmp_index;

				/* Update orignal group link number */
				ima_grp[i].link_num = j - tmp_index;

				/* Update total group nmbers */
				tmp_grp_num++;
				break;
			}
		}
		if (tmp_grp_num >= MAX_GRP_NUM) {
			LOGERROR("Error, ima_grp_num >= MAX_GRP_NUM");
			return -1;
		}
	}
	/* Update the tmp value to the original output parameter */
	*grp_num = tmp_grp_num;

	return 0;
}

static int rcv_icp_from_link(unsigned char *pdata, IMA_PHY_LINK *ima_link)
{
	pkt_hdr *ph = (pkt_hdr *)pdata;
	IMA_ICP *p_icp = NULL;
	int phy_link;

	phy_link = (int)pkthdr_get_channel(ph);

	if ((phy_link > MAX_LNK_NUM) || (phy_link < 0))
		return -1;

	p_icp = is_icp_cell(pkthdr_get_data(ph));
	if (p_icp == NULL)
		return -1;

	LOGDEBUG("ICP Cell:"
		" IMAID[%u] LinkID[%u] PhyLink[%u] IFSN[%u] M[%u] Offset[%u]",
		p_icp->ima_id, p_icp->cell_link_id & 0x7F, phy_link, p_icp->ifsn,
		ima_m[p_icp->grp_status_ctrl & 0x03], p_icp->icp_off);

	if (ima_link[phy_link].rcv_nr == 0) {
		ima_link[phy_link].p_icp = p_icp;
		ima_link[phy_link].phy_nr = phy_link;
		ima_link[phy_link].rcv_nr++;
		return 0;
	} else if (is_link_stable(ima_link[phy_link].p_icp, p_icp)) {
		free(ima_link[phy_link].p_icp);
		ima_link[phy_link].p_icp = p_icp;
		ima_link[phy_link].rcv_nr++;
		return 0;
	} else {
		ima_link[phy_link].rcv_nr = 0;
		LOGERROR("PhyLink[%u] unstable.", phy_link);
		if (p_icp)
			free(p_icp);
		return -1;
	}
}

#define icpvalue(i, fields) (ima_link[ima_grp[i].index].p_icp->fields)

static int imascan_outcfg(IMA_GRP *ima_grp, unsigned int grp_num,
	   IMA_PHY_LINK *ima_link, unsigned int link_num, IMA_CFG *imaCfg)
{
	unsigned int i, j;
	int phy;

	imaCfg->grpNumAll = 0;
	imaCfg->groupNum = 0;
	imaCfg->tcNum = 0;

	for (i = 0; i < grp_num; i++) {
		if (ima_grp[i].ok == 0)
			continue;
		imaCfg->grpMode[i] = 2;
		imaCfg->grpCfg[i].clockMd    = (icpvalue(i, tx_timing_info) & 0x20) + 1;
		imaCfg->grpCfg[i].symmetryMd  = icpvalue(i, grp_status_ctrl) & 0x0C;
		imaCfg->grpCfg[i].imaVersion  = icpvalue(i, ver) == 3 ? 0 : 1;
		imaCfg->grpCfg[i].imaOamLabel = icpvalue(i, ver);
		imaCfg->grpCfg[i].rxImaIdCfgEn= 0;
		imaCfg->grpCfg[i].rxImaIdCfg  = 0;
		imaCfg->grpCfg[i].txImaId     = i + 1;
		imaCfg->grpCfg[i].txVphy      = i;
		imaCfg->grpCfg[i].rxVphy      = i;
		imaCfg->grpCfg[i].rxM         = 0xf;
		imaCfg->grpCfg[i].pRx         = 2;
		imaCfg->grpCfg[i].pTx         = 2;
		imaCfg->grpCfg[i].txM         = icpvalue(i, grp_status_ctrl) & 0x03;
		imaCfg->grpCfg[i].maxDelay    = 1023;
		imaCfg->grpCfg[i].delayGb     = 10;
		imaCfg->grpCfg[i].delayAddEn  = i;
		imaCfg->grpCfg[i].stuffMd     = imaCfg->grpCfg[i].clockMd;
		imaCfg->grpCfg[i].stuffAdvMd  = 1;
		imaCfg->grpCfg[i].numRxLinks  = ima_grp[i].link_num;
		imaCfg->grpCfg[i].numTxLinks  = ima_grp[i].link_num;

		for (j = 0; j < ima_grp[i].link_num; j++) {
			imaCfg->grpId[i] = ima_link[ima_grp[i].index+j].p_icp->ima_id;
			phy = ima_link[ima_grp[i].index+j].phy_nr;
			phy = phy + ((phy / 21) * 7);
			imaCfg->grpCfg[i].rxLinks[j] = phy;

			LOGDEBUG("Group[%u]: Index[%d] LinkNum[%d] Link[%d]: phy_nr %d, phy %d",
					i, ima_grp[i].index, ima_grp[i].link_num,
					j, ima_link[ima_grp[i].index+j].phy_nr, phy);

			imaCfg->grpCfg[i].txLinks[j].lid       =
						ima_link[ima_grp[i].index+j].p_icp->cell_link_id & 0x1F;
			imaCfg->grpCfg[i].txLinks[j].icpOffset =
						ima_link[ima_grp[i].index+j].p_icp->icp_off;
			imaCfg->grpCfg[i].txLinks[j].phyLink   = phy;
		}
		imaCfg->grpCfg[i].txTrl       = imaCfg->grpCfg[i].txLinks[0].phyLink;

		(imaCfg->grpNumAll)++;
		(imaCfg->groupNum)++;
	}

	return 0;
}

static int imascan_result(IMA_PHY_LINK *ima_link, IMA_CFG *imacfg)
{
	IMA_GRP ima_grp[MAX_GRP_NUM];
	unsigned int grp_num = 0;
	unsigned int link_num = 0;
	unsigned int i, tmp_val1, tmp_val2;

	/* Init group information */
	for (i = 0; i < MAX_GRP_NUM; i++) {
		ima_grp[i].index = 0;
		ima_grp[i].link_num = 0;
		ima_grp[i].ok = 0;
	}

	/* Get the total number of all the links that recieved ICP cells */
	for (i = 0; i < MAX_LNK_NUM; i++)
		if(ima_link[i].p_icp != NULL)
			link_num++;

	if (link_num == 0) {
		LOGERROR("Err:No link recieved IMA cells\n");
		return -1;
	}

	ima_grp[0].index = 0;
	ima_grp[0].link_num = link_num;
	grp_num = 1;

	/* Split based on IMA version must be called as the first one
	 * becuase the actual link number are counted int this split function */
	if (split_for_ima_ver(ima_link, ima_grp, &grp_num) < 0)
		LOGERROR("Error: split_for_ima_ver\n");

	if (split_for_ima_id(ima_link, ima_grp, &grp_num) < 0)
		LOGERROR("Error: split_for_ima_id\n");

	if (split_for_status_ctrl_ind(ima_link, ima_grp, &grp_num) < 0)
		LOGERROR("Error: split_for_status_ctrl_ind\n");

	if (split_for_grp_status_ctrl(ima_link, ima_grp, &grp_num) < 0)
		LOGERROR("Error: split_for_grp_status_ctrl\n");

	if (split_for_tx_timing_info(ima_link, ima_grp, &grp_num) < 0)
		LOGERROR("Error: split_for_tx_timing_info\n");

	if (split_for_tx_test_ctrl(ima_link, ima_grp, &grp_num) < 0)
		LOGERROR("Error: split_for_tx_test_ctrl\n");

	if (split_for_tx_test_pattern(ima_link, ima_grp, &grp_num) < 0)
		LOGERROR("Error: split_for_tx_test_pattern\n");

	if (split_for_rx_test_pattern(ima_link, ima_grp, &grp_num) < 0)
		LOGERROR("Error: split_for_rx_test_pattern\n");

	if (split_for_link_status_ctrl(ima_link, ima_grp, &grp_num) < 0)
		LOGERROR("Error: split_for_link_status_ctrl\n");

	for (i = 0; i < grp_num; i++) {
		qsort(&ima_link[ima_grp[i].index], ima_grp[i].link_num,
                         sizeof(IMA_PHY_LINK), sort_link_for_ifsn);
	}

	for (i = 0; i < grp_num; i++)
	{
		tmp_val1 = ima_link[ima_grp[i].index].p_icp->ifsn;
		tmp_val2 = ima_link[ima_grp[i].index+ima_grp[i].link_num-1].p_icp->ifsn;
		if ((tmp_val2 - tmp_val1) <= 1)
			ima_grp[i].ok = 1;
	}

	for (i = 0; i < grp_num; i++) {
		qsort(&ima_link[ima_grp[i].index], ima_grp[i].link_num,
                         sizeof(IMA_PHY_LINK), sort_link_for_phylink);
	}

	for (i = 0; i< link_num; i++) {
		LOGDEBUG("%u LinkID[%u]: IMAID[%u] PhyLink[%u]"
			" IFSN[%u] Offset[%u] M[%u]", i,
			ima_link[i].p_icp->cell_link_id & 0x7F,
			ima_link[i].p_icp->ima_id,
			ima_link[i].phy_nr,
			ima_link[i].p_icp->ifsn,
			ima_link[i].p_icp->icp_off,
			ima_m[ima_link[i].p_icp->grp_status_ctrl & 0x03]);
	}

	for (i = 0; i < grp_num; i++)
	{
		LOGDEBUG("Group[%u]: Index[%u] LinkNum[%u] %s", i, ima_grp[i].index,
			ima_grp[i].link_num, ima_grp[i].ok ? "OK" : "...");
	}

	if (imascan_outcfg(ima_grp, grp_num, ima_link, link_num, imacfg) < 0)
		return -1;

	return 0;
}

void *imascan_open(int max_channel, int max_icpnum)
{
	int i;
	IMA_PROBE *hd;
	IMA_PHY_LINK *imaplink = NULL;

	if ((max_channel <= 0) || (max_channel > 256))
		return NULL;

	hd = (IMA_PROBE *)malloc(sizeof(*hd));
	if (hd == NULL)
		return NULL;

	hd->icpcount = 0;
	hd->maxicp	= max_icpnum;
	hd->imaplink = (IMA_PHY_LINK *)malloc(max_channel * sizeof(IMA_PHY_LINK));
	if (hd->imaplink == NULL) {
		free(hd);
		return NULL;
	}

	/*Init physical link information */
	for (i = 0, imaplink = hd->imaplink; i < max_channel; i++, imaplink++) {
		memset(imaplink, 0, sizeof(IMA_PHY_LINK));
		imaplink->phy_nr = i;
		imaplink->rcv_nr = 0;
		if (imaplink->p_icp)
			free(imaplink->p_icp);
		imaplink->p_icp = NULL;
	}

	return hd;
}

void *imascan_proc(void *handle, unsigned char *pktcell)
{
	IMA_CFG *imacfg = NULL;
	IMA_PROBE *hd = (IMA_PROBE *)handle;

	/* Recieve ICP cells from all the links */
	if (rcv_icp_from_link(pktcell, hd->imaplink) < 0)
		return NULL;

	hd->icpcount++;
	if (hd->icpcount < hd->maxicp)
		return NULL;

	imacfg = malloc(sizeof(IMA_CFG));
	if (imacfg == NULL)
		return NULL;

	memset(imacfg, 0, sizeof(IMA_CFG));
	if (imascan_result(hd->imaplink, imacfg) < 0) {
		hd->icpcount = 0;
		free(imacfg);
		return NULL;
	}

	return imacfg;
}

void icpinfo_print(IMA_PHY_LINK *ima_link)
{
	int i, j;
	unsigned int link_num = 0;

	for (i = 0; i < MAX_LNK_NUM; i++) {
		if(ima_link[i].p_icp != NULL)
			link_num++;
	}

	if (link_num == 0) {
		LOGERROR("Err:No link recieved IMA cells\n");
		return;
	}

	i = 0;
	j = 0;
	while (j < link_num) {
		if (ima_link[i].p_icp == NULL) {
			i++;
			continue;
		}
		printf("PhyLink %2u, LinkID %2u, IMAID %2u, IFSN %3u,"
				" Offset %2u, M %3u\n",
				ima_link[i].phy_nr,
				ima_link[i].p_icp->cell_link_id & 0x7F,
				ima_link[i].p_icp->ima_id,
				ima_link[i].p_icp->ifsn,
				ima_link[i].p_icp->icp_off,
				ima_m[ima_link[i].p_icp->grp_status_ctrl & 0x03]);

		i++;
		j++;
	}
}

int icpinfo(void *handle, unsigned char *pktcell)
{
	IMA_PROBE *hd = (IMA_PROBE *)handle;

	/* Recieve ICP cells from all the links */
	if (rcv_icp_from_link(pktcell, hd->imaplink) < 0)
		return 0;

	hd->icpcount++;
	if (hd->icpcount < hd->maxicp)
		return 0;

	icpinfo_print(hd->imaplink);

	return 1;
}

void imascan_close(void *handle)
{
	IMA_PROBE *hd = (IMA_PROBE *)handle;

	if (hd) {
		if (hd->imaplink)
			free(hd->imaplink);
		free(hd);
	}
}

