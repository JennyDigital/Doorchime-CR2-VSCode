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
#define PB_BUFF_SZ        2048U
#define CHUNK_SZ          ( PB_BUFF_SZ / 2 )
#define HALFCHUNK_SZ      ( CHUNK_SZ / 2 )
#define FIRST             0U
#define SECOND            1U

// Bitfields for options
//
#define OPT_AUTO_TRIG     0b1000
#define OPT_VOLUME        0b0111


// DAC Switch defines
//
#define DAC_ON            1
#define DAC_OFF           0

// Trigger Option macros
// These clarify meaning to the code.
//
#define AUTO_TRIG_ENABLED     1
#define AUTO_TRIG_DISABLED    0

// Trigger counter values
//
// These define the thresholds for setting and clearing the trigger state.
// Calibrated for optimal perfomance.
//
#define TC_LOW_THRESHOLD      120U
#define TC_HIGH_THRESHOLD     240U
#define TC_MAX                360U
#define TRIGGER_SET           1
#define TRIGGER_CLR           0
#define TRIG_TIMEOUT_MAX      5000U   // Number of main loop cycles before timeout
#define TRIG_TIMEOUT_ENABLED  1
#define TRIG_TIMEOUT_DISABLED 0
#define TRIG_TIMEOUT_MS       1000U  // Timeout period in milliseconds

// Special development/customer switches
// Adjust as needed.
//
//#define TEST_CYCLING
#define FORCE_TRIGGER_OPT
//#define LOCK_BUILD

// Volume special config
//
#define VOL_SCALING           4         // This is the master volume divider multiplication factor
#define VOL_MULT              3         // Sets the multiplication back so we can use integers.

// Fade-out configuration
// Number of samples to fade out at the end (prevents pop)
//
#define FADEOUT_SAMPLES       2048U    // About 100ms at 22kHz

// Pause fade-out configuration
// Fade duration when pausing playback (for smooth transition)
//
#define PAUSE_FADEOUT_SAMPLES 1024U    // About 50ms at 22kHz

// Fade-in configuration
// Number of samples to fade in at the start (prevents click)
//
#define FADEIN_SAMPLES        2048U    // About 93ms at 22kHz

// DC blocking filter configuration
// High-pass filter to remove DC offset from audio
//
#define DC_FILTER_ALPHA       64225    // 0.98 in fixed-point (64225/65536)
#define DC_FILTER_SHIFT       16       // Right shift for fixed-point division

// Soft DC filter for 16-bit playback
// Gentler DC removal with lower cutoff frequency (~2Hz vs ~8Hz)
// Runtime-configurable via FilterConfig
#define SOFT_DC_FILTER_ALPHA  65216    // 0.995 in fixed-point (65216/65536)

// Biquad low-pass filter for 16-bit samples
// Second-order IIR filter for superior frequency response
// Runtime-configurable via FilterConfig
#define LPF_16BIT_ALPHA       57344    // 0.875 - gentle filtering (same as LPF_SOFT)

// Soft clipping configuration
// Prevents harsh digital distortion on loud samples
// Runtime-configurable via FilterConfig

// Low-pass filter for 8-bit samples
// Smooths out high-frequency artifacts from 8-bit conversion
// Runtime-configurable via FilterConfig
#define LPF_8BIT_SHIFT        16       // Right shift for fixed-point division
#define LPF_MAKEUP_GAIN_Q16   70779    // ~1.08x post-LPF makeup to offset gentle attenuation (default)

// Low-pass filter aggressiveness levels (alpha coefficients in fixed-point)
#define LPF_VERY_SOFT         61440    // 0.9375 - very gentle filtering, minimal loss of highs
#define LPF_SOFT              57344    // 0.875 - gentle filtering, preserves highs
#define LPF_MEDIUM            49152    // 0.75 - balanced filtering
#define LPF_AGGRESSIVE        40960    // 0.625 - strong filtering, removes more highs

// Noise gate configuration
// Silences samples below threshold to remove background noise
// Runtime-configurable via FilterConfig
#define NOISE_GATE_THRESHOLD  512      // ~1.5% of full scale (adjust 256-2048)

// Filter chain runtime configuration
typedef struct {
  uint8_t enable_16bit_biquad_lpf;
  uint8_t enable_soft_dc_filter_16bit;
  uint8_t enable_8bit_lpf;
  uint8_t enable_noise_gate;
  uint8_t enable_soft_clipping;
  uint32_t lpf_makeup_gain_q16;  // Q16 gain applied after LPF
} FilterConfig_TypeDef;

void SetFilterConfig( const FilterConfig_TypeDef *cfg );
void GetFilterConfig( FilterConfig_TypeDef *cfg );
void SetLpfMakeupGain( float gain );

// Midpoint for Signed/Unsigned silence.
//
#define MIDPOINT_U8           127U
#define MIDPOINT_S16          0

// Playback status type
//
typedef enum {
  PB_Idle,
  PB_Error,
  PB_Playing,
  PB_Paused,
  PB_PlayingFailed
} PB_StatusTypeDef;

// Playback mode type
//
typedef enum {
  Mode_stereo,
  Mode_mono
} PB_ModeTypeDef;

// Low-pass filter aggressiveness type
//
typedef enum {
  LPF_VerySoft,
  LPF_Soft,
  LPF_Medium,
  LPF_Aggressive
} LPF_Level;

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
