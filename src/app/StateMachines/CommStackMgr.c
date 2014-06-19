/*****************************************************************************
* Model: CommStackMgr.qm
* File:  ./CommStackMgr_gen.c
*
* This code has been generated by QM tool (see state-machine.com/qm).
* DO NOT EDIT THIS FILE MANUALLY. All your changes will be lost.
*
* This program is open source software: you can redistribute it and/or
* modify it under the terms of the GNU General Public License as published
* by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
* or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
* for more details.
*****************************************************************************/
/*${.::CommStackMgr_gen.c} .................................................*/
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
#include "project_includes.h"         /* Includes common to entire project. */
#include "bsp.h"                            /* For time to ticks conversion */

Q_DEFINE_THIS_FILE;

#define LWIP_ALLOWED
#define USER_FLASH_FIRST_PAGE_ADDRESS                              0x08020000
#define BOOT_APP_TIMEOUT                                           30000

typedef void (*pFunction)(void);
pFunction Jump_To_Application;
uint32_t JumpAddress;

/* Active object class -----------------------------------------------------*/

/**
 * \brief CommStackMgr "class"
 */
/*${AOs::CommStackMgr} .....................................................*/
typedef struct {
/* protected: */
    QActive super;

    /**< Local timer for testing the clock. */
    QTimeEvt timeTestTimerEvt;
} CommStackMgr;

/* protected: */
static QState CommStackMgr_initial(CommStackMgr * const me, QEvt const * const e);

/**
 * This state is a catch-all Active state.  If any signals need
 * to be handled that do not cause state transitions and are
 * common to the entire AO, they should be handled here.
 *
 * @param  [in|out] me: Pointer to the state machine
 * @param  [in|out]  e:  Pointer to the event being processed.
 * @return status: QState type that specifies where the state
 * machine is going next.
 */
static QState CommStackMgr_Active(CommStackMgr * const me, QEvt const * const e);


/* Local objects -----------------------------------------------------------*/
static CommStackMgr l_CommStackMgr; /* the single instance of the Interstage active object */

/* Global-scope objects ----------------------------------------------------*/
QActive * const AO_CommStackMgr = (QActive *)&l_CommStackMgr;  /* "opaque" AO pointer */


/**
 * @brief C "constructor" for CommStackMgr "class".
 * Initializes all the timers and queues used by the AO and sets of the
 * first state.
 * @param  None
 * @param  None
 * @retval None
 */
/*${AOs::CommStackMgr_ctor} ................................................*/
void CommStackMgr_ctor(void) {
    CommStackMgr *me = &l_CommStackMgr;
    QActive_ctor(&me->super, (QStateHandler)&CommStackMgr_initial);
    QTimeEvt_ctor(&me->timeTestTimerEvt, TIME_TEST_SIG);
}

/**
 * \brief CommStackMgr "class"
 */
/*${AOs::CommStackMgr} .....................................................*/
/*${AOs::CommStackMgr::SM} .................................................*/
static QState CommStackMgr_initial(CommStackMgr * const me, QEvt const * const e) {
    /* ${AOs::CommStackMgr::SM::initial} */
    (void)e;        /* suppress the compiler warning about unused parameter */

    QS_OBJ_DICTIONARY(&l_CommStackMgr);
    QS_FUN_DICTIONARY(&QHsm_top);
    QS_FUN_DICTIONARY(&CommStackMgr_initial);
    QS_FUN_DICTIONARY(&CommStackMgr_Active);

    QActive_subscribe((QActive *)me, MSG_SEND_OUT_SIG);
    QActive_subscribe((QActive *)me, MSG_RECEIVED_SIG);
    QActive_subscribe((QActive *)me, TIME_TEST_SIG);
    return Q_TRAN(&CommStackMgr_Active);
}

/**
 * This state is a catch-all Active state.  If any signals need
 * to be handled that do not cause state transitions and are
 * common to the entire AO, they should be handled here.
 *
 * @param  [in|out] me: Pointer to the state machine
 * @param  [in|out]  e:  Pointer to the event being processed.
 * @return status: QState type that specifies where the state
 * machine is going next.
 */
/*${AOs::CommStackMgr::SM::Active} .........................................*/
static QState CommStackMgr_Active(CommStackMgr * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        /* ${AOs::CommStackMgr::SM::Active} */
        case Q_ENTRY_SIG: {
            /* Every 2 seconds, post an event to print time */
            QTimeEvt_postEvery(
                &me->timeTestTimerEvt,
                (QActive *)me,
                SEC_TO_TICKS( 5 )
            );
            status_ = Q_HANDLED();
            break;
        }
        /* ${AOs::CommStackMgr::SM::Active::MSG_SEND_OUT} */
        case MSG_SEND_OUT_SIG: {
            status_ = Q_HANDLED();
            break;
        }
        /* ${AOs::CommStackMgr::SM::Active::MSG_RECEIVED} */
        case MSG_RECEIVED_SIG: {
            status_ = Q_HANDLED();
            break;
        }
        /* ${AOs::CommStackMgr::SM::Active::TIME_TEST} */
        case TIME_TEST_SIG: {
            DBG_printf("Time test\n");

            /*
            t_Time fast_print_start_time = TIME_getTime();
            DBG_printf("Fast DBG_printf() test message from CommStackMgr\n");
            t_Time fast_print_finish_time = TIME_getTime();

            t_Time slow_print_start_time = TIME_getTime();
            dbg_slow_printf("Slow dbg_slow_printf() test message from CommStackMgr\n");
            t_Time slow_print_finish_time = TIME_getTime();

            DBG_printf("DBG_printf() start: %02d:%02d:%02d:%d stop: %02d:%02d:%02d:%d\n",
                 fast_print_start_time.hour_min_sec.RTC_Hours,
                 fast_print_start_time.hour_min_sec.RTC_Minutes,
                 fast_print_start_time.hour_min_sec.RTC_Seconds,
                 (int)fast_print_start_time.sub_sec,
                 fast_print_finish_time.hour_min_sec.RTC_Hours,
                 fast_print_finish_time.hour_min_sec.RTC_Minutes,
                 fast_print_finish_time.hour_min_sec.RTC_Seconds,
                 (int)fast_print_finish_time.sub_sec
            );
            DBG_printf("dbg_slow_printf() start: %02d:%02d:%02d:%d stop: %02d:%02d:%02d:%d\n",
                 slow_print_start_time.hour_min_sec.RTC_Hours,
                 slow_print_start_time.hour_min_sec.RTC_Minutes,
                 slow_print_start_time.hour_min_sec.RTC_Seconds,
                 (int)slow_print_start_time.sub_sec,
                 slow_print_finish_time.hour_min_sec.RTC_Hours,
                 slow_print_finish_time.hour_min_sec.RTC_Minutes,
                 slow_print_finish_time.hour_min_sec.RTC_Seconds,
                 (int)slow_print_finish_time.sub_sec
            );
            */
            status_ = Q_HANDLED();
            break;
        }
        default: {
            status_ = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status_;
}


/******** Copyright (C) 2014 Datacard. All rights reserved *****END OF FILE****/
