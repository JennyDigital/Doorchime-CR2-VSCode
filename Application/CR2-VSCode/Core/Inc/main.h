/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
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
#include "stm32g4xx_hal.h"

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
#define NSD_MODE_Pin GPIO_PIN_2
#define NSD_MODE_GPIO_Port GPIOB
#define TRIGGER_Pin GPIO_PIN_4
#define TRIGGER_GPIO_Port GPIOB
#define OPT4_Pin GPIO_PIN_5
#define OPT4_GPIO_Port GPIOB
#define OPT3_Pin GPIO_PIN_6
#define OPT3_GPIO_Port GPIOB
#define OPT2_Pin GPIO_PIN_7
#define OPT2_GPIO_Port GPIOB
#define OPT1_Pin GPIO_PIN_9
#define OPT1_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
// Playback engine buffer defines
//
#define PB_BUFF_SZ        512U
#define CHUNK_SZ          ( PB_BUFF_SZ / 2 )
#define HALFCHUNK_SZ      ( CHUNK_SZ / 2 )
#define FIRST             0
#define SECOND            1U

// Bitfields for options
//
#define OPT_AUTO_TRIG     0b1000
#define OPT_VOLUME        0b0111


// DAC Switch defines
//
#define DAC_ON            1
#define DAC_OFF           0

// Stereo file playback option
//#define STEREO_INPUT_MODE

// Trigger Option macros
// These clarify meaning to the code.
//
#define AUTO_TRIG_ENABLED     1
#define AUTO_TRIG_DISABLED    0

// Trigger counter values
//
#define TC_LOW_THRESHOLD      120U
#define TC_HIGH_THRESHOLD     240U
#define TC_MAX                360U
#define TRIGGER_SET           1
#define TRIGGER_CLR           0

// Special development/customer switches
// Adjust as needed.
//
//#define TEST_CYCLING
#define FORCE_TRIGGER_OPT
//#define LOCK_BUILD


// Volume special config
//
#define VOL_SCALING           1       // This is the master volume divider multiplication factor
#define VOL_MULT              1       // Sets the multiplication back so we can use integers.

// Playback status type
//
typedef enum {
  PB_IDLE,
  PB_ERROR,
  PLAYING,
  PLAYING_FAILED
} PB_StatusTypeDef;

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
