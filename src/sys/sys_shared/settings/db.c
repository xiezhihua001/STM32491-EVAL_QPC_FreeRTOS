/**
 * @file		db.c
 * @brief   A simple generic database of settings that worked on any memory type.
 *
 * @date    Feb 24, 2015
 * @author  Harry Rostovtsev
 * @email   rost0031@gmail.com
 * Copyright (C) 2015 Harry Rostovtsev.  All rights reserved. 
 *
 * @addtogroup groupSettings
 * @{
 */

/* Includes ------------------------------------------------------------------*/
#include "qp_port.h"                                        /* for QP support */
#include "project_includes.h"
#include "dbg_out_cntrl.h"
#include <stddef.h>
#include "i2c_defs.h"                                /* for I2C functionality */
#include "i2c_dev.h"                               /* for I2C device mappings */
#include "db.h"
#include "ipAndMac.h"                                  /* for default IP addr */

/* Compile-time called macros ------------------------------------------------*/
Q_DEFINE_THIS_FILE                  /* For QSPY to know the name of this file */
DBG_DEFINE_THIS_MODULE( DBG_MODL_DB );  /* For debug system to ID this module */

/* Private typedefs ----------------------------------------------------------*/
/**
 * DB elements that can be retrieved.
 */
typedef enum {
   DB_EEPROM = 0,           /**< Setting is located in the main EEPROM memory */
   DB_SN_ROM,         /**< Setting is located in the RO SNR section of EEPROM */
   DB_UI_ROM,        /**< Setting is located in the RO UI64 section of EEPROM */

   /* .. insert more I2C devices before GPIO so they are all together .. */

   DB_GPIO,                          /**< Setting is specified via a GPIO pin */
   DB_FLASH                  /**< Setting is located in the main FLASH memory */
} DB_ElemLoc_t;

/**
 * Description type of each DB element (location, etc).
 */
typedef struct {
   DB_Elem_t    elem;  /**< Specifies which settings element is being described */
   DB_ElemLoc_t  loc;  /**< Specifies how the db element is stored */
   size_t       size;  /**< Specifies the size of the db element */
   uint16_t    offset; /**< Specifies the offset from beginning of EEPROM memory
                            where the element is stored */
} SettingsDB_Desc_t;

/**
 * DB element structure mapping that resides in the main memory of the EEPROM
 */
typedef struct {
   uint32_t dbMagicWord; /**< Magic word that specifies whether a DB exists.  If
                              this is not present, we have to initialize the DB
                              in EEPROM memory to some default.*/
   uint16_t dbVersion;   /**< Version of the database */
   uint8_t ipAddr[4];  /**< 4 values of the 111.222.333.444 ip address in hex */
} SettingsDB_t;


/* Private defines -----------------------------------------------------------*/

/**< Magic word that will be stored at the head of the DB.  If this is not
 * there, the DB needs to be initialized to a default and updated after. */
#define DB_MAGIC_WORD_DEF   0xdefec8db

/**< Current version of the DB.  This needs to be bumped once the FW is out in
 * the field and any changes are made to the DB. This is to allow for a smooth
 * upgrade of the DB. */
#define DB_VERSION_DEF      0x0001

/* Private macros ------------------------------------------------------------*/

/**< Macro to get size of the element stored in the EEPROM memory */
#define DB_SIZE_OF_ELEM(s,m)     ((size_t) sizeof(((s *)0)->m))

/**< Macro to get the offset of the element stored in the EEPROM memory */
#define DB_LOC_OF_ELEM(s,m)      offsetof(s, m)

/* Private variables and Local objects ---------------------------------------*/

/**< Array to specify where all the DB elements reside */
static const SettingsDB_Desc_t settingsDB[DB_MAX_ELEM] = {
      { DB_MAGIC_WORD,  DB_EEPROM,  DB_SIZE_OF_ELEM(SettingsDB_t, dbMagicWord), DB_LOC_OF_ELEM(SettingsDB_t, dbMagicWord)},
      { DB_VERSION,     DB_EEPROM,  DB_SIZE_OF_ELEM(SettingsDB_t, dbVersion)  , DB_LOC_OF_ELEM(SettingsDB_t, dbVersion)  },
      { DB_MAC_ADDR,    DB_UI_ROM,  6                                         , 2                                        },
      { DB_IP_ADDR,     DB_EEPROM,  DB_SIZE_OF_ELEM(SettingsDB_t, ipAddr)     , DB_LOC_OF_ELEM(SettingsDB_t, ipAddr)     },
      { DB_SN,          DB_SN_ROM,  16                                        , 0                                        },
};

/**
 * Array to lookup the I2C device for the DB elements which are stored on some
 * I2C device
 */
static const I2C_Dev_t DB_I2C_devices[] = {
      EEPROM,
      SN_ROM,
      EUI_ROM
};

/**< Default settings that belong in the main memory of the EEPROM */
static const SettingsDB_t DB_defaultEeepromSettings = {
      .dbMagicWord = DB_MAGIC_WORD_DEF,
      .dbVersion   = DB_VERSION_DEF,
      .ipAddr = {STATIC_IPADDR0, STATIC_IPADDR1, STATIC_IPADDR2, STATIC_IPADDR3}
};

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
char* DB_elemToStr( DB_Elem_t elem )
{
   switch ( elem ) {
      case DB_MAGIC_WORD:  return("DB_MAGIC_WORD");   break;
      case DB_VERSION:     return("DB_VERSION");      break;
      case DB_MAC_ADDR:    return("DB_MAC_ADDR");     break;
      case DB_IP_ADDR:     return("DB_IP_ADDR");      break;
      case DB_SN:          return("DB_SN");           break;
      default:             return("");                break;
   }
}

/******************************************************************************/
CBErrorCode DB_isValid( AccessType_t accessType )
{
   CBErrorCode status = ERR_NONE;            /* keep track of success/failure */

   uint32_t db_magicWord = 0;
   status = DB_getElemBLK(
         DB_MAGIC_WORD,
         (uint8_t *)&db_magicWord,
         sizeof(db_magicWord),
         ACCESS_BARE_METAL
   );

   if ( ERR_NONE != status ) {
      goto DB_isValid_ERR_HANDLE;          /* Stop and jump to error handling */
   } else {
      if ( db_magicWord != DB_MAGIC_WORD_DEF ) {
         wrn_slow_printf(
               "Magic word in DB (0x%08x) doesn't match expected (0x%08x)\n",
               db_magicWord,
               DB_MAGIC_WORD_DEF
         );
         return( ERR_DB_NOT_INIT);
      }
   }

   uint16_t db_version = 0;
   status = DB_getElemBLK(
         DB_VERSION,
         (uint8_t *)&db_version,
         sizeof(db_version),
         ACCESS_BARE_METAL
   );

   if ( ERR_NONE != status ) {
      goto DB_isValid_ERR_HANDLE;          /* Stop and jump to error handling */
   } else {
      if ( db_version != DB_VERSION_DEF ) {

         wrn_slow_printf(
               "DB version in EEPROM (0x%04x) doesn't match expected (0x%04x)\n",
               db_version,
               DB_VERSION_DEF
         );
         return( ERR_DB_VER_MISMATCH);
      }
   }

DB_isValid_ERR_HANDLE:      /* Handle any error that may have occurred. */
   ERR_COND_OUTPUT(
         status,
         accessType,
         "Error 0x%08x validating the settings DB\n",
         status
   );
   return( status );
}

/******************************************************************************/
CBErrorCode DB_initToDefault( AccessType_t accessType )
{
   CBErrorCode status = ERR_NONE;            /* keep track of success/failure */

   switch( accessType ) {
      case ACCESS_BARE_METAL:
         status = DB_setElemBLK(
               DB_MAGIC_WORD,
               (uint8_t *)&DB_defaultEeepromSettings.dbMagicWord,
               sizeof(DB_defaultEeepromSettings.dbMagicWord),
               accessType
         );
         if ( ERR_NONE != status ) {
            goto DB_initToDefault_ERR_HANDLE;
         }

         status = DB_setElemBLK(
               DB_VERSION,
               (uint8_t *)&DB_defaultEeepromSettings.dbVersion,
               sizeof(DB_defaultEeepromSettings.dbVersion),
               accessType
         );
         if ( ERR_NONE != status ) {
            goto DB_initToDefault_ERR_HANDLE;
         }

         status = DB_setElemBLK(
               DB_IP_ADDR,
               (uint8_t *)&DB_defaultEeepromSettings.ipAddr,
               sizeof(DB_defaultEeepromSettings.ipAddr),
               accessType
         );
         if ( ERR_NONE != status ) {
            goto DB_initToDefault_ERR_HANDLE;
         }

         break;                              /* end of case ACCESS_BARE_METAL */

      case ACCESS_QPC:
         status = ERR_UNIMPLEMENTED;
         goto DB_initToDefault_ERR_HANDLE; /* Stop and jump to error handling */
         break;                                     /* end of case ACCESS_QPC */

      case  ACCESS_FREERTOS:
         status = ERR_UNIMPLEMENTED;
         goto DB_initToDefault_ERR_HANDLE; /* Stop and jump to error handling */
         break;                                /* end of case ACCESS_FREERTOS */

      default:
         status = ERR_UNIMPLEMENTED;
         goto DB_initToDefault_ERR_HANDLE; /* Stop and jump to error handling */
         break;                                        /* end of default case */

   }                                               /* End of switch statement */

DB_initToDefault_ERR_HANDLE:      /* Handle any error that may have occurred. */
   ERR_COND_OUTPUT(
         status,
         accessType,
         "Error 0x%08x initializing the DB to default\n",
         status
   );
   return( status );
}

/******************************************************************************/
CBErrorCode DB_getElemBLK(
      DB_Elem_t elem,
      uint8_t* pBuffer,
      size_t bufSize,
      AccessType_t accessType
)
{
   CBErrorCode status = ERR_NONE;            /* keep track of success/failure */

   /* 1. Sanity checks of buffer sizes and memory allocations. */
   if ( bufSize < settingsDB[elem].size ) {
      status = ERR_MEM_BUFFER_LEN;
      goto DB_getElemBLK_ERR_HANDLE;       /* Stop and jump to error handling */
   }

   if ( NULL == pBuffer ) {
      status = ERR_MEM_NULL_VALUE;
      goto DB_getElemBLK_ERR_HANDLE;       /* Stop and jump to error handling */
   }

   /* 2. Find where the element lives */
   DB_ElemLoc_t loc = settingsDB[elem].loc;

   /* 3. Call the location dependent functions to retrieve the data from DB */
   switch( loc ) {
      case DB_EEPROM:                           /* Intentionally fall through */
      case DB_SN_ROM:                           /* Intentionally fall through */
      case DB_UI_ROM:
         status = I2C_readDevMemBLK(
               DB_I2C_devices[loc],          // I2C_Dev_t iDev,
               settingsDB[elem].offset,      // uint16_t offset,
               settingsDB[elem].size,        // uint16_t bytesToRead,
               accessType,                   // AccessType_t accType,
               pBuffer,                      // uint8_t* pBuffer,
               bufSize                       // uint8_t  bufSize
         );
         break;
      case DB_GPIO:
         status = ERR_UNIMPLEMENTED;
         goto DB_getElemBLK_ERR_HANDLE;    /* Stop and jump to error handling */
         break;
      case DB_FLASH:
         status = ERR_UNIMPLEMENTED;
         goto DB_getElemBLK_ERR_HANDLE;    /* Stop and jump to error handling */
         break;
         /* Add more locations here. Anything that fails will go to the default
          * case and get logged as an error. */
      default:
         status = ERR_DB_ELEM_NOT_FOUND;
         goto DB_getElemBLK_ERR_HANDLE;    /* Stop and jump to error handling */
         break;
   }

DB_getElemBLK_ERR_HANDLE:         /* Handle any error that may have occurred. */
   ERR_COND_OUTPUT(
         status,
         accessType,
         "Error 0x%08x getting element %s (%d) from DB\n",
         DB_elemToStr( elem ),
         elem,
         status
   );
   return( status );
}

/******************************************************************************/
CBErrorCode DB_setElemBLK(
      DB_Elem_t elem,
      uint8_t* pBuffer,
      size_t bufSize,
      AccessType_t accessType
)
{
   CBErrorCode status = ERR_NONE; /* keep track of success/failure of operations. */

   /* 1. Sanity checks of buffer sizes and memory allocations. */
   if ( bufSize > settingsDB[elem].size ) {
      status = ERR_MEM_BUFFER_LEN;
      goto DB_getElemBLK_ERR_HANDLE;       /* Stop and jump to error handling */
   }

   if ( NULL == pBuffer ) {
      status = ERR_MEM_NULL_VALUE;
      goto DB_getElemBLK_ERR_HANDLE;       /* Stop and jump to error handling */
   }

   /* 2. Find where the element lives */
   DB_ElemLoc_t loc = settingsDB[elem].loc;

   /* 3. Call the location dependent functions to write the data to DB */
   switch( loc ) {
      case DB_EEPROM:
         status = I2C_writeDevMemBLK(
               DB_I2C_devices[loc],                // I2C_Dev_t iDev,
               settingsDB[elem].offset,            // uint16_t offset,
               settingsDB[elem].size,              // uint16_t bytesToWrite,
               accessType,                         // AccessType_t accType,
               pBuffer,                            // uint8_t* pBuffer,
               bufSize                             // uint8_t  bufSize
         );
         break;
      case DB_SN_ROM:                           /* Fall through intentionally */
      case DB_UI_ROM:                           /* Fall through intentionally */
      case DB_GPIO:                             /* Fall through intentionally */
      case DB_FLASH:                            /* Fall through intentionally */
         status = ERR_DB_ELEM_IS_READ_ONLY;
         goto DB_getElemBLK_ERR_HANDLE;
         break;
         /* Add more locations here. Anything that fails will go to the default
          * case and get logged as an error. */
      default:
         status = ERR_DB_ELEM_NOT_FOUND;
         goto DB_getElemBLK_ERR_HANDLE;
         break;
   }

DB_getElemBLK_ERR_HANDLE:         /* Handle any error that may have occurred. */
   ERR_COND_OUTPUT(
         status,
         accessType,
         "Error 0x%08x setting element %s (%d) to DB\n",
         DB_elemToStr( elem ),
         elem,
         status
   );
   return( status );
}

/**
 * @}
 * end addtogroup groupSettings
 */

/******** Copyright (C) 2015 Harry Rostovtsev. All rights reserved *****END OF FILE****/
