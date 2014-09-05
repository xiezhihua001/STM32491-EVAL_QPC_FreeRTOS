/**
 ******************************************************************************
 * @file    stm32f4x7_eth_bsp.c
 * @author  MCD Application Team
 * @version V1.0.0
 * @date    31-October-2011
 * @brief   STM32F4x7 Ethernet hardware configuration.
 ******************************************************************************
 * @attention
 *
 * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
 * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
 * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
 * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
 * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
 * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 *
 * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "stm32f4x7_eth.h"
#include "stm32f4x7_eth_bsp.h"

#include "stm32f4xx.h"                                     /* STM32F4 support */
#include "stm32f4xx_rcc.h"                             /* STM32F4 clk support */
#include "stm32f4xx_dma.h"                             /* STM32F4 DMA support */
#include "stm32f4xx_gpio.h"                           /* STM32F4 gpio support */
#include "stm32f4xx_exti.h"                           /* STM32F4 EXTI support */
#include "stm32f4xx_syscfg.h"         /* STM32F4 System configuration support */
#include "misc.h"                            /* STM32F4 miscellaneous defines */

#include "console_output.h"                         /* Console output support */
#include "lwipopts.h"
#include "bsp.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
__IO uint32_t  EthInitStatus = 0;
__IO uint8_t EthLinkStatus = 0;

/* Private function prototypes -----------------------------------------------*/
static void ETH_GPIO_Config(void);
static void ETH_MACDMA_Config(void);

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  ETH_BSP_Config
 * @param  None
 * @retval None
 */
void ETH_BSP_Config(void)
{
//   RCC_ClocksTypeDef RCC_Clocks;

   /* Configure the GPIO ports for ethernet pins */
   ETH_GPIO_Config();

   /* Configure the Ethernet MAC/DMA */
   ETH_MACDMA_Config();

   if (EthInitStatus == 0) {
      err_slow_printf("Ethernet Init failed\n");
      while(1);
   } else {
      dbg_slow_printf("Ethernet Init succeeded\n");
   }

   /* Configure the PHY to generate an interrupt on change of link status */
//   Eth_Link_PHYITConfig(DP83848_PHY_ADDRESS);

   /* Configure the EXTI for Ethernet link status. */
//   Eth_Link_EXTIConfig();

//   /* Configure Systick clock source as HCLK */
//   SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);
//
//   /* SystTick configuration: an interrupt every 10ms */
//   RCC_GetClocksFreq(&RCC_Clocks);
//   SysTick_Config(RCC_Clocks.HCLK_Frequency / 10000);
}

/**
 * @brief  Configures the Ethernet Interface
 * @param  None
 * @retval None
 */
static void ETH_MACDMA_Config(void)
{
   ETH_InitTypeDef ETH_InitStructure;

   /* Reset ETHERNET on AHB Bus */
   ETH_DeInit();

   /* Software reset */
   ETH_SoftwareReset();

   /* Wait for software reset */
   while (ETH_GetSoftwareResetStatus() == SET);

   /* ETHERNET Configuration --------------------------------------------------*/
   /* Call ETH_StructInit if you don't like to configure all ETH_InitStructure parameter */
   ETH_StructInit(&ETH_InitStructure);

   /* Fill ETH_InitStructure parametrs */
   /*------------------------   MAC   -----------------------------------*/
   ETH_InitStructure.ETH_AutoNegotiation = ETH_AutoNegotiation_Enable;
//   ETH_InitStructure.ETH_AutoNegotiation = ETH_AutoNegotiation_Disable;
//   ETH_InitStructure.ETH_Speed = ETH_Speed_10M;
   ETH_InitStructure.ETH_Mode = ETH_Mode_FullDuplex;

   ETH_InitStructure.ETH_LoopbackMode = ETH_LoopbackMode_Disable;
   ETH_InitStructure.ETH_RetryTransmission = ETH_RetryTransmission_Disable;
   ETH_InitStructure.ETH_AutomaticPadCRCStrip = ETH_AutomaticPadCRCStrip_Disable;
   ETH_InitStructure.ETH_ReceiveAll = ETH_ReceiveAll_Disable;
   ETH_InitStructure.ETH_BroadcastFramesReception = ETH_BroadcastFramesReception_Enable;
   ETH_InitStructure.ETH_PromiscuousMode = ETH_PromiscuousMode_Disable;
   ETH_InitStructure.ETH_MulticastFramesFilter = ETH_MulticastFramesFilter_Perfect;
   ETH_InitStructure.ETH_UnicastFramesFilter = ETH_UnicastFramesFilter_Perfect;
#ifdef CHECKSUM_BY_HARDWARE
   ETH_InitStructure.ETH_ChecksumOffload = ETH_ChecksumOffload_Enable;
#endif

   /*------------------------   DMA   -----------------------------------*/

   /* When we use the Checksum offload feature, we need to enable the Store and Forward mode:
  the store and forward guarantee that a whole frame is stored in the FIFO, so the MAC can insert/verify the checksum, 
  if the checksum is OK the DMA can handle the frame otherwise the frame is dropped */
   ETH_InitStructure.ETH_DropTCPIPChecksumErrorFrame = ETH_DropTCPIPChecksumErrorFrame_Enable;
   ETH_InitStructure.ETH_ReceiveStoreForward = ETH_ReceiveStoreForward_Enable;
   ETH_InitStructure.ETH_TransmitStoreForward = ETH_TransmitStoreForward_Enable;

   ETH_InitStructure.ETH_ForwardErrorFrames = ETH_ForwardErrorFrames_Disable;
   ETH_InitStructure.ETH_ForwardUndersizedGoodFrames = ETH_ForwardUndersizedGoodFrames_Disable;
   ETH_InitStructure.ETH_SecondFrameOperate = ETH_SecondFrameOperate_Enable;
   ETH_InitStructure.ETH_AddressAlignedBeats = ETH_AddressAlignedBeats_Enable;
   ETH_InitStructure.ETH_FixedBurst = ETH_FixedBurst_Enable;
   ETH_InitStructure.ETH_RxDMABurstLength = ETH_RxDMABurstLength_32Beat;
   ETH_InitStructure.ETH_TxDMABurstLength = ETH_TxDMABurstLength_32Beat;
   ETH_InitStructure.ETH_DMAArbitration = ETH_DMAArbitration_RoundRobin_RxTx_2_1;

   /* Configure Ethernet */
   EthInitStatus = ETH_Init(&ETH_InitStructure, DP83848_PHY_ADDRESS);

   /* Enable the Ethernet Rx and Tx Interrupts */
   ETH_DMAITConfig(ETH_DMA_IT_NIS | ETH_DMA_IT_R | ETH_DMA_IT_T, ENABLE);
}

/**
 * @brief  Configures the different GPIO ports.
 * @param  None
 * @retval None
 */
void ETH_GPIO_Config(void)
{
   GPIO_InitTypeDef GPIO_InitStructure;

   /* Configure MCO (PA8) */
//   GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
//   GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
//   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
//   GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
//   GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
//   GPIO_Init(GPIOA, &GPIO_InitStructure);

   /* MII/RMII Media interface selection --------------------------------------*/
#ifdef MII_MODE /* Mode MII with STM324xG-EVAL  */
#ifdef PHY_CLOCK_MCO

   /* Output HSE clock (25MHz) on MCO pin (PA8) to clock the PHY */
   RCC_MCO1Config(RCC_MCO1Source_HSE, RCC_MCO1Div_1);
#endif /* PHY_CLOCK_MCO */

   SYSCFG_ETH_MediaInterfaceConfig(SYSCFG_ETH_MediaInterface_MII);
#elif defined RMII_MODE  /* Mode RMII with STM324xG-EVAL */

   SYSCFG_ETH_MediaInterfaceConfig(SYSCFG_ETH_MediaInterface_RMII);
#endif


   /* Enable SYSCFG clock */
   RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

   /* Enable GPIOs clocks */
   RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB |
         RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOI |
         RCC_AHB1Periph_GPIOG | RCC_AHB1Periph_GPIOH |
         RCC_AHB1Periph_GPIOF, ENABLE);

   /* Ethernet pins configuration ************************************************/
   /*
        ETH_MDIO -------------------------> PA2
        ETH_MDC --------------------------> PC1
        ETH_PPS_OUT ----------------------> PB5
        ETH_MII_CRS ----------------------> PH2 x PA0
        ETH_MII_COL ----------------------> PH3
        ETH_MII_RX_ER --------------------> PI10
        ETH_MII_RXD2 ---------------------> PH6
        ETH_MII_RXD3 ---------------------> PH7
        ETH_MII_TX_CLK -------------------> PC3
        ETH_MII_TXD2 ---------------------> PC2
        ETH_MII_TXD3 ---------------------> PB8
        ETH_MII_RX_CLK/ETH_RMII_REF_CLK---> PA1
        ETH_MII_RX_DV/ETH_RMII_CRS_DV ----> PA7
        ETH_MII_RXD0/ETH_RMII_RXD0 -------> PC4
        ETH_MII_RXD1/ETH_RMII_RXD1 -------> PC5
        ETH_MII_TX_EN/ETH_RMII_TX_EN -----> PG11
        ETH_MII_TXD0/ETH_RMII_TXD0 -------> PG13
        ETH_MII_TXD1/ETH_RMII_TXD1 -------> PG14
    */

   /* Common pin configuration */
   GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
   GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
   GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;

   /* Configure P0, PA1, PA2 and PA7 */
   GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_7;
   GPIO_Init(GPIOA, &GPIO_InitStructure);
//   GPIO_PinAFConfig(GPIOA, GPIO_PinSource0, GPIO_AF_ETH);
   GPIO_PinAFConfig(GPIOA, GPIO_PinSource1, GPIO_AF_ETH);
   GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_ETH);
   GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_ETH);

   /* Configure PB5 and PB8 */
   GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_8;
   GPIO_Init(GPIOB, &GPIO_InitStructure);
   GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_ETH);
   GPIO_PinAFConfig(GPIOB, GPIO_PinSource8, GPIO_AF_ETH);

   /* Configure PC1, PC2, PC3, PC4 and PC5 */
   GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5;
   GPIO_Init(GPIOC, &GPIO_InitStructure);
   GPIO_PinAFConfig(GPIOC, GPIO_PinSource1, GPIO_AF_ETH);
   GPIO_PinAFConfig(GPIOC, GPIO_PinSource2, GPIO_AF_ETH);
   GPIO_PinAFConfig(GPIOC, GPIO_PinSource3, GPIO_AF_ETH);
   GPIO_PinAFConfig(GPIOC, GPIO_PinSource4, GPIO_AF_ETH);
   GPIO_PinAFConfig(GPIOC, GPIO_PinSource5, GPIO_AF_ETH);

   /* Configure PG11, PG14 and PG13 */
   GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_11 | GPIO_Pin_13 | GPIO_Pin_14;
   GPIO_Init(GPIOG, &GPIO_InitStructure);
   GPIO_PinAFConfig(GPIOG, GPIO_PinSource11, GPIO_AF_ETH);
   GPIO_PinAFConfig(GPIOG, GPIO_PinSource13, GPIO_AF_ETH);
   GPIO_PinAFConfig(GPIOG, GPIO_PinSource14, GPIO_AF_ETH);

   /* Configure PH3, PH6, PH7 */
   GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
   GPIO_Init(GPIOH, &GPIO_InitStructure);
//   GPIO_PinAFConfig(GPIOH, GPIO_PinSource2, GPIO_AF_ETH);
//   GPIO_PinAFConfig(GPIOH, GPIO_PinSource3, GPIO_AF_ETH);
   GPIO_PinAFConfig(GPIOH, GPIO_PinSource6, GPIO_AF_ETH);
   GPIO_PinAFConfig(GPIOH, GPIO_PinSource7, GPIO_AF_ETH);

   /* Configure PI10 */
//   GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
//   GPIO_Init(GPIOI, &GPIO_InitStructure);
//   GPIO_PinAFConfig(GPIOI, GPIO_PinSource10, GPIO_AF_ETH);

   /* Enable ETHERNET clock  */
   RCC_AHB1PeriphClockCmd(
         RCC_AHB1Periph_ETH_MAC |
         RCC_AHB1Periph_ETH_MAC_Tx |
         RCC_AHB1Periph_ETH_MAC_Rx,
         ENABLE
   );
}

/**
 * @brief  Configure the PHY to generate an interrupt on change of link status.
 * @param PHYAddress: external PHY address
 * @retval None
 */
uint32_t Eth_Link_PHYITConfig(uint16_t PHYAddress)
{
   uint32_t tmpreg = 0;

   /* Read MICR register */
   tmpreg = ETH_ReadPHYRegister(PHYAddress, PHY_MICR);

   dbg_slow_printf("PHY_MICR: 0x%08\n", tmpreg);

   /* Enable output interrupt events to signal via the INT pin */
   tmpreg |= (uint32_t)PHY_MICR_INT_EN | PHY_MICR_INT_OE;
   dbg_slow_printf("Attempting to write 0x%08x to PHY_MICR\n", tmpreg);
   if(!(ETH_WritePHYRegister(PHYAddress, PHY_MICR, tmpreg)))
   {
      /* Return ERROR in case of write timeout */
      err_slow_printf("Failed to enable interrupts on PHY\n");
      return ETH_ERROR;
   }

   tmpreg = ETH_ReadPHYRegister(PHYAddress, PHY_MICR);

   dbg_slow_printf("PHY_MICR: 0x%08x\n", tmpreg);

   /* Read MISR register */
   tmpreg = ETH_ReadPHYRegister(PHYAddress, PHY_MISR);
   dbg_slow_printf("PHY_MISR: 0x%08x\n", tmpreg);
   /* Enable Interrupt on change of link status */
   tmpreg |= (uint32_t)PHY_MISR_LINK_INT_EN;
   if(!(ETH_WritePHYRegister(PHYAddress, PHY_MISR, tmpreg)))
   {
      /* Return ERROR in case of write timeout */
      err_slow_printf("Failed to enable 'change of link status' interrupt on PHY\n");
      return ETH_ERROR;
   }

   tmpreg = ETH_ReadPHYRegister(PHYAddress, PHY_MISR);

   dbg_slow_printf("PHY_MISR: 0x%08x\n", tmpreg);

   /* Return SUCCESS */
   return ETH_SUCCESS;
}

/**
 * @brief  EXTI configuration for Ethernet link status.
 * @param PHYAddress: external PHY address
 * @retval None
 */
void Eth_Link_EXTIConfig(void)
{
   GPIO_InitTypeDef GPIO_InitStructure;
   EXTI_InitTypeDef EXTI_InitStructure;
   NVIC_InitTypeDef NVIC_InitStructure;

   /* Enable the INT (PB14) Clock */
   RCC_AHB1PeriphClockCmd(ETH_LINK_GPIO_CLK, ENABLE);
   RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

   /* Configure INT pin as input */
   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
   GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
   GPIO_InitStructure.GPIO_Pin = ETH_LINK_PIN;
   GPIO_Init(ETH_LINK_GPIO_PORT, &GPIO_InitStructure);

   /* Connect EXTI Line to INT Pin */
   SYSCFG_EXTILineConfig(ETH_LINK_EXTI_PORT_SOURCE, ETH_LINK_EXTI_PIN_SOURCE);

   /* Configure EXTI line */
   EXTI_InitStructure.EXTI_Line = ETH_LINK_EXTI_LINE;
   EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
   EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
   EXTI_InitStructure.EXTI_LineCmd = ENABLE;
   EXTI_Init(&EXTI_InitStructure);

   /* Enable and set the EXTI interrupt to the highest priority */
//   NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
//   NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
//   NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
//   NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//   NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//   NVIC_Init(&NVIC_InitStructure);

   NVIC_Config( EXTI15_10_IRQn, ETH_LINK_PRIO );
}

/**
 * @brief  This function handles Ethernet link status.
 * @param  None
 * @retval None
 */
void Eth_Link_ITHandler(uint16_t PHYAddress)
{
   printf("EthLinkH\n");
   /* Check whether the link interrupt has occurred or not */
   if(((ETH_ReadPHYRegister(PHYAddress, PHY_MISR)) & PHY_LINK_STATUS) != 0)
   {
      EthLinkStatus = ~EthLinkStatus;


      if(EthLinkStatus != 0) {
         log_slow_printf("Network cable unplugged\n");
      } else {
         log_slow_printf("Network cable plugged in\n");
      }
   }


}

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
