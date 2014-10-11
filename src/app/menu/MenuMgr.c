/*****************************************************************************
* Model: MenuMgr.qm
* File:  ./MenuMgr_gen.c
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
/*${.::MenuMgr_gen.c} ......................................................*/
/**
 * @file    MenuMgr.c
 * Declarations for functions for the MenuMgr AO.  This state machine handles
 * all menu interactions.
 *
 * Note: If editing this file, please make sure to update the MenuMgr.qm
 * model.  The generated code from that model should be very similar to the
 * code in this file.
 *
 * @date    09/26/2014
 * @author  Harry Rostovtsev
 * @email   harry_rostovtsev@datacard.com
 * Copyright (C) 2014 Datacard. All rights reserved.
 *
 * @addtogroup groupMenu
 * @{
 */

/* Includes ------------------------------------------------------------------*/
#include "MenuMgr.h"
#include "project_includes.h"           /* Includes common to entire project. */
#include "menu_top.h"

/* Compile-time called macros ------------------------------------------------*/
Q_DEFINE_THIS_FILE;                 /* For QSPY to know the name of this file */
DBG_DEFINE_THIS_MODULE( DBG_MODL_MENU );/* For debug system to ID this module */

/* Private typedefs ----------------------------------------------------------*/

/**
 * \brief MenuMgr "class"
 */
/*${AOs::MenuMgr} ..........................................................*/
typedef struct {
/* protected: */
    QActive super;

/* public: */

    /**
     * @brief	Pointer to the top level of the menu.  Gets initialized on startup.
     */
    treeNode_t * menu;
} MenuMgr;

/* protected: */
static QState MenuMgr_initial(MenuMgr * const me, QEvt const * const e);

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
static QState MenuMgr_Active(MenuMgr * const me, QEvt const * const e);


/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private variables and Local objects ---------------------------------------*/

static MenuMgr l_MenuMgr; /* the single instance of the Interstage active object */

/* Global-scope objects ----------------------------------------------------*/
QActive * const AO_MenuMgr = (QActive *)&l_MenuMgr;  /* "opaque" AO pointer */

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
 * @brief C "constructor" for MenuMgr "class".
 * Initializes all the timers and queues used by the AO and sets of the
 * first state.
 * @param  None
 * @param  None
 * @retval None
 */
/*${AOs::MenuMgr_ctor} .....................................................*/
void MenuMgr_ctor(void) {
    MenuMgr *me = &l_MenuMgr;
    QActive_ctor(&me->super, (QStateHandler)&MenuMgr_initial);
}

/**
 * \brief MenuMgr "class"
 */
/*${AOs::MenuMgr} ..........................................................*/
/*${AOs::MenuMgr::SM} ......................................................*/
static QState MenuMgr_initial(MenuMgr * const me, QEvt const * const e) {
    /* ${AOs::MenuMgr::SM::initial} */
    (void)e;        /* suppress the compiler warning about unused parameter */

    QS_OBJ_DICTIONARY(&l_MenuMgr);
    QS_FUN_DICTIONARY(&QHsm_top);
    QS_FUN_DICTIONARY(&MenuMgr_initial);
    QS_FUN_DICTIONARY(&MenuMgr_Active);

    QActive_subscribe((QActive *)me, MENU_GENERAL_REQ_SIG);

    return Q_TRAN(&MenuMgr_Active);
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
/*${AOs::MenuMgr::SM::Active} ..............................................*/
static QState MenuMgr_Active(MenuMgr * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        /* ${AOs::MenuMgr::SM::Active} */
        case Q_ENTRY_SIG: {
            /* Initialize the menu */
            me->menu = MENU_init();
            MENU_printf("*************************************************************\n");
            MENU_printf("***** Press '?' at any time to request a menu and help. *****\n");
            MENU_printf("*************************************************************\n");
            status_ = Q_HANDLED();
            break;
        }
        /* ${AOs::MenuMgr::SM::Active::MENU_GENERAL_REQ} */
        case MENU_GENERAL_REQ_SIG: {
            me->menu = MENU_parse(me->menu, ((MenuEvt const *)e)->buffer, ((MenuEvt const *)e)->bufferLen, ((MenuEvt const *)e)->msgSrc);
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
 * @} end addtogroup groupMenu
 */
/******** Copyright (C) 2014 Datacard. All rights reserved *****END OF FILE****/