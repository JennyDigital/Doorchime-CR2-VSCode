/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @copyright (c) 2024-2026 Jennifer A. Gunn (with technological assistance).
  * email       : jennifer.a.gunn@outlook.com
  *
  * MIT License
  * Permission is hereby granted, free of charge, to any person obtaining a copy
  * of this software and associated documentation files (the "Software"), to deal
  * in the Software without restriction, including without limitation the rights
  * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  * copies of the Software, and to permit persons to whom the Software is
  * furnished to do so, subject to the following conditions:
  * The above copyright notice and this permission notice shall be included in all
  * copies or substantial portions of the Software.
  * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  * SOFTWARE.
  *****************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
//
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "cmsis_gcc.h"
#include "stm32g4xx_hal_adc_ex.h"
#include "stm32g4xx_hal_tim_ex.h"
#include "stm32g4xx_hal_tim.h"
#include "audio_engine.h"
#include "lock.h"
#include "interrupt_utils.h"

// Sample data includes, use as needed.
#include "newchallenger11k.h"
#include "guitar.h"
#include "mind_the_door.h"
#include "three_tone_arrival_c.h"
#include "tunnelbarra.h"
#include "tunnelbarra16.h"
#include "konnichiwa.h"
#include "dinding.h"
#include "elevator_ping.h"
#include "Danger.h"
#include "selfdestruct.h"
#include "new_rage32k.h"
#include "accoustic_rock22k.h"
#include "hey_yeah32k.h"
#include "darkblues32k.h"
#include "custom_tritone16k.h"
#include "ocarina_melody32k.h"
#include "theremin_quartet.h"
#include "steves_doorbell.h"
#include "harmony8b.h"
#include "andean_flute.h"
#include "quencho_flute.h"
#include "dreamy.h"
#include "guitar_small.h"
#include "guitar_riff.h"
#include "magic_gong44k.h"
#include "dramatic_organ11k.h"
#include "haunted_organ22k.h"
#include "KillBillWhistle11k.h"
#include "KillBillShort22k.h"
#include "rooster.h"
#include "rooster16b2c.h"
#include "dalby_tritone16b16k.h"
#include "handpan_c16b.h"
#include "guitar_harmony2.h"
#include "into_suffering22k1c.h"
#include "doors_closing.h"
#include "doors_closing11k.h"
#include "doors_opening.h"
#include "doors_opening11k.h"
#include "dalby_tritoneJan26.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
TIM_HandleTypeDef htim7;
I2S_HandleTypeDef hi2s2;
DMA_HandleTypeDef hdma_spi2_tx;

/* USER CODE BEGIN PV */

// Trigger control variables (hardware-specific)
volatile  uint16_t        trig_counter                  = 0;
volatile  uint8_t         trig_timeout_flag             = 0;
volatile  uint16_t        trig_timeout_counter          = 0;
volatile  uint8_t         trig_status                   = TRIGGER_CLR;
volatile  uint16_t        adc_raw                       = 0;

// External variables from audio_engine
extern FilterConfig_TypeDef filter_cfg;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
        void                SystemClock_Config          ( void );
static  void                MX_GPIO_Init                ( void );
static  void                MX_DMA_Init                 ( void );
static  void                MX_I2S2_Init                ( void );                                     // Used in audio_engine
#ifndef VOLUME_INPUT_DIGITAL
static  void                MX_ADC1_Init                ( void );
static  void                MX_TIM7_Init                ( void );
#endif
/* USER CODE BEGIN PFP */

// Hardware-specific function prototypes
        void                DAC_MasterSwitch            ( GPIO_PinState setting );    // Used in audio_engine
        uint16_t            ReadVolume                  ( void );                     // Used in audio_engine
        void                WaitForTrigger              ( uint8_t trig_to_wait_for );
        uint8_t             GetTriggerOption            ( void );
        void                LPSystemClock_Config        ( void );
      // ...existing code...

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */


  HAL_Init();

  /* USER CODE BEGIN Init */
  /* Handle Readout protection. */
  #ifdef LOCK_BUILD
  TOOLS_RDPLevel1_Lock();
  #endif
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  #ifndef VOLUME_INPUT_DIGITAL
  MX_ADC1_Init();
  MX_TIM7_Init();
  #endif
  /* USER CODE BEGIN 2 */

  #ifndef VOLUME_INPUT_DIGITAL
  HAL_ADC_Start_IT( &hadc1 );         // Start ADC in interrupt mode
  HAL_TIM_Base_Start( &htim7 );       // Start TIM7 for ADC triggering
  #endif
  
  /* Initialize audio engine with hardware interface functions */
  if( AudioEngine_Init( DAC_MasterSwitch, ReadVolume, MX_I2S2_Init ) != PB_Idle ) {
    Error_Handler();
  }

  // Configure volume response curve (human perception matched)
  SetVolumeResponseNonlinear( 1 );    // Enable non-linear (logarithmic) response
  SetVolumeResponseGamma( 2.0f );     // Gamma = 2.0 (quadratic, typical for human perception)

  // Small delay to allow hardware to stabilize
  HAL_Delay( 150 );

  // FilterConfig_TypeDef filter_cfg;
  filter_cfg.enable_noise_gate            = 0;  // Noise gate disabled by default; enable as needed
  filter_cfg.enable_16bit_biquad_lpf      = 0;  // 16-bit biquad LPF disabled by default; enable as needed
  filter_cfg.enable_8bit_lpf              = 0;  // 8-bit LPF disabled by default; enable as needed
  filter_cfg.enable_soft_dc_filter_16bit  = 0;  // Soft DC blocking filter for 16-bit samples enabled by default
  filter_cfg.enable_soft_clipping         = 1;  // Soft clipping enabled by default
  filter_cfg.enable_air_effect            = 0;  // Air effect (high-shelf brightening) disabled by default; enable as needed

  // Apply initial filter configuration
  SetFilterConfig( &filter_cfg );

  // Set initial Air Effect boost in dB (runtime adjustable)
  SetAirEffectPresetDb( 0 );       // default +3 dB preset
  
  // Set fade times
  SetFadeInTime(0.15f );                // 150 ms fade-in
  SetFadeOutTime( 0.15f );              // 150 ms fade-out
  SetPauseFadeTime( 0.15f );             // 150 ms pause fade-out
  SetResumeFadeTime( 0.15f );            // 150 ms resume fade-in

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while( true )
  {

    // Wait for playback trigger (if enabled)
    //
#ifndef TEST_CYCLING
    if( GetTriggerOption() == AUTO_TRIG_ENABLED ) {
      WaitForTrigger( TRIGGER_SET );
    }
#endif
 
    // Start playback of samples
    //
    SetLpf16BitLevel( LPF_VerySoft );
    SetSoftClippingEnable( 1 );

    // PlaySample( custom_tritone16k, CUSTOM_TRITONE16K_SZ,
    //   I2S_AUDIOFREQ_16K, 16, Mode_mono ); 
    PlaySample( handpan16bm, HANDPAN16BM_SZ,
      I2S_AUDIOFREQ_44K, 16, Mode_mono );

    WaitForSampleEnd();
    ShutDownAudio();

    // Handle permanent stop if auto-trigger is disabled
    // Otherwise wait for trigger signal.
    // Wake from interrupt on TRIGGER pin.
    //
    if( GetTriggerOption() == AUTO_TRIG_DISABLED ) {
      LPSystemClock_Config();
      HAL_SuspendTick();
      __disable_irq();
      while( 1 ) {  // Infinite sleep loop
        __DSB();    // Data synchronization barrier
        __WFE();    // Wait for event (STOP mode)
        __ISB();    // Instruction synchronization barrier
      }
    }
    else {
#ifndef TEST_CYCLING
      /* Wait for trigger to clear, we need this because people might hold the trigger button */
      WaitForTrigger( TRIGGER_CLR );
#else
      HAL_Delay( 1000 );
  // TEST_CYCLING: demonstrate runtime adjustment by cycling Air Effect boost
  CycleAirEffectPresetDb();
#endif
    }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}


/**
  * @brief System Clock Configuration, Sets the system clock to 150 MHz
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM            = RCC_PLLM_DIV4;
  RCC_OscInitStruct.PLL.PLLN            = 75;
  RCC_OscInitStruct.PLL.PLLP            = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ            = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR            = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType           = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                        | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource        = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider       = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider      = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider      = RCC_HCLK_DIV1;

  if( HAL_RCC_ClockConfig( &RCC_ClkInitStruct, FLASH_LATENCY_4)  != HAL_OK )
  {
    Error_Handler();
  }
}


/** Configures the system clock for low power sleep mode
  * 
  * params: none
  * retval: none
  */
void LPSystemClock_Config( void )
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType       = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                    | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource    = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider   = RCC_SYSCLK_DIV64;
  RCC_ClkInitStruct.APB1CLKDivider  = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider  = RCC_HCLK_DIV1;

  if( HAL_RCC_ClockConfig( &RCC_ClkInitStruct, FLASH_LATENCY_1 ) != HAL_OK )
  {
    Error_Handler();
  }
  HAL_PWREx_ControlVoltageScaling( PWR_REGULATOR_VOLTAGE_SCALE2 );
  HAL_PWREx_EnableLowPowerRunMode();
}


/**
  * @brief I2S2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2S2_Init( void )
{

  /* USER CODE BEGIN I2S2_Init 0 */

  /* USER CODE END I2S2_Init 0 */

  /* USER CODE BEGIN I2S2_Init 1 */

  /* USER CODE END I2S2_Init 1 */
  hi2s2.Instance        = SPI2;
  hi2s2.Init.Mode       = I2S_MODE_MASTER_TX;
  hi2s2.Init.Standard   = I2S_STANDARD_PHILIPS;
  hi2s2.Init.DataFormat = I2S_DATAFORMAT_16B;
  hi2s2.Init.MCLKOutput = I2S_MCLKOUTPUT_DISABLE;

  /* Use the requested playback speed so PlaySample() can change sample rate */
  hi2s2.Init.AudioFreq  = I2S_PlaybackSpeed;
  hi2s2.Init.CPOL       = I2S_CPOL_LOW;
  if( HAL_I2S_Init( &hi2s2)  != HAL_OK )
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2S2_Init 2 */

  /* USER CODE END I2S2_Init 2 */

}


/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init( void )
{

  /* DMA controller clock enable */
  __HAL_RCC_DMAMUX1_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority( DMA1_Channel1_IRQn, 0, 0 );
  HAL_NVIC_EnableIRQ( DMA1_Channel1_IRQn );

}


/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init( void )
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin( NSD_MODE_GPIO_Port, NSD_MODE_Pin, GPIO_PIN_RESET );

  /*Configure GPIO pin : NSD_MODE_Pin */
  GPIO_InitStruct.Pin   = NSD_MODE_Pin;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init( NSD_MODE_GPIO_Port, &GPIO_InitStruct );

  /*Configure GPIO pins : TRIGGER_Pin with interrupt */
  GPIO_InitStruct.Pin   = TRIGGER_Pin;
  GPIO_InitStruct.Mode  = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull  = GPIO_PULLDOWN;
  HAL_GPIO_Init( TRIGGER_GPIO_Port, &GPIO_InitStruct );
  HAL_NVIC_SetPriority( EXTI4_IRQn, 0, 0 );
  HAL_NVIC_EnableIRQ( EXTI4_IRQn );

  /*Configure GPIO pins : OPT4_Pin OPT3_Pin OPT2_Pin OPT1_Pin */
  GPIO_InitStruct.Pin   = OPT4_Pin | OPT3_Pin | OPT2_Pin | OPT1_Pin;
  GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull  = GPIO_PULLDOWN;
  HAL_GPIO_Init( GPIOB, &GPIO_InitStruct );

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}


#ifndef VOLUME_INPUT_DIGITAL
/**
  * @brief TIM7 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM7_Init(void)
{

  /* USER CODE BEGIN TIM7_Init 0 */

  /* USER CODE END TIM7_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM7_Init 1 */

  /* USER CODE END TIM7_Init 1 */
  htim7.Instance                  = TIM7;
  htim7.Init.Prescaler            = 150-1;
  htim7.Init.CounterMode          = TIM_COUNTERMODE_UP;
  htim7.Init.Period               = 39999;
  htim7.Init.AutoReloadPreload    = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim7)   != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
  if( HAL_TIMEx_MasterConfigSynchronization( &htim7, &sMasterConfig ) != HAL_OK )
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM7_Init 2 */

  /* USER CODE END TIM7_Init 2 */

}
#endif


#ifndef VOLUME_INPUT_DIGITAL
/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init( void )
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_MultiModeTypeDef multimode = {0};
  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance                      = ADC1;
  hadc1.Init.ClockPrescaler           = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution               = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign                = ADC_DATAALIGN_RIGHT;
  hadc1.Init.GainCompensation         = 0;
  hadc1.Init.ScanConvMode             = ADC_SCAN_DISABLE;
  hadc1.Init.EOCSelection             = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait         = DISABLE;
  hadc1.Init.ContinuousConvMode       = DISABLE;
  hadc1.Init.NbrOfConversion          = 1;
  hadc1.Init.DiscontinuousConvMode    = DISABLE;
  hadc1.Init.ExternalTrigConv         = ADC_EXTERNALTRIG_T7_TRGO;
  hadc1.Init.ExternalTrigConvEdge     = ADC_EXTERNALTRIGCONVEDGE_RISING;
  hadc1.Init.DMAContinuousRequests    = DISABLE;
  hadc1.Init.Overrun                  = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.OversamplingMode         = DISABLE;
  if( HAL_ADC_Init( &hadc1 ) != HAL_OK )
  {
    Error_Handler();
  }

  /** Configure the ADC multi-mode
  */
  multimode.Mode = ADC_MODE_INDEPENDENT;
  if( HAL_ADCEx_MultiModeConfigChannel( &hadc1, &multimode ) != HAL_OK )
  {
    Error_Handler();
  }


  /** Configure Regular Channel
  */
  sConfig.Channel       = ADC_CHANNEL_10;
  sConfig.Rank          = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime  = ADC_SAMPLETIME_2CYCLES_5;
  sConfig.SingleDiff    = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber  = ADC_OFFSET_NONE;
  sConfig.Offset        = 0;
  if( HAL_ADC_ConfigChannel( &hadc1, &sConfig ) != HAL_OK )
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}
#endif


/* USER CODE BEGIN 4 */

/** Changes NSD_MODE_Pin pin to control the DAC between on and Shutdown
  *
  * param: DAC_OFF (0) or DAC_ON (non zero)
  * retval: none
  *
  * NOTE: HAL_GPIO_WritePin() is not reentrant; protect with interrupt disable
  * to prevent race conditions on GPIO register access.
  */
void DAC_MasterSwitch( GPIO_PinState setting )
{
  /* Disable interrupts to protect GPIO register access (atomic operation) */
  ATOMIC_ENTER();
  
  /* Change the setting */
  HAL_GPIO_WritePin( NSD_MODE_GPIO_Port, NSD_MODE_Pin, setting );
  
  /* Restore interrupt state before delay to avoid blocking IRQs unnecessarily */
  ATOMIC_EXIT();
  
  /* Wait 10mS to allow the MAX98357A to settle */
  HAL_Delay( 10 );
}


/** Read the master volume level for playback.
  *
  * params: none
  * retval: uint16_t between 1 and 65535 for volume scaling.
  *
  * Note: Non-linear volume response is now handled internally by the audio engine.
  *       Use SetVolumeResponseNonlinear() and SetVolumeResponseGamma() to configure.
  */
uint16_t ReadVolume( void )
{
  uint16_t volume = 0;

  #ifdef VOLUME_INPUT_DIGITAL
    // Use digital GPIOs for volume (3 bits, scaled to 1-65535)
    uint8_t v =
      ( ( (OPT3_GPIO_Port->IDR & OPT3_Pin) != 0 ) << 2 ) |
      ( ( (OPT2_GPIO_Port->IDR & OPT2_Pin) != 0 ) << 1 ) |
      ( ( (OPT1_GPIO_Port->IDR & OPT1_Pin) != 0 ) << 0 );

    v = 7 - v;        // Invert so 0b000 = max volume, 0b111 = min volume
    uint32_t scaled = ( (uint32_t)v * 65535U ) / 7U;  // Map 0-7 to 0-65535
    volume = (uint16_t)scaled;
  #else
    // Use 12-bit ADC value (0-4095) for linear volume
    // Scale 12-bit ADC directly to match 16-bit volume range with 16x scaling factor
    // (4095 * 16 = 65520, close to full 65535 range)
    uint32_t lin = (uint32_t)adc_raw * MASTER_VOLUME_SCALE;         // Scale 12-bit to acceptable range
    if( lin > VOLUME_ADC_MAX_SCALED ) lin = VOLUME_ADC_MAX_SCALED;  // Cap at maximum ADC * 16
    volume = (uint16_t)lin;
  #endif

  /* Analog signals have noise; clamp low values to avoid noise-induced ultra-quiet audio */
  if( volume < 32U ) volume = 32U;

  /* Return raw volume - audio engine applies non-linear response curve internally */
  return volume;
} 

/* ADC Conversion Complete Callback */
void HAL_ADC_ConvCpltCallback( ADC_HandleTypeDef *hadc )
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hadc);
  adc_raw = HAL_ADC_GetValue( &hadc1 );
}


/** Wait for the trigger signal
 *
 * Waits until the trigger signal is received.
 *
 * params: none
 * retval: none
 *
 */
void WaitForTrigger( uint8_t trig_to_wait_for )
{
  while( 1 ) {
    trig_timeout_flag = 0;
    while( trig_status != trig_to_wait_for ) {
      HAL_Delay( 1 );
      trig_timeout_counter++;
      if( trig_timeout_counter >= TRIG_TIMEOUT_MS ) {
        trig_timeout_flag = 1;
        trig_timeout_counter = 0;
        break;
      }
    }
    if( trig_status == trig_to_wait_for ) return;

#ifndef NO_SLEEP_MODE
  /* Prep for sleep mode */
#ifndef VOLUME_INPUT_DIGITAL
    HAL_TIM_Base_Stop( &htim7 );                // Stop TIM7 to prevent ADC triggers during sleep
    HAL_ADC_Stop_IT( &hadc1 );                  // Stop ADC in interrupt mode
#endif
    LPSystemClock_Config();                     // Reduce clock speed for low power sleep
    HAL_SuspendTick();                          // Stop SysTick interrupts to prevent wakeups
    __HAL_GPIO_EXTI_CLEAR_IT( TRIGGER_Pin );    // Clear EXTI pending bit
#ifndef VOLUME_INPUT_DIGITAL
    __HAL_TIM_CLEAR_IT( &htim7, TIM_IT_UPDATE );
#endif
    
    /* Memory barrier to ensure all writes complete before sleep */
    __DSB();
    __ISB();

    /* Enter low power sleep mode and wait for the trigger */
    HAL_PWR_EnterSLEEPMode( PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFI );

    /* Rise from your slumber mighty microcontroller! */
    __HAL_GPIO_EXTI_CLEAR_IT( TRIGGER_Pin );    // Clear EXTI pending bit
#ifndef VOLUME_INPUT_DIGITAL
    __HAL_TIM_CLEAR_IT( &htim7, TIM_IT_UPDATE );
#endif
    HAL_PWREx_DisableLowPowerRunMode();
    SystemClock_Config();
    HAL_ResumeTick();
#ifndef VOLUME_INPUT_DIGITAL
    HAL_ADC_Start_IT( &hadc1 );                 // Restart ADC in interrupt mode
    HAL_TIM_Base_Start( &htim7 );               // Restart TIM7 for ADC triggering
#endif  
#endif
  }
}


/** Returns the state of the trigger option pad.
  *
  * params: none
  * retval: trigger option pad state, 1 Being enabled.
  *
  */
uint8_t GetTriggerOption( void )
{
#if defined (TEST_CYCLING) || defined (FORCE_TRIGGER_OPT)
  return 1;
#else
  return HAL_GPIO_ReadPin( OPT4_GPIO_Port, OPT4_Pin );
#endif
}


/* Shuts down audio playback and DAC 
 * 
 * @params: none
 * @retval: none
 */
// ...existing code...


/** Systick IRQ including trigger handling
  *
  * params: none
  * retval: none
  */
void HAL_IncTick( void )
{
  uwTick += uwTickFreq;

  if( TRIGGER_GPIO_Port->IDR & TRIGGER_Pin ) {    // Trigger is high, so increase counter.
    if( trig_counter < TC_MAX ) trig_counter++;
  }
  else {                                          // Trigger is low, so decrease counter.
    if ( trig_counter > 0 ) trig_counter--;
  }

  /* Handle trigger status with hysteresis */
  if( trig_counter < TC_LOW_THRESHOLD )   trig_status = TRIGGER_CLR;
  if( trig_counter > TC_HIGH_THRESHOLD )  trig_status = TRIGGER_SET;
}


/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* Shutdown playback, disable interrupts amd switch off DAC */
  __disable_irq();
  HAL_I2S_DMAStop( &AUDIO_ENGINE_I2S_HANDLE );
  DAC_MasterSwitch( DAC_OFF );
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
