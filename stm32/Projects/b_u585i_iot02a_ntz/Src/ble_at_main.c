/**
  ******************************************************************************
  * @file    BLE/BLE_AT_Client/Src/main.c
  * @author  MCD Application Team
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>

#include "logging_levels.h"
/* define LOG_LEVEL here if you want to modify the logging level from the default */
#define LOG_LEVEL    LOG_INFO
#include "logging.h"
#ifdef printf
#undef printf
#endif
#define printf LogInfo

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "main.h"
#include "stm32wb_at.h"
#include "stm32wb_at_ble.h"
#include "stm32wb_at_client.h"
#include "ble_at_appli.h"

UART_HandleTypeDef huart4 =
{
    .Instance                    = UART4,
    .Init.BaudRate               = 9600,
    .Init.WordLength             = UART_WORDLENGTH_8B,
    .Init.StopBits               = UART_STOPBITS_1,
    .Init.Parity                 = UART_PARITY_NONE,
    .Init.Mode                   = UART_MODE_TX_RX,
    .Init.HwFlowCtl              = UART_HWCONTROL_NONE,
    .Init.OverSampling           = UART_OVERSAMPLING_16,
    .Init.OneBitSampling         = UART_ONE_BIT_SAMPLE_DISABLE,
    .Init.ClockPrescaler         = UART_PRESCALER_DIV1,
    .AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT,
};

static BaseType_t xInitBtUart( void );

void ble_at_main(void)
{
  uint8_t status = 0;
  xInitBtUart();

  printf("--------------------------------------------\n");
  printf("Start of the STM32WB5M module AT example.\n");
  printf("Run ST BLE sensor application on your smartphone and connect to you device.\n");
  printf("Press user button:\n");
  printf("  - once to notify a value.\n");
  printf("  - twice to toggle the BLE service.\n");
  printf("--------------------------------------------\n");

  vTaskDelay(pdMS_TO_TICKS(4000));
  status |= stm32wb_at_Init(&at_buffer_isr[0], sizeof(at_buffer_isr));
  status |= stm32wb_at_client_Init();

  /* Test the UART communication link with BLE module */
  status |= stm32wb_at_client_Query(BLE_TEST);
  vTaskDelay(pdMS_TO_TICKS(1000));

  if(status != 0)
  {
    Error_Handler();
  }
  /* Send a BLE AT command to start the BLE P2P server application */
  stm32wb_at_BLE_SVC_t param_BLE_SVC;
  global_svc_index = 1;
  param_BLE_SVC.index = global_svc_index;
  stm32wb_at_client_Set(BLE_SVC, &param_BLE_SVC);

  BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);
  while (1) {
	  vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void UART4_IRQHandler(void)
{
  /* USER CODE BEGIN UART4_IRQn 0 */

  /* USER CODE END UART4_IRQn 0 */
  HAL_UART_IRQHandler(&huart4);
  /* USER CODE BEGIN UART4_IRQn 1 */

  /* USER CODE END UART4_IRQn 1 */
}

#if 0
static void MX_USART4_UART_Init(void)
{
  if (HAL_UART_Init(&huart4) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart4, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart4, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart4) != HAL_OK)
  {
    Error_Handler();
  }
}
#endif
// NOTE: ignore USART1. Should not be called.
void vUart4MspInitCallback(UART_HandleTypeDef* huart)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
  if(huart->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspInit 0 */

  /* USER CODE END USART1_MspInit 0 */
  /** Initializes the peripherals clock
  */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
    PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    /* Peripheral clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART1 GPIO Configuration
    PA10     ------> USART1_RX
    PA9     ------> USART1_TX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN USART1_MspInit 1 */

  /* USER CODE END USART1_MspInit 1 */
  }
  if(huart->Instance==UART4)
  {
  /* USER CODE BEGIN USART4_MspInit 0 */

  /* USER CODE END USART4_MspInit 0 */
  /** Initializes the peripherals clock
  */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_UART4;
    PeriphClkInit.Uart4ClockSelection = RCC_UART4CLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    /* Peripheral clock enable */
    __HAL_RCC_UART4_CLK_ENABLE();

    __HAL_RCC_GPIOC_CLK_ENABLE();
    /**UART4 GPIO Configuration
    PC10     ------> UART4_TX
    PC11     ------> UART4_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF8_UART4;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* UART4 interrupt Init */
    HAL_NVIC_SetPriority(UART4_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(UART4_IRQn);
  /* USER CODE BEGIN UART4_MspInit 1 */

  /* USER CODE END UART4_MspInit 1 */
  }

}

// NOTE: ignore USART1. Should not be called.
void vUart4MspDeInitCallback(UART_HandleTypeDef* huart)
{
  if(huart->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspDeInit 0 */
    __HAL_RCC_USART1_FORCE_RESET();
    __HAL_RCC_USART1_RELEASE_RESET();
  /* USER CODE END USART1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART1_CLK_DISABLE();

    /**USART1 GPIO Configuration
    PA10     ------> USART1_RX
    PA9     ------> USART1_TX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_10|GPIO_PIN_9);

  /* USER CODE BEGIN USART1_MspDeInit 1 */

  /* USER CODE END USART1_MspDeInit 1 */
  }
  if(huart->Instance==UART4)
  {
  /* USER CODE BEGIN UART4_MspDeInit 0 */
    __HAL_RCC_UART4_FORCE_RESET();
    __HAL_RCC_UART4_RELEASE_RESET();
  /* USER CODE END UART4_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_UART4_CLK_DISABLE();

    /**UART4 GPIO Configuration
    PC10     ------> UART4_TX
    PC11     ------> UART4_RX
    */
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_11|GPIO_PIN_11);

  /* USER CODE BEGIN UART4_MspDeInit 1 */

  /* USER CODE END UART4_MspDeInit 1 */
  }

}

static BaseType_t xInitBtUart( void )
{
    HAL_StatusTypeDef xHalRslt = HAL_OK;

    ( void ) HAL_UART_DeInit( &huart4 );

    xHalRslt |= HAL_UART_RegisterCallback( &huart4, HAL_UART_MSPINIT_CB_ID, vUart4MspInitCallback );
    xHalRslt |= HAL_UART_RegisterCallback( &huart4, HAL_UART_MSPDEINIT_CB_ID, vUart4MspDeInitCallback );

    if( xHalRslt == HAL_OK )
    {
        /* HAL_UART_Init calls mspInitCallback internally */
        xHalRslt = HAL_UART_Init( &huart4 );
    }

    /* Register callbacks */
    if( xHalRslt == HAL_OK )
    {
        xHalRslt |= ble_at_appli_register_uart_callbacks(&huart4);
    }

    /* Set the FIFO thresholds */
    if( xHalRslt == HAL_OK )
    {
        xHalRslt |= HAL_UARTEx_SetTxFifoThreshold( &huart4, UART_TXFIFO_THRESHOLD_1_8 );
    }

    if( xHalRslt == HAL_OK )
    {
        xHalRslt |= HAL_UARTEx_SetRxFifoThreshold( &huart4, UART_RXFIFO_THRESHOLD_1_8 );
    }

    /* Enable FIFO mode */
    if( xHalRslt == HAL_OK )
    {
        xHalRslt |= HAL_UARTEx_DisableFifoMode( &huart4 );
    }
    return( xHalRslt == HAL_OK );
}


/* Should only be called before the scheduler has been initialized / after an assertion has occurred */
UART_HandleTypeDef * vInitUartBtEarly( void )
{
    ( void ) HAL_UART_DeInit( &huart4 );

    ( void ) HAL_UART_RegisterCallback( &huart4, HAL_UART_MSPINIT_CB_ID, vUart4MspInitCallback );
    ( void ) HAL_UART_RegisterCallback( &huart4, HAL_UART_MSPDEINIT_CB_ID, vUart4MspDeInitCallback );
    ( void ) HAL_UART_Init( &huart4 );


    return &huart4;
}


