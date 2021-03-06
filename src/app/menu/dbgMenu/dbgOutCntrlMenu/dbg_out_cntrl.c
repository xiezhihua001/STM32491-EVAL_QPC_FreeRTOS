/**
 * @file    dbg_out_cntrl.c
 * @brief   Handles the debug output control menu items of the menu application.
 *
 * @date    10/13/2014
 * @author  Harry Rostovtsev
 * @email   rost0031@gmail.com
 * Copyright (C) 2014 Harry Rostovtsev. All rights reserved.
 *
 * @addtogroup groupMenu
 * @{
 */
/* Includes ------------------------------------------------------------------*/
#include "qp_port.h"                                        /* for QP support */
#include "project_includes.h"
#include "dbg_out_cntrl.h"
#include "LWIPMgr.h"

/* Compile-time called macros ------------------------------------------------*/
Q_DEFINE_THIS_FILE                  /* For QSPY to know the name of this file */
DBG_DEFINE_THIS_MODULE( DBG_MODL_DBG );/* For debug system to ID this module */

/* Private typedefs ----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private variables and Local objects ---------------------------------------*/

treeNode_t menuDbgOutCntrl;
char *const menuDbgOutCntrl_TitleTxt = "Debug Output Control Menu";
char *const menuDbgOutCntrl_SelectKey = "OUT";

treeNode_t menuDbgOutCntrlItem_toggleSerialDebug;
char *const menuDbgOutCntrlItem_toggleSerialDebugTxt =
      "Toggle debugging over serial port ON/OFF";
char *const menuDbgOutCntrlItem_toggleSerialDebugSelectKey = "S";

treeNode_t menuDbgOutCntrlItem_toggleEthDebug;
char *const menuDbgOutCntrlItem_toggleEthDebugTxt =
      "Toggle debugging over ethernet port ON/OFF";
char *const menuDbgOutCntrlItem_toggleEthDebugSelectKey = "E";

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
void MENU_toggleSerialDebugAction(
      const char* dataBuf,
      uint16_t dataLen,
      MsgSrc dst
)
{
   MENU_printf(dst, "Toggling debugging output over Serial:\n");
   QEvt *qEvt = Q_NEW( QEvt, UART_DMA_DBG_TOGGLE_SIG );
   QF_PUBLISH(qEvt, AO_DbgMgr);
   LOG_printf("Toggle Serial Debug Action test\n");
}

/******************************************************************************/
void MENU_toggleEthDebugAction(
      const char* dataBuf,
      uint16_t dataLen,
      MsgSrc dst
)
{
   MENU_printf(dst, "Toggling debugging output over Ethernet:\n");
   QEvt *qEvt = Q_NEW( QEvt, ETH_DBG_TOGGLE_SIG );
   QF_PUBLISH(qEvt, AO_DbgMgr);
   LOG_printf("Toggle Ethernet Debug Action test\n");

//   DBG_printf("Toggle Ethernet Debug Action test\n");
//   EthEvt *ethEvt = Q_NEW(EthEvt, ETH_LOG_TCP_SEND_SIG);
//   char tmp[] = "Testing Ethernet TCP send function\n";
//   ethEvt->msg_len = sizeof(tmp);
//   MEMCPY(ethEvt->msg, tmp, ethEvt->msg_len);
//   ethEvt->msg_src = 0;
//   QF_PUBLISH((QEvent *)ethEvt, AO_DbgMgr);
//
//
//   MENU_printf(ETH_PORT_LOG, "Test1\n");
//   MENU_printf(ETH_PORT_LOG, "Test2\n");
//   MENU_printf(ETH_PORT_LOG, "Test3\n");
//   MENU_printf(ETH_PORT_LOG, "Test4\n");
//   MENU_printf(ETH_PORT_LOG, "Test5\n");
//   MENU_printf(ETH_PORT_LOG, "Test6\n");
}

/**
 * @}
 * end addtogroup groupMenu
 */

/******** Copyright (C) 2014 Harry Rostovtsev. All rights reserved *****END OF FILE****/
