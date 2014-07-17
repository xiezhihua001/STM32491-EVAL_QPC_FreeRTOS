/**
 * @file 	i2c.c
 * @brief   Basic driver for I2C bus.
 *
 * @date   	06/30/2014
 * @author 	Harry Rostovtsev
 * @email  	harry_rostovtsev@datacard.com
 * Copyright (C) 2014 Datacard. All rights reserved.
 *
 * @addtogroup groupI2C
 * @{
 *
 */

/* Includes ------------------------------------------------------------------*/
#include "i2c.h"
#include "qp_port.h"                                        /* for QP support */
#include "stm32f2xx_it.h"
#include "project_includes.h"
#include "Shared.h"
#include "I2CMgr.h"                                    /* For I2C event types */
#include "stm32f2xx_dma.h"                           /* For STM32 DMA support */

/* Compile-time called macros ------------------------------------------------*/
Q_DEFINE_THIS_FILE                  /* For QSPY to know the name of this file */

/* Private typedefs ----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
#define EE_START_ADDR      0x00 /**< EEProms internal memory starting address */
#define EE_PAGESIZE        32    /**< EEProm's internal page size (in bytes). */

/* Private macros ------------------------------------------------------------*/
/* Private variables and Local objects ---------------------------------------*/
/**
 * @brief Buffer for I2C device
 */
static uint8_t          i2c1RxBuffer[MAX_MSG_LEN];

/**
 * @brief An internal structure that holds settings for I2C devices on all I2C
 * busses.
 */
static I2C_BusDevice_t s_I2C_Dev[MAX_I2C_DEV] =
{
      {
            EEPROM,                    /**< i2c_dev */
            I2C1,                      /**< i2c_bus */

            0xA0,                      /**< i2c_address */
      }
};

/**
 * @brief An internal structure that holds almost all the settings for the I2C
 * Busses
 */
I2C_BusSettings_t s_I2C_Bus[MAX_I2C_BUS] =
{
      {
            I2CBus1,                   /**< i2c_sys_bus */

            /* I2C bus settings */
            I2C1,                      /**< i2c_bus */
            100000,                    /**< i2c_bus_speed (Hz)*/
            RCC_APB1Periph_I2C1,       /**< i2c_clk */

            /* I2C interrupt settings */
            I2C1_EV_IRQn,              /**< i2c_ev_irq_num */
            I2C1_EV_PRIO,              /**< i2c_ev_irq_prio */
            I2C1_ER_IRQn,              /**< i2c_er_irq_num */
            I2C1_ER_PRIO,              /**< i2c_er_irq_prio */

            /* I2C GPIO settings for SCL */
            GPIOB,                     /**< scl_port */
            GPIO_Pin_8,                /**< scl_pin */
            GPIO_PinSource8,           /**< scl_af_pin_source */
            GPIO_AF_I2C1,              /**< scl_af */

            /* I2C GPIO settings for SDA */
            GPIOB,                     /**< sda_port */
            GPIO_Pin_9,                /**< sda_pin */
            GPIO_PinSource9,           /**< sda_af_pin_source */
            GPIO_AF_I2C1,              /**< sda_af */

            /* I2C DMA settings */
            DMA1,                      /**< i2c_dma */
            DMA_Channel_1,             /**< i2c_dma_channel */
            ((uint32_t)0x40005410),    /**< i2c_dma_dr_addr */

            DMA1_Stream6,              /**< i2c_dma_tx_stream */
            DMA1_Stream6_IRQn,         /**< i2c_dma_tx_irq_num */
            DMA1_Stream6_PRIO,         /**< i2c_dma_tx_irq_prio */

            DMA1_Stream0,              /**< i2c_dma_rx_stream */
            DMA1_Stream0_IRQn,         /**< i2c_dma_rx_irq_num */
            DMA1_Stream0_PRIO,         /**< i2c_dma_rx_irq_prio */


            /* Buffer management */
            &i2c1RxBuffer[0],          /**< *pRXBuffer */
            0,                         /**< nRXindex */

            /* Device management */
            EEPROM,                    /**< i2c_cur_dev */
            0xA0,                      /**< i2c_cur_dev_addr */
            I2C_Direction_Transmitter, /**< bTransDirection */
            0,                         /**< nBytesExpected */
            0,                         /**< nBytesCurrent */
      }
};

DMA_InitTypeDef    DMA_InitStructure;        /**< For I2C DMA initialization. */

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
void I2C_BusInit( I2C_Bus_t iBus )
{
   /* Periph clock enable.  The GPIO clocks should already be enabled by BSP_Init() */
   RCC_APB1PeriphClockCmd(s_I2C_Bus[iBus].i2c_clk, ENABLE);

   /* Connect PXx to I2C SCL alt function. */
   GPIO_PinAFConfig(
         s_I2C_Bus[iBus].scl_port,
         s_I2C_Bus[iBus].scl_af_pin_source,
         s_I2C_Bus[iBus].scl_af
   );

   /* Connect PXx to I2C SDA alt function. */
   GPIO_PinAFConfig(
         s_I2C_Bus[iBus].sda_port,
         s_I2C_Bus[iBus].sda_af_pin_source,
         s_I2C_Bus[iBus].sda_af
   );

   /* Configure I2C pins: SCL */
   GPIO_InitTypeDef  GPIO_InitStructure;
   GPIO_InitStructure.GPIO_Pin      = s_I2C_Bus[iBus].scl_pin;
   GPIO_InitStructure.GPIO_Mode     = GPIO_Mode_AF;
   GPIO_InitStructure.GPIO_Speed    = GPIO_Speed_50MHz;
   GPIO_InitStructure.GPIO_OType    = GPIO_OType_OD;
   GPIO_InitStructure.GPIO_PuPd     = GPIO_PuPd_NOPULL;
   GPIO_Init(s_I2C_Bus[iBus].scl_port, &GPIO_InitStructure);

   /* Configure I2C pins: SDA */
   GPIO_InitStructure.GPIO_Pin = s_I2C_Bus[iBus].sda_pin;
   GPIO_Init(s_I2C_Bus[iBus].sda_port, &GPIO_InitStructure);

   /* Reset I2C IP */
   I2C_DeInit( s_I2C_Bus[iBus].i2c_bus );

   /* I2C configuration */
   I2C_InitTypeDef  I2C_InitStructure;
   I2C_InitStructure.I2C_Mode                = I2C_Mode_SMBusHost;
   I2C_InitStructure.I2C_DutyCycle           = I2C_DutyCycle_2;
   I2C_InitStructure.I2C_OwnAddress1         = s_I2C_Bus[iBus].i2c_cur_dev_addr;
   I2C_InitStructure.I2C_Ack                 = I2C_Ack_Enable;
   I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
   I2C_InitStructure.I2C_ClockSpeed          = s_I2C_Bus[iBus].i2c_bus_speed;

   /* Set up Interrupt controller to handle I2C Event interrupts */
//   NVIC_Config(
//         s_I2C_Bus[iBus].i2c_ev_irq_num,
//         s_I2C_Bus[iBus].i2c_ev_irq_prio
//   );

   /* Set up Interrupt controller to handle I2C Error interrupts */
//   NVIC_Config(
//         s_I2C_Bus[iBus].i2c_er_irq_num,
//         s_I2C_Bus[iBus].i2c_er_irq_prio
//   );

   /* Enable All I2C Interrupts */
//   I2C_ITConfig(
//         s_I2C_Bus[iBus].i2c_bus,
//         I2C_IT_EVT, // | I2C_IT_BUF, // | I2C_IT_ERR,
//         ENABLE
//   );

   /* Initialize the IRQ and priorities for I2C DMA */
   NVIC_Config(
         s_I2C_Bus[iBus].i2c_dma_rx_irq_num,
         s_I2C_Bus[iBus].i2c_dma_rx_irq_prio
   );
//
//   NVIC_Config(
//         s_I2C_Bus[iBus].i2c_dma_tx_irq_num,
//         s_I2C_Bus[iBus].i2c_dma_tx_irq_prio
//   );

   /* Apply I2C configuration */
   I2C_Init( s_I2C_Bus[iBus].i2c_bus, &I2C_InitStructure );
   I2C_AcknowledgeConfig(s_I2C_Bus[iBus].i2c_bus, ENABLE );

   /* I2C Peripheral Enable */
   I2C_Cmd( s_I2C_Bus[iBus].i2c_bus, ENABLE );

}

/******************************************************************************/
void I2C_SetDirection( I2C_Bus_t iBus,  uint8_t I2C_Direction )
{
   /* Check inputs */
   assert_param( IS_I2C_DIRECTION( I2C_Direction ) );
   assert_param( IS_I2C_BUS( iBus ) );

   s_I2C_Bus[iBus].bTransDirection = I2C_Direction;
}

/******************************************************************************/
void I2C_DMARead( I2C_Bus_t iBus, uint16_t wReadAddr, uint16_t wReadLen )
{
   /* Check inputs */
   assert_param( IS_I2C_BUS( iBus ) );

   /* Set the structure buffer management for how many bytes to expect and how
    * many are currently there (none). */
   s_I2C_Bus[iBus].nBytesExpected = wReadLen;
   s_I2C_Bus[iBus].nBytesCurrent  = 0;
   s_I2C_Bus[iBus].nRxindex       = 0;

   /* Clear out the DMA settings */
   DMA_DeInit( s_I2C_Bus[iBus].i2c_dma_rx_stream );

   /* Set up a new DMA structure for reading */
   DMA_InitStructure.DMA_Channel             = s_I2C_Bus[iBus].i2c_dma_channel;
   DMA_InitStructure.DMA_PeripheralBaseAddr  = s_I2C_Bus[iBus].i2c_dma_dr_addr;
   DMA_InitStructure.DMA_DIR                 = DMA_DIR_PeripheralToMemory;
   DMA_InitStructure.DMA_PeripheralInc       = DMA_PeripheralInc_Disable;
   DMA_InitStructure.DMA_MemoryInc           = DMA_MemoryInc_Enable;
   DMA_InitStructure.DMA_PeripheralDataSize  = DMA_PeripheralDataSize_Byte;
   DMA_InitStructure.DMA_MemoryDataSize      = DMA_MemoryDataSize_Byte;
   DMA_InitStructure.DMA_Mode                = DMA_Mode_Normal;
   DMA_InitStructure.DMA_Priority            = DMA_Priority_VeryHigh;
   DMA_InitStructure.DMA_FIFOMode            = DMA_FIFOMode_Enable;
   DMA_InitStructure.DMA_FIFOThreshold       = DMA_FIFOThreshold_Full;
   DMA_InitStructure.DMA_MemoryBurst         = DMA_MemoryBurst_Single;
   DMA_InitStructure.DMA_PeripheralBurst     = DMA_PeripheralBurst_Single;
   DMA_InitStructure.DMA_Memory0BaseAddr     = s_I2C_Bus[iBus].pRxBuffer;
   DMA_InitStructure.DMA_BufferSize          = s_I2C_Bus[iBus].nBytesExpected;

   /* Initialize the DMA with the filled in structure */
   DMA_Init( s_I2C_Bus[iBus].i2c_dma_rx_stream, &DMA_InitStructure );

   /* Enable the TransferComplete interrupt in the DMA */
   DMA_ITConfig( s_I2C_Bus[iBus].i2c_dma_rx_stream, DMA_IT_TC, ENABLE );

   /* Clear any pending flags on RX Stream */
   DMA_ClearFlag(
         s_I2C_Bus[iBus].i2c_dma_rx_stream,
         DMA_FLAG_TCIF0 |
         DMA_FLAG_FEIF0 |
         DMA_FLAG_DMEIF0 |
         DMA_FLAG_TEIF0 |
         DMA_FLAG_HTIF0
   );

   /* I2Cx DMA Enable */
   I2C_DMACmd( s_I2C_Bus[iBus].i2c_bus, ENABLE );

   /* Finally, activate DMA */
   DMA_Cmd( s_I2C_Bus[iBus].i2c_dma_rx_stream, ENABLE );
   DBG_printf("Sent DMA read\n");
}

/**
 * @}
 * end addtogroup groupI2C
 */

/******************************************************************************/
void DMA1_Stream0_IRQHandler( void )
{
   QK_ISR_ENTRY();                         /* inform QK about entering an ISR */

   /* Test on DMA Stream Transfer Complete interrupt */
   if ( RESET != DMA_GetITStatus(DMA1_Stream0, DMA_IT_TCIF0) ) {
      I2C_AcknowledgeConfig( I2C1, DISABLE);
      I2C_GenerateSTOP(I2C1, ENABLE);

      /* Wait for the byte to be received */
      uint16_t EETimeout = 10000;
      while(I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE) == RESET) {
         if((EETimeout--) == 0) {
            isr_dbg_slow_printf("Timeout 1\n");
            return;
         }
      }

      I2C_AcknowledgeConfig(I2C1, ENABLE);

      /* Disable DMA so it doesn't keep outputting the buffer. */
      DMA_Cmd( DMA1_Stream0, DISABLE );

      /* Publish event stating that the count has been reached */
      I2CDataEvt *i2cDataEvt = Q_NEW( I2CDataEvt, I2C_READ_DONE_SIG );
      i2cDataEvt->i2cDevice = EEPROM;
      i2cDataEvt->wBufferLen = s_I2C_Bus[I2CBus1].nBytesExpected;
      MEMCPY(
            i2cDataEvt->buffer,
            s_I2C_Bus[I2CBus1].pRxBuffer,
            i2cDataEvt->wBufferLen
      );
      QF_PUBLISH( (QEvent *)i2cDataEvt, AO_SerialMgr );

      isr_dbg_slow_printf("DMA done. EETimeout was %d\n", EETimeout);

      /* Try to read a junk byte to see if the stop big makes it through */
//      /* Disable Acknowledgment */
//      I2C_AcknowledgeConfig( I2C1, DISABLE);
//
//      I2C_GenerateSTOP(I2C1, ENABLE);

//      /* Wait for the byte to be received */
//      uint16_t EETimeout = 10000;
//      while(I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE) == RESET) {
//         if((EETimeout--) == 0) {
//            isr_dbg_slow_printf("Timeout 1\n");
//            return;
//         }
//      }

      /* Read the byte received from the EEPROM */
//      I2C_ReceiveData(I2C1);
//
//      /* Wait to make sure that STOP control bit has been cleared */
//      EETimeout = 10000;
//      while(I2C1->CR1 & I2C_CR1_STOP) {
//         if((EETimeout--) == 0) {
//            isr_dbg_slow_printf("Timeout 2\n");
//            return;
//         }
//      }

      /* Re-Enable Acknowledgment to be ready for another reception */
//      I2C_AcknowledgeConfig(I2C1, ENABLE);

      isr_dbg_slow_printf("Stop bit done\n");
      /* Clear DMA Stream Transfer Complete interrupt pending bit */
      DMA_ClearITPendingBit( DMA1_Stream0, DMA_IT_TCIF0 );
   }

   QK_ISR_EXIT();                           /* inform QK about exiting an ISR */
}

/******************************************************************************/
void I2C1_EV_IRQHandler( void )
{
   QK_ISR_ENTRY();                         /* inform QK about entering an ISR */

   /* Get Last I2C Event */
   __IO uint32_t I2CEvent = I2C_GetLastEvent( I2C1 );

   /* Check which I2C device on this bus is being used */
//   if ( EEPROM == s_I2C_Bus[I2CBus1].i2c_cur_dev ) {
      switch (I2CEvent) {
         //      case 0x00000000:
         //      case 0x00000001:
         //      case 0x00004001:
         //      case 0x00034044:
         //         /* This is the I2C "Event" that is always triggering so we explicitly
         //          * ignore it here */
         //         break;

         case I2C_EVENT_MASTER_MODE_SELECT:

            /* Send slave Address for write */
            I2C_Send7bitAddress(
                  I2C1,                                  // This is always the bus used in this ISR
                  s_I2C_Bus[I2CBus1].i2c_cur_dev_addr,   // Look up the current device address for this bus
                  s_I2C_Bus[I2CBus1].bTransDirection     // Direction of data on this bus
            );

            isr_dbg_slow_printf("I2C_EVENT_MASTER_MODE_SELECT\n");
            break;


         /* Check on EV6 */
         case I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED:

            /* Send the EEPROM's internal address to read from:
             * MSB of the address first */
//            I2C_SendData(I2C1, (uint8_t)((0x00 & 0xFF00) >> 8));
            isr_dbg_slow_printf("I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED\n");


//            /*!< Send the EEPROM's internal address to read from: MSB of the address first */
//            I2C_SendData(I2C1, (uint8_t)((0x0000 & 0xFF00) >> 8));
//
//            /*!< Test on EV8 and clear it */
//            uint16_t sEETimeout = 10000;
//            while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTING))
//            {
//               if((sEETimeout--) == 0) {
//                  isr_dbg_slow_printf("Timed out sending addr msb\n");
//               }
//            }
//
//            /*!< Send the EEPROM's internal address to read from: LSB of the address */
//            I2C_SendData(I2C1, (uint8_t)(0x0000 & 0x00FF));
//            sEETimeout = 10000;
//            while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BTF) == RESET)
//            {
//               if((sEETimeout--) == 0) {
//                  isr_dbg_slow_printf("Timed out sending addr lsb\n");
//               }
//            }
//            isr_dbg_slow_printf("Sent addr\n");
            /* Create and publish event for I2CMgr */
//            QEvt *qEvt = Q_NEW( QEvt, I2C_MSTR_MODE_SELECTED_SIG );
//            QF_PUBLISH( (QEvt *)qEvt, AO_I2CMgr );
            break;

         /* Check on EV8 */
         case I2C_EVENT_MASTER_BYTE_TRANSMITTING:
            isr_dbg_slow_printf("I2C_EVENT_MASTER_BYTE_TRANSMITTING\n");
            break;

         case I2C_EVENT_MASTER_BYTE_TRANSMITTED:
            isr_dbg_slow_printf("I2C_EVENT_MASTER_BYTE_TRANSMITTED\n");
            /* Create and publish event for I2CMgr */
//            QEvt *qEvt1 = Q_NEW( QEvt, I2C_MSTR_BYTE_TRANSMITTED_SIG );
//            QF_PUBLISH( (QEvt *)qEvt1, AO_I2CMgr );

            //         if (Tx_Idx == (uint8_t)NumberOfByteToTransmit) {
            //            /* Send STOP condition */
            //            I2C_GenerateSTOP(I2C1, ENABLE);
            //            I2C_ITConfig(I2C1, I2C_IT_EVT | I2C_IT_BUF, DISABLE);
            //         } else {
            //            /* Transmit Data TxBuffer */
            //            I2C_SendData(I2C1, TxBuffer[Tx_Idx++]);
            //         }

            break;

//         case I2C_EVENT_MASTER_BYTE_RECEIVED:
//            isr_dbg_slow_printf("I2C_EVENT_MASTER_BYTE_RECEIVED\n");
//
//            /* Read the byte received */
//            s_I2C_Bus[I2CBus1].pRxBuffer[ s_I2C_Bus[I2CBus1].nRxindex++ ] = I2C_ReceiveData( I2C1 );
//               ++s_I2C_Bus[I2CBus1].nBytesCurrent;
//
//               /* Start
//                * This is a workaround for the STM32 I2C hardware bug.  You have
//                * to send the STOP bit before receiving the last byte.  See the
//                * STM32 errata for more details. */
//               if ( 1 == s_I2C_Bus[I2CBus1].nBytesExpected - s_I2C_Bus[I2CBus1].nBytesCurrent) {
//                  /* Disable Acknowledgment */
//                  I2C_AcknowledgeConfig( I2C1, DISABLE );
//
//                  /* Send STOP Condition */
//                  I2C_GenerateSTOP( I2C1, ENABLE );
//               }/* End */
//
//               /* Check if all the expected bytes have been read in from the bus */
//               if ( s_I2C_Bus[I2CBus1].nBytesExpected == s_I2C_Bus[I2CBus1].nBytesCurrent ) {
//                  /* Create an event and fill it with the data from the RX buffer of the bus.*/
//                  I2CDataEvt *qEvtI2CReadDone = Q_NEW( I2CDataEvt, I2C_READ_DONE_SIG );
//                  qEvtI2CReadDone->i2cDevice = EEPROM;
//                  qEvtI2CReadDone->wBufferLen = s_I2C_Bus[I2CBus1].nBytesCurrent;
//                  MEMCPY(
//                        qEvtI2CReadDone->buffer,
//                        s_I2C_Bus[I2CBus1].pRxBuffer,
//                        qEvtI2CReadDone->wBufferLen
//                  );
//                  QF_PUBLISH( (QEvt *)qEvtI2CReadDone, AO_I2CMgr );
//               }
//
//               /* Re-Enable Acknowledgment to be ready for another reception */
//               I2C_AcknowledgeConfig(I2C1, ENABLE);
//
//               break;
            default:
               isr_dbg_slow_printf("dc: %08x\n", (unsigned int)I2CEvent);
               break;

      }
//   }else {
//      isr_dbg_slow_printf("Invalid I2C device selected: %d\n", s_I2C_Bus[I2CBus1].i2c_cur_dev );
//   }

 isr_dbg_slow_printf("I2C Event\n");
   QK_ISR_EXIT();                           /* inform QK about exiting an ISR */
}

/******************************************************************************/
void I2C1_ER_IRQHandler( void )
{
   QK_ISR_ENTRY();                         /* inform QK about entering an ISR */

   /* Read SR1 register to get I2C error */
   __IO uint16_t regVal = I2C_ReadRegister(I2C1, I2C_Register_SR1) & 0xFF00;
   if (regVal != 0x0000) {
      /* Clears error flags */
      I2C1->SR1 &= 0x00FF;

      isr_dbg_slow_printf("I2C Error: 0x%04x\n", regVal);
   }
   QK_ISR_EXIT();                           /* inform QK about exiting an ISR */
}

/******** Copyright (C) 2014 Datacard. All rights reserved *****END OF FILE****/
