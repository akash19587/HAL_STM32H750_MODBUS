/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define NAND_WAIT_Pin GPIO_PIN_6
#define NAND_WAIT_GPIO_Port GPIOD
#define NAND_WAIT_EXTI_IRQn EXTI9_5_IRQn
#define USART1_CAN485_EN_Pin GPIO_PIN_10
#define USART1_CAN485_EN_GPIO_Port GPIOC
#define CTP_WAKE_Pin GPIO_PIN_12
#define CTP_WAKE_GPIO_Port GPIOA
#define ETH_RST_Pin GPIO_PIN_8
#define ETH_RST_GPIO_Port GPIOC
#define VDD_LCD_EN_Pin GPIO_PIN_6
#define VDD_LCD_EN_GPIO_Port GPIOA
#define LIST_INT0_Pin GPIO_PIN_0
#define LIST_INT0_GPIO_Port GPIOB
#define LIST_INT0_EXTI_IRQn EXTI0_IRQn
void   MX_USART1_UART_Init(void);
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
