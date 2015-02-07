

#include "int.h"
#include "mib.h"
#include "tmacro.h"
#include "mac.h"
#include "power.h"
#include "bssdb.h"
#include "usbpipe.h"

/*---------------------  Static Definitions -------------------------*/
/* static int msglevel = MSG_LEVEL_DEBUG; */
static int msglevel = MSG_LEVEL_INFO;


/*---------------------  Static Classes  ----------------------------*/

/*---------------------  Static Variables  --------------------------*/

/*---------------------  Static Functions  --------------------------*/

/*---------------------  Export Variables  --------------------------*/


/*---------------------  Export Functions  --------------------------*/


void INTvWorkItem(void *Context)
{
	PSDevice pDevice = (PSDevice) Context;
	NTSTATUS ntStatus;

	DBG_PRT(MSG_LEVEL_DEBUG, KERN_INFO"---->Interrupt Polling Thread\n");

	spin_lock_irq(&pDevice->lock);
	if (pDevice->fKillEventPollingThread != TRUE)
		ntStatus = PIPEnsInterruptRead(pDevice);
	spin_unlock_irq(&pDevice->lock);
}

NTSTATUS
INTnsProcessData(PSDevice pDevice)
{
	NTSTATUS	status = STATUS_SUCCESS;
	PSINTData	pINTData;
	PSMgmtObject	pMgmt = &(pDevice->sMgmtObj);
	struct net_device_stats *pStats = &pDevice->stats;

	DBG_PRT(MSG_LEVEL_DEBUG, KERN_INFO"---->s_nsInterruptProcessData\n");

	pINTData = (PSINTData) pDevice->intBuf.pDataBuf;
	if (pINTData->byTSR0 & TSR_VALID) {
		STAvUpdateTDStatCounter(&(pDevice->scStatistic),
					(BYTE) (pINTData->byPkt0 & 0x0F),
					(BYTE) (pINTData->byPkt0>>4),
					pINTData->byTSR0);
		BSSvUpdateNodeTxCounter(pDevice,
					&(pDevice->scStatistic),
					pINTData->byTSR0,
					pINTData->byPkt0);
		/*DBG_PRN_GRP01(("TSR0 %02x\n", pINTData->byTSR0));*/
	}
	if (pINTData->byTSR1 & TSR_VALID) {
		STAvUpdateTDStatCounter(&(pDevice->scStatistic),
					(BYTE) (pINTData->byPkt1 & 0x0F),
					(BYTE) (pINTData->byPkt1>>4),
					pINTData->byTSR1);
		BSSvUpdateNodeTxCounter(pDevice,
					&(pDevice->scStatistic),
					pINTData->byTSR1,
					pINTData->byPkt1);
		/*DBG_PRN_GRP01(("TSR1 %02x\n", pINTData->byTSR1));*/
	}
	if (pINTData->byTSR2 & TSR_VALID) {
		STAvUpdateTDStatCounter(&(pDevice->scStatistic),
					(BYTE) (pINTData->byPkt2 & 0x0F),
					(BYTE) (pINTData->byPkt2>>4),
					pINTData->byTSR2);
		BSSvUpdateNodeTxCounter(pDevice,
					&(pDevice->scStatistic),
					pINTData->byTSR2,
					pINTData->byPkt2);
		/*DBG_PRN_GRP01(("TSR2 %02x\n", pINTData->byTSR2));*/
	}
	if (pINTData->byTSR3 & TSR_VALID) {
		STAvUpdateTDStatCounter(&(pDevice->scStatistic),
					(BYTE) (pINTData->byPkt3 & 0x0F),
					(BYTE) (pINTData->byPkt3>>4),
					pINTData->byTSR3);
		BSSvUpdateNodeTxCounter(pDevice,
					&(pDevice->scStatistic),
					pINTData->byTSR3,
					pINTData->byPkt3);
		/*DBG_PRN_GRP01(("TSR3 %02x\n", pINTData->byTSR3));*/
	}
	if (pINTData->byISR0 != 0) {
		if (pINTData->byISR0 & ISR_BNTX) {
			if (pDevice->eOPMode == OP_MODE_AP) {
				if (pMgmt->byDTIMCount > 0) {
					pMgmt->byDTIMCount--;
					pMgmt->sNodeDBTable[0].bRxPSPoll =
						FALSE;
				} else if (pMgmt->byDTIMCount == 0) {
					/* check if mutltcast tx bufferring */
					pMgmt->byDTIMCount =
						pMgmt->byDTIMPeriod-1;
					pMgmt->sNodeDBTable[0].bRxPSPoll = TRUE;
					if (pMgmt->sNodeDBTable[0].bPSEnable)
						bScheduleCommand((void *) pDevice,
								 WLAN_CMD_RX_PSPOLL,
								 NULL);
				}
				bScheduleCommand((void *) pDevice,
						WLAN_CMD_BECON_SEND,
						NULL);
			} /* if (pDevice->eOPMode == OP_MODE_AP) */
		pDevice->bBeaconSent = TRUE;
		} else {
			pDevice->bBeaconSent = FALSE;
		}
		if (pINTData->byISR0 & ISR_TBTT) {
			if (pDevice->bEnablePSMode)
				bScheduleCommand((void *) pDevice,
						WLAN_CMD_TBTT_WAKEUP,
						NULL);
			if (pDevice->bChannelSwitch) {
				pDevice->byChannelSwitchCount--;
				if (pDevice->byChannelSwitchCount == 0)
					bScheduleCommand((void *) pDevice,
							WLAN_CMD_11H_CHSW,
							NULL);
			}
		}
		LODWORD(pDevice->qwCurrTSF) = pINTData->dwLoTSF;
		HIDWORD(pDevice->qwCurrTSF) = pINTData->dwHiTSF;
		/*DBG_PRN_GRP01(("ISR0 = %02x ,
				LoTsf =  %08x,
				HiTsf =  %08x\n",
				pINTData->byISR0,
				pINTData->dwLoTSF,
				pINTData->dwHiTSF)); */

		STAvUpdate802_11Counter(&pDevice->s802_11Counter,
					&pDevice->scStatistic,
					pINTData->byRTSSuccess,
					pINTData->byRTSFail,
					pINTData->byACKFail,
					pINTData->byFCSErr);
		STAvUpdateIsrStatCounter(&pDevice->scStatistic,
					pINTData->byISR0,
					pINTData->byISR1);
	}

	if (pINTData->byISR1 != 0)
		if (pINTData->byISR1 & ISR_GPIO3)
			bScheduleCommand((void *) pDevice,
					WLAN_CMD_RADIO,
					NULL);
	pDevice->intBuf.uDataLen = 0;
	pDevice->intBuf.bInUse = FALSE;

	pStats->tx_packets = pDevice->scStatistic.ullTsrOK;
	pStats->tx_bytes = pDevice->scStatistic.ullTxDirectedBytes +
			pDevice->scStatistic.ullTxMulticastBytes +
			pDevice->scStatistic.ullTxBroadcastBytes;
	pStats->tx_errors = pDevice->scStatistic.dwTsrErr;
	pStats->tx_dropped = pDevice->scStatistic.dwTsrErr;

	return status;
}
