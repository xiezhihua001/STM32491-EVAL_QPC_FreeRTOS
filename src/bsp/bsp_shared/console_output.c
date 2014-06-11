/**
 * @file   console_output.c
 * This file contains the definitions for debug and output functions over the
 * serial dma console.
 *
 * @date   06/09/2014
 * @author Harry Rostovtsev
 * @email  harry_rostovtsev@datacard.com
 * Copyright (C) 2014 Datacard. All rights reserved.
 */
#include "console_output.h"
#include "qp_port.h"                                        /* for QP support */
#include "CBSignals.h"
#include "CBErrors.h"
#include "Shared.h"
#include "time.h"
#include "SerialMgr.h"

/******************************************************************************/
void CON_output(
      DEBUG_LEVEL_T dbgLvl,
      char *pFilePath,
      char *pFuncName,
      uint16_t wLineNumber,
      const char *fmt,
      ...
)
{
   /* 1. Get the time first so the printout of the event is as close as possible
    * to when it actually occurred */
   t_Time time = TIME_getTime();

   /* 2. Construct a new msg event pointer and allocate storage in the QP event
    * pool */
   SerialDataEvt *serDataEvt = Q_NEW(SerialDataEvt, UART_DMA_START_SIG);
   serDataEvt->wBufferLen = 0;

   /* 3. Based on the debug level specified by the calling macro, decide what to
    * prepend (if anything). */
   switch (dbgLvl) {
      case DBG:
         serDataEvt->wBufferLen += snprintf(
               &serDataEvt->buffer[serDataEvt->wBufferLen],
               MAX_MSG_LEN,
               "DBG-%02d:%02d:%02d:%05d-%s():%d:",
               time.hour_min_sec.RTC_Hours,
               time.hour_min_sec.RTC_Minutes,
               time.hour_min_sec.RTC_Seconds,
               (int)time.sub_sec,
               pFuncName,
               wLineNumber
         );
         break;
      case LOG:
         serDataEvt->wBufferLen += snprintf(
               &serDataEvt->buffer[serDataEvt->wBufferLen],
               MAX_MSG_LEN,
               "LOG-%02d:%02d:%02d:%05d-%s():%d:",
               time.hour_min_sec.RTC_Hours,
               time.hour_min_sec.RTC_Minutes,
               time.hour_min_sec.RTC_Seconds,
               (int)time.sub_sec,
               pFuncName,
               wLineNumber
         );
         break;
      case WRN:
         serDataEvt->wBufferLen += snprintf(
               &serDataEvt->buffer[serDataEvt->wBufferLen],
               MAX_MSG_LEN,
               "WRN-%02d:%02d:%02d:%05d-%s():%d:",
               time.hour_min_sec.RTC_Hours,
               time.hour_min_sec.RTC_Minutes,
               time.hour_min_sec.RTC_Seconds,
               (int)time.sub_sec,
               pFuncName,
               wLineNumber
         );
         break;
      case ERR:
         serDataEvt->wBufferLen += snprintf(
               &serDataEvt->buffer[serDataEvt->wBufferLen],
               MAX_MSG_LEN,
               "ERR-%02d:%02d:%02d:%05d-%s():%d:",
               time.hour_min_sec.RTC_Hours,
               time.hour_min_sec.RTC_Minutes,
               time.hour_min_sec.RTC_Seconds,
               (int)time.sub_sec,
               pFuncName,
               wLineNumber
         );
         break;
      case CON: // This is used to print menu so don't prepend anything
      default:
         break;
   }

   /* 4. Pass the va args list to get output to a buffer */
   va_list args;
   va_start(args, fmt);

   /* 5. Print the actual user supplied data to the buffer and set the length */
   serDataEvt->wBufferLen += vsnprintf(
         &serDataEvt->buffer[serDataEvt->wBufferLen],
         MAX_MSG_LEN,
         fmt,
         args
   );
   va_end(args);

   /* 6. Publish the event*/
   QF_PUBLISH((QEvent *)serDataEvt, 0);
}