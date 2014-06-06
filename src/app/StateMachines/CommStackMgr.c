/*****************************************************************************
* Model: CommStackMgr.qm
* File:  ./CommStackMgr_gen.c
*
* This file has been generated automatically by QP Modeler (QM).
* DO NOT EDIT THIS FILE MANUALLY.
*
* Please visit www.state-machine.com/qm for more information.
*****************************************************************************/
/**
 * @file    CommStackMgr.c
 * Declarations for functions for the CommStackMgr AO.  This state
 * machine handles all incoming messages and their distribution for the
 * bootloader.
 *
 * Note: If editing this file, please make sure to update the CommStackMgr.qm
 * model.  The generated code from that model should be very similar to the
 * code in this file.
 *
 * @date    05/27/2014
 * @author  Harry Rostovtsev
 * @email   harry_rostovtsev@datacard.com
 * Copyright (C) 2014 Datacard. All rights reserved.
 */
#include "CommStackMgr.h"
#include "CBSignals.h"                              /* Signal declarations. */
#include "SerialMgr.h"                     /* For SerialDataEvt declaration */
#include "bsp.h"                            /* For time to ticks conversion */
#include "time.h"                                 /* For time functionality */
#include <stdio.h>                              /* For snprintf declaration */

Q_DEFINE_THIS_FILE;

/* application signals cannot overlap the device-driver signals */
Q_ASSERT_COMPILE(MAX_SHARED_SIG < DEV_DRIVER_SIG);

#define LWIP_ALLOWED
#define USER_FLASH_FIRST_PAGE_ADDRESS                              0x08020000
#define BOOT_APP_TIMEOUT                                           30000

typedef void (*pFunction)(void);
pFunction Jump_To_Application;
uint32_t JumpAddress;

/* Active object class -----------------------------------------------------*/
/* @(/2/0) .................................................................*/
/** 
* MtrDiag Active Object
*/
typedef struct CommStackMgrTag {
/* protected: */
    QActive super;
    /** 
    * Used to timeout for testing the clock.
    */
    QTimeEvt timeTestTimerEvt;
} CommStackMgr;

/* protected: */
static QState CommStackMgr_initial(CommStackMgr * const me, QEvt const * const e);

/** 
* This state is a catch-all Active state.  If any signals need
* to be handled that do not cause state transitions and are 
* common to the entire AO, they should be handled here.
*  
* @param  me: Pointer to the Interstage state machine
* @param  e:  Pointer to the event being processed.
* @retval status: QState type that specifies where the state
* machine is going next.
* 
*/
static QState CommStackMgr_Active(CommStackMgr * const me, QEvt const * const e);


/* Local objects -----------------------------------------------------------*/
static CommStackMgr l_CommStackMgr; /* the single instance of the Interstage active object */

/* Global-scope objects ----------------------------------------------------*/
QActive * const AO_CommStackMgr = (QActive *)&l_CommStackMgr;  /* "opaque" AO pointer */

/* @(/2/2) .................................................................*/
void CommStackMgr_ctor(void) {
    CommStackMgr *me = &l_CommStackMgr;
    QActive_ctor(&me->super, (QStateHandler)&CommStackMgr_initial);
    QTimeEvt_ctor(&me->timeTestTimerEvt, TIME_TEST_SIG);
}
/* @(/2/0) .................................................................*/
/* @(/2/0/1) ...............................................................*/
/* @(/2/0/1/0) */
static QState CommStackMgr_initial(CommStackMgr * const me, QEvt const * const e) {
    (void)e;        /* suppress the compiler warning about unused parameter */

    QS_OBJ_DICTIONARY(&l_CommStackMgr);
    QS_FUN_DICTIONARY(&QHsm_top);
    QS_FUN_DICTIONARY(&CommStackMgr_initial);
    QS_FUN_DICTIONARY(&CommStackMgr_Running);

    QActive_subscribe((QActive *)me, MSG_SEND_OUT_SIG);
    QActive_subscribe((QActive *)me, MSG_RECEIVED_SIG);
    QActive_subscribe((QActive *)me, TIME_TEST_SIG);
    return Q_TRAN(&CommStackMgr_Active);
}
/* @(/2/0/1/1) .............................................................*/
static QState CommStackMgr_Active(CommStackMgr * const me, QEvt const * const e) {
    QState status;
    switch (e->sig) {
        /* @(/2/0/1/1) */
        case Q_ENTRY_SIG: {
            /* Send a test debug msg */
            /* 1. Construct a new msg event indicating that a msg has been received */
            SerialDataEvt *serDataEvt = Q_NEW(SerialDataEvt, UART_DMA_START_SIG);

            /* 2. Fill the msg payload and get the msg source and length */
            serDataEvt->wBufferLen = snprintf(
                serDataEvt->buffer,
                MAX_MSG_LEN,
                "Test debug msg from CommStackMgr\n"
            );

            QF_PUBLISH((QEvent *)serDataEvt, AO_CommStackMgr);

            SerialDataEvt *serDataEvt1 = Q_NEW(SerialDataEvt, UART_DMA_START_SIG);

            /* 2. Fill the msg payload and get the msg source and length */
            serDataEvt1->wBufferLen = snprintf(
                serDataEvt1->buffer,
                MAX_MSG_LEN,
                "Another Test debug msg from CommStackMgr\n"
            );

            QF_PUBLISH((QEvent *)serDataEvt1, AO_CommStackMgr);

            /* Every 2 seconds, post an event to print time */
            QTimeEvt_postEvery(
                &me->timeTestTimerEvt,
                (QActive *)me,
                SEC_TO_TICKS( 2 )
            );
            status = Q_HANDLED();
            break;
        }
        /* @(/2/0/1/1/0) */
        case MSG_SEND_OUT_SIG: {
            status = Q_HANDLED();
            break;
        }
        /* @(/2/0/1/1/1) */
        case MSG_RECEIVED_SIG: {
            status = Q_HANDLED();
            break;
        }
        /* @(/2/0/1/1/2) */
        case TIME_TEST_SIG: {
            /* 1. Construct a new msg event indicating that a msg has been received */
            /*SerialDataEvt *serDataEvt = Q_NEW(SerialDataEvt, UART_DMA_START_SIG);

            t_Time time = TIME_getTime();


            serDataEvt->wBufferLen = snprintf(
                serDataEvt->buffer,
                MAX_MSG_LEN,
                "Time is now %02d:%02d:%02d:%d\n",
                time.hour_min_sec.RTC_Hours,
                time.hour_min_sec.RTC_Minutes,
                time.hour_min_sec.RTC_Seconds,
                (int)time.sub_sec
            );

            QF_PUBLISH((QEvent *)serDataEvt, AO_CommStackMgr); */
            status = Q_HANDLED();
            break;
        }
        default: {
            status = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status;
}


/******** Copyright (C) 2014 Datacard. All rights reserved *****END OF FILE****/
