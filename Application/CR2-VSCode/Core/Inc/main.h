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
#include "audio_engine.h"
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

// Hardware-specific options
#define OPT_AUTO_TRIG     0b1000
#define OPT_VOLUME        0b0111

// DAC Switch defines
#define DAC_ON            1
#define DAC_OFF           0

// Trigger Option macros
#define AUTO_TRIG_ENABLED     1
#define AUTO_TRIG_DISABLED    0

// Trigger counter values
#define TC_LOW_THRESHOLD      120U
#define TC_HIGH_THRESHOLD     240U
#define TC_MAX                360U
#define TRIGGER_SET           1
#define TRIGGER_CLR           0
//#define TRIG_TIMEOUT_MAX      5000U

#define TRIG_TIMEOUT_MS       1000U

// Special development/customer switches
//#define TEST_CYCLING
#define FORCE_TRIGGER_OPT
//#define LOCK_BUILD
#define NO_SLEEP_MODE

// Uncomment to use digital GPIOs for volume instead of ADC
//#define VOLUME_INPUT_DIGITAL

// Volume response configuration
// NOTE: Non-linear volume response is now managed by the audio engine library.
// Use SetVolumeResponseNonlinear(1) and SetVolumeResponseGamma(2.0f) instead.
// These defines are kept for reference only and are no longer used.
// #define VOLUME_RESPONSE_NONLINEAR    // Enable non-linear (logarithmic) volume response
// #define VOLUME_RESPONSE_GAMMA 2.0f   // Gamma exponent: 2.0 = quadratic (typical for human perception)

// Analog volume ADC scaling (12-bit ADC scaled to 16-bit range)
// ADC max (4095) * 16 = 65520, representing the maximum usable 16-bit volume value
#define VOLUME_ADC_MAX_SCALED 65520U

// Scale factor for master volume input.
#define MASTER_VOLUME_SCALE 8U

// ADC reading inversion option (uncomment to invert ADC so 0 = max volume, 4095 = min volume)
// This is a convenience option for the user who wants to layout their PCB with reverse pinout.
//#define VOLUME_ADC_INVERTED

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
