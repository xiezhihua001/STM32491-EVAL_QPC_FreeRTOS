/*****************************************************************************
* Model: I2CBusMgr.qm
* File:  ./I2CBusMgr_gen.c
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
/*${.::I2CBusMgr_gen.c} ....................................................*/
/**
 * @file    I2CBusMgr.c
 * @brief   Declarations for functions for the I2CBusMgr AO.
 * This state machine handles all I/O on the I2C bus.  It can be instantiated
 * several times with a different bus for a parameter.  This AO doesn't handle
 * any actual I2C devices, just generic I2C bus operations such as sending the
 * start/end bit, waiting for events, initiating DMA reads/writes, etc.  I2C
 * devices should be handled in the I2CXDevMgr AOs.  This allows this AO to be
 * generic and reusable.
 *
 * @note 1: If editing this file, please make sure to update the I2CBusMgr.qm
 * model.  The generated code from that model should be very similar to the
 * code in this file.
 *
 * @date    10/24/2014
 * @author  Harry Rostovtsev
 * @email   harry_rostovtsev@datacard.com
 * Copyright (C) 2014 Datacard. All rights reserved.
 *
 * @addtogroup groupI2C
 * @{
 */

/* Includes ------------------------------------------------------------------*/
#include "I2CBusMgr.h"
#include "project_includes.h"           /* Includes common to entire project. */
#include "bsp.h"          /* For seconds to bsp tick conversion (SEC_TO_TICK) */

/* Compile-time called macros ------------------------------------------------*/
Q_DEFINE_THIS_FILE                  /* For QSPY to know the name of this file */
DBG_DEFINE_THIS_MODULE( DBG_MODL_I2C ); /* For debug system to ID this module */

/* Private typedefs ----------------------------------------------------------*/

/**
 * @brief I2CMgr Active Object (AO) "class" that manages the I2C bus.
 * This AO manages the I2C bus and all events associated with it. It
 * has exclusive access to the I2C bus and the ISR handlers will let
 * the AO know that the transfer has completed.  See I2CMgr.qm for
 * diagram and model.
 */
/*${AOs::I2CBusMgr} ........................................................*/
typedef struct {
/* protected: */
    QActive super;

    /**< QPC timer Used to timeout I2C transfers if errors occur. */
    QTimeEvt i2cTimerEvt;

    /**< Specifies which I2C device is currently being handled by this AO.  This should
         be set when a new I2C_READ_START or I2C_WRITE_START events come in.  Those
         events should contain the device for which they are meant for. */
    I2C_Device_t iDevice;

    /**< Which I2C bus this AO is responsible for.  This variable is set on
         startup and is used to index into the structure that holds all the
         I2C bus settings. */
    I2C_Bus_t iBus;

    /**< Counter used to manually timeout some I2C operations.  Though we supposed to
         not do blocking operations like this, it's unavoidable in this case since
         the I2C ISRs won't post events until they are cleared, which happens after
         here in the AO so nothing moves forward.  With all the delays introduced by
         just event handling, there should be no blocking in reality but just in case,
         there will still be timeout events launched from these loops if this counter
         gets to 0. */
    uint32_t nI2CLoopTimeout;

    /**< QPC timer Used to time I2C DMA operations. */
    QTimeEvt i2cDMATimerEvt;

    /**< Specifies what the current I2C bus operation is happening.  Gets set upon
         reception of I2C_READ_START_SIG and I2C_WRITE_START_SIG */
    I2C_Operation_t i2cCurrOperation;

    /**< QPC timer Used to timeout I2C bus recovery wait states. */
    QTimeEvt i2cRecoveryTimerEvt;
} I2CBusMgr;

/* protected: */
static QState I2CBusMgr_initial(I2CBusMgr * const me, QEvt const * const e);

/**
 * @brief This state is a catch-all Active state.
 * If any signals need to be handled that do not cause state transitions and
 * are common to the entire AO, they should be handled here.
 *
 * @param  [in,out] me: Pointer to the state machine
 * @param  [in,out] e:  Pointer to the event being processed.
 * @return status_: QState type that specifies where the state
 * machine is going next.
 */
static QState I2CBusMgr_Active(I2CBusMgr * const me, QEvt const * const e);

/**
 * @brief This state indicates that the I2C bus is currently idle and the
 * incoming msg can be handled.
 * This state is the default rest state of the state machine and can handle
 * various I2C requests.  Upon entry, it also checks the deferred queue to see
 * if any request events are waiting which were posted while I2C bus was busy.
 * if there are any waiting, it will read them out, which automatically posts
 * them and the state machine will go and handle them.
 *
 * @param  [in,out] me: Pointer to the state machine
 * @param  [in,out] e:  Pointer to the event being processed.
 * @return status: QState type that specifies where the state
 * machine is going next.
 */
static QState I2CBusMgr_Idle(I2CBusMgr * const me, QEvt const * const e);


/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private variables and Local objects ---------------------------------------*/
static I2CBusMgr l_I2CBusMgr[MAX_I2C_BUS]; /* the single instance of the active object */

/* Global-scope objects ----------))------------------------------------------*/
QActive * const AO_I2CBusMgr[MAX_I2C_BUS] = {
    (QActive *)&l_I2CBusMgr[I2CBus1], /* "opaque" AO pointer to the I2CBusMgr for I2C1 */
};
extern I2C_BusSettings_t s_I2C_Bus[MAX_I2C_BUS];
extern I2C_BusDevice_t   s_I2C_Dev[MAX_I2C_DEV];

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
 * @brief C "constructor" for I2CBusMgr "class".
 * Initializes all the timers and queues used by the AO, sets up a deferral
 * queue, and sets of the first state.
 * @param  [in] i2cBus: I2C_TypeDef * type that specifies which STM32 I2C bus this
 * AO is responsible for.
 * @retval None
 */
/*${AOs::I2CBusMgr_ctor} ...................................................*/
void I2CBusMgr_ctor(I2C_Bus_t iBus) {
    I2CBusMgr *me = &l_I2CBusMgr[iBus]; // Get the local pointer to the external instance
    me->iBus = iBus;  // Store which I2C bus this instance of the AO is handling

    QActive_ctor( &me->super, (QStateHandler)&I2CBusMgr_initial );
    QTimeEvt_ctor( &me->i2cTimerEvt, I2C_TIMEOUT_SIG );
    QTimeEvt_ctor( &me->i2cDMATimerEvt, I2C_DMA_TIMEOUT_SIG );
    QTimeEvt_ctor( &me->i2cRecoveryTimerEvt, I2C_RECOVERY_TIMEOUT_SIG );

    dbg_slow_printf("Constructor\n");
}

/**
 * @brief I2CMgr Active Object (AO) "class" that manages the I2C bus.
 * This AO manages the I2C bus and all events associated with it. It
 * has exclusive access to the I2C bus and the ISR handlers will let
 * the AO know that the transfer has completed.  See I2CMgr.qm for
 * diagram and model.
 */
/*${AOs::I2CBusMgr} ........................................................*/
/*${AOs::I2CBusMgr::SM} ....................................................*/
static QState I2CBusMgr_initial(I2CBusMgr * const me, QEvt const * const e) {
    /* ${AOs::I2CBusMgr::SM::initial} */
    (void)e;        /* suppress the compiler warning about unused parameter */

    QS_OBJ_DICTIONARY(&l_I2CBusMgr);
    QS_FUN_DICTIONARY(&QHsm_top);
    QS_FUN_DICTIONARY(&I2CBusMgr_initial);
    QS_FUN_DICTIONARY(&I2CBusMgr_Active);
    QS_FUN_DICTIONARY(&I2CBusMgr_Idle);
    return Q_TRAN(&I2CBusMgr_Idle);
}

/**
 * @brief This state is a catch-all Active state.
 * If any signals need to be handled that do not cause state transitions and
 * are common to the entire AO, they should be handled here.
 *
 * @param  [in,out] me: Pointer to the state machine
 * @param  [in,out] e:  Pointer to the event being processed.
 * @return status_: QState type that specifies where the state
 * machine is going next.
 */
/*${AOs::I2CBusMgr::SM::Active} ............................................*/
static QState I2CBusMgr_Active(I2CBusMgr * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        /* ${AOs::I2CBusMgr::SM::Active} */
        case Q_ENTRY_SIG: {
            /* Post all the timers and disarm them right away so it can be
             * rearmed at any point without worrying asserts. */
            QTimeEvt_postIn(
                &me->i2cTimerEvt,
                (QActive *)me,
                SEC_TO_TICKS( LL_MAX_TIMEOUT_SERIAL_DMA_BUSY_SEC )
            );
            QTimeEvt_disarm(&me->i2cTimerEvt);

            QTimeEvt_postIn(
                &me->i2cDMATimerEvt,
                (QActive *)me,
                SEC_TO_TICKS( LL_MAX_TIMEOUT_I2C_READ_OP_SEC )
            );
            QTimeEvt_disarm(&me->i2cDMATimerEvt);

            QTimeEvt_postIn(
                &me->i2cRecoveryTimerEvt,
                (QActive *)me,
                SEC_TO_TICKS( LL_MAX_TIMEOUT_I2C_BUS_RECOVERY_SEC )
            );
            QTimeEvt_disarm(&me->i2cRecoveryTimerEvt);

            /* Initialize the I2C devices and associated busses */
            I2C_BusInit( me->iBus );
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

/**
 * @brief This state indicates that the I2C bus is currently idle and the
 * incoming msg can be handled.
 * This state is the default rest state of the state machine and can handle
 * various I2C requests.  Upon entry, it also checks the deferred queue to see
 * if any request events are waiting which were posted while I2C bus was busy.
 * if there are any waiting, it will read them out, which automatically posts
 * them and the state machine will go and handle them.
 *
 * @param  [in,out] me: Pointer to the state machine
 * @param  [in,out] e:  Pointer to the event being processed.
 * @return status: QState type that specifies where the state
 * machine is going next.
 */
/*${AOs::I2CBusMgr::SM::Active::Idle} ......................................*/
static QState I2CBusMgr_Idle(I2CBusMgr * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        /* ${AOs::I2CBusMgr::SM::Active::Idle} */
        case Q_ENTRY_SIG: {
            DBG_printf("I2CBusMgr for I2CBus%d back in Idle\n", me->iBus+1);
            status_ = Q_HANDLED();
            break;
        }
        default: {
            status_ = Q_SUPER(&I2CBusMgr_Active);
            break;
        }
    }
    return status_;
}


/**
 * @} end addtogroup groupI2C
 */
/******** Copyright (C) 2014 Datacard. All rights reserved *****END OF FILE****/
