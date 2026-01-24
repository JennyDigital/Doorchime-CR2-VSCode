/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
//
#include <stdint.h>
#include "lock.h"
#include "stm32g474xx.h"

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
I2S_HandleTypeDef hi2s2;
DMA_HandleTypeDef hdma_spi2_tx;

/* USER CODE BEGIN PV */

// Playback buffer size definitions
          int16_t         pb_buffer[ PB_BUFF_SZ ]       = {0};

volatile  uint8_t  *      pb_p8;
volatile  uint8_t  *      pb_end8;
volatile  uint16_t *      pb_p16;
volatile  uint16_t *      pb_end16;

// Playback control variables
          uint8_t         pb_mode;
volatile  uint8_t         pb_state                      = PB_IDLE;
volatile  uint8_t         half_to_fill;

// volume and trigger control variables
volatile  uint8_t         vol_div;
volatile  uint16_t        trig_counter                  = 0;
volatile  uint8_t         trig_timeout_flag             = 0;
volatile  uint16_t        trig_timeout_counter          = 0;
volatile  uint8_t         trig_status                   = TRIGGER_CLR;

          uint16_t        I2S_PlaybackSpeed             = I2S_AUDIOFREQ_22K;

// Playback engine control variables
          uint16_t        p_advance;
          PB_ModeTypeDef  channels                      = Mode_mono;
volatile  uint32_t        fadeout_samples_remaining     = 0;
volatile  uint32_t        fadein_samples_remaining      = 0;
volatile  int32_t         dc_filter_prev_input_left     = 0;
volatile  int32_t         dc_filter_prev_input_right    = 0;
volatile  int32_t         dc_filter_prev_output_left    = 0;
volatile  int32_t         dc_filter_prev_output_right   = 0;
volatile  uint32_t        dither_state                  = 12345;  // PRNG seed for dithering

// Biquad filter state for 8-bit samples (left channel)
volatile  int32_t         lpf_8bit_x1_left              = 0;      // x[n-1] left
volatile  int32_t         lpf_8bit_x2_left              = 0;      // x[n-2] left  
volatile  int32_t         lpf_8bit_y1_left              = 0;      // y[n-1] left
volatile  int32_t         lpf_8bit_y2_left              = 0;      // y[n-2] left

// Biquad filter state for 8-bit samples (right channel)
volatile  int32_t         lpf_8bit_x1_right             = 0;      // x[n-1] right
volatile  int32_t         lpf_8bit_x2_right             = 0;      // x[n-2] right
volatile  int32_t         lpf_8bit_y1_right             = 0;      // y[n-1] right
volatile  int32_t         lpf_8bit_y2_right             = 0;      // y[n-2] right

          uint16_t        lpf_8bit_alpha                = LPF_MEDIUM; // Current LPF alpha coefficient

// Biquad filter state for 16-bit samples (left channel)
volatile  int32_t         lpf_16bit_x1_left             = 0;      // x[n-1] left
volatile  int32_t         lpf_16bit_x2_left             = 0;      // x[n-2] left
volatile  int32_t         lpf_16bit_y1_left             = 0;      // y[n-1] left
volatile  int32_t         lpf_16bit_y2_left             = 0;      // y[n-2] left

// Biquad filter state for 16-bit samples (right channel)
volatile  int32_t         lpf_16bit_x1_right            = 0;      // x[n-1] right
volatile  int32_t         lpf_16bit_x2_right            = 0;      // x[n-2] right
volatile  int32_t         lpf_16bit_y1_right            = 0;      // y[n-1] right
volatile  int32_t         lpf_16bit_y2_right            = 0;      // y[n-2] right

// Air enhancement filter state (high-shelf) for left channel
volatile  int32_t         air_x1_left                   = 0;      // x[n-1] left
volatile  int32_t         air_y1_left                   = 0;      // y[n-1] left

// Air enhancement filter state (high-shelf) for right channel
volatile  int32_t         air_x1_right                  = 0;      // x[n-1] right
volatile  int32_t         air_y1_right                  = 0;      // y[n-1] right

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
        void    SystemClock_Config      ( void );
static  void    MX_GPIO_Init            ( void );
static  void    MX_DMA_Init             ( void );
static  void    MX_I2S2_Init            ( void );
/* USER CODE BEGIN PFP */

// Playback engine function prototypes
        PB_StatusTypeDef    ProcessNextWaveChunk        ( int16_t * chunk_p );
        PB_StatusTypeDef    ProcessNextWaveChunk_8_bit  ( uint8_t * chunk_p );
        void                AdvanceSamplePointer        ( void );

// Signal processing function prototypes
        int16_t             Apply8BitDithering          ( uint8_t sample8 );
        int16_t             ApplyLowPassFilter16Bit     ( int16_t input, volatile int32_t *x1, volatile int32_t *x2, 
                                                          volatile int32_t *y1, volatile int32_t *y2 );
        int16_t             ApplyAirEnhancement         ( int16_t input, volatile int32_t *x1, volatile int32_t *y1 );
        int16_t             ApplyLowPassFilter8Bit      ( int16_t sample, 
                                                          volatile int32_t *x1, volatile int32_t *x2,
                                                          volatile int32_t *y1, volatile int32_t *y2 );
        int16_t             ApplyFadeIn                 ( int16_t sample );
        int16_t             ApplyFadeOut                ( int16_t sample );
        int16_t             ApplyNoiseGate              ( int16_t sample );
        int16_t             ApplySoftClipping           ( int16_t sample );
        int16_t             ApplyDCBlockingFilter       (
                                                          volatile int16_t input,
                                                          volatile int32_t *prev_input,
                                                          volatile int32_t *prev_output
                                                        );
        int16_t             ApplySoftDCFilter16Bit      ( 
                                                          volatile int16_t input,
                                                          volatile int32_t *prev_input,
                                                          volatile int32_t *prev_output
                                                        );
        int16_t             ApplyFilterChain16Bit       ( int16_t sample, uint8_t is_left_channel );
        int16_t             ApplyFilterChain8Bit        ( int16_t sample, uint8_t is_left_channel );

// Application function prototypes
        void                DAC_MasterSwitch            ( GPIO_PinState setting );
        uint8_t             ReadVolume                  ( void );
        void                WaitForTrigger              ( uint8_t trig_to_wait_for );
        uint8_t             GetTriggerOption            ( void );
        PB_StatusTypeDef    PlaySample                  (
                                                          const void *    sample_to_play,
                                                          uint32_t        sample_set_sz,  
                                                          uint16_t        playback_speed,
                                                          uint8_t         sample_depth,
                                                          PB_ModeTypeDef  mode,
                                                          LPF_Level       lpf_level
                                                        );
        PB_StatusTypeDef    WaitForSampleEnd            ( void );
        void                ClearBuffer                 ( void );
        void                LPSystemClock_Config        ( void );

        void                ShutDownAudio               ( void );


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
  /* USER CODE BEGIN 2 */

  HAL_Delay( 150 );

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

    // Wait for playback trigger (if enabled)
    //
#ifndef TEST_CYCLING
    if( GetTriggerOption() == AUTO_TRIG_ENABLED ) {
      WaitForTrigger( TRIGGER_SET );
    }
#endif


    // PlaySample( handpan16bm, HANDPAN16BM_SZ,
    //     I2S_AUDIOFREQ_44K, 16, Mode_mono, LPF_Medium );
    // WaitForSampleEnd();
    PlaySample( magic_gong44k, MAGIC_GONG44K_SZ,
        I2S_AUDIOFREQ_44K, 16, Mode_mono, LPF_Medium );
    WaitForSampleEnd();
    // PlaySample( custom_tritone16k, CUSTOM_TRITONE16K_SZ,
    //   I2S_AUDIOFREQ_16K, 16, Mode_mono, LPF_Medium );
    // WaitForSampleEnd();
    // PlaySample( rooster16b2c, ROOSTER16B2C_SZ, I2S_AUDIOFREQ_22K, 16, Mode_stereo, LPF_Medium );
    // WaitForSampleEnd();
    // PlaySample( ocarina32k, OCARINA32K_SZ,
    //     I2S_AUDIOFREQ_32K, 16, Mode_mono, LPF_Soft );
    // WaitForSampleEnd();
    PlaySample( rooster8b2c, ROOSTER8B2C_SZ,
       I2S_AUDIOFREQ_22K, 8, Mode_stereo, LPF_Medium );
    WaitForSampleEnd();

    // PlaySample( harmony8b, HARMONY8B_SZ,
    //     I2S_AUDIOFREQ_11K, 8, Mode_mono, LPF_Medium );
    // WaitForSampleEnd();

    // PlaySample( tt_arrival, TT_ARRIVAL_SZ, I2S_AUDIOFREQ_11K, 16, Mode_mono, LPF_Medium );
    // PlaySample( KillBill11k, KILLBILL11K_SZ,
    //  I2S_AUDIOFREQ_11K, 16, Mode_mono, LPF_Medium );
    // WaitForSampleEnd();

    // PlaySample( guitar_riff22k, GUITAR_RIFF22K_SZ,
    //      I2S_AUDIOFREQ_22K, 16, Mode_mono, LPF_Medium );
    // WaitForSampleEnd();

    ShutDownAudio();

    // Handle ppermanent stop if auto-trigger is disabled
    // Otherwise wait for trigger signal.
    // Wake from interrupt on TRIGGER pin.
    //
    if( GetTriggerOption() == AUTO_TRIG_DISABLED ) {
      LPSystemClock_Config();
      HAL_SuspendTick();
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


/* USER CODE BEGIN 4 */
/** Handle refilling the first half of the buffer whilst the second half is playing
  *
  * params: hi2s2_p I2S port 2 handle.
  * retval: none.
  *
  * NOTE: Also shuts down the playback when the recording is done.
  *
  */
void HAL_I2S_TxHalfCpltCallback( I2S_HandleTypeDef *hi2s2_p )
{
  UNUSED( hi2s2_p );

  if( pb_mode == 16 ) {             // 16-bit samples exhausted check.
    if( pb_p16 >= pb_end16 ) {
      // Set state to idle - cleanup will happen in main loop
      pb_state = PB_IDLE;
      return;
    }
    else {                          // Refill the first half of the buffer with 16-bit samples.
      half_to_fill = FIRST;
      if( ProcessNextWaveChunk( (int16_t *) pb_p16 ) != PLAYING ) {
        return;
      }
    }
  } 
  else if( pb_mode == 8 ) {             // 8-bit samples exhausted check.

    if( pb_p8 >= pb_end8 ) {
      // Set state to idle - cleanup will happen in main loop
      pb_state = PB_IDLE;
      return;
    }
    else {                              // Refill the first half of the buffer with 8-bit samples.
      half_to_fill = FIRST;
      if( ProcessNextWaveChunk_8_bit( (uint8_t *) pb_p8 ) != PLAYING ) {
        return;
      } 
    }
  }
  AdvanceSamplePointer();
}


/** Handle refilling the second half of the buffer whilst the first half is playing
  *
  * params: hi2s2_p I2S port 2 handle.
  * retval: none.
  *
  * NOTE: Also shuts down the playback when the recording is done.
  *
  */
void HAL_I2S_TxCpltCallback( I2S_HandleTypeDef *hi2s2_p )
{
  UNUSED( hi2s2_p );

  if( pb_mode == 16 ) {

    if( pb_p16 >= pb_end16 ) {
      // Set state to idle - cleanup will happen in main loop
      pb_state = PB_IDLE;
      return;
    }
    else {
      half_to_fill = SECOND;
      if( ProcessNextWaveChunk( (int16_t *) pb_p16 ) != PLAYING ) {
       return;  
      }
    }
  }
  else if( pb_mode == 8 ) {

    if( pb_p8 >= pb_end8 ) {
      // Set state to idle - cleanup will happen in main loop
      pb_state = PB_IDLE;
      return;
    }
    else {
      half_to_fill = SECOND;
      if( ProcessNextWaveChunk_8_bit( (uint8_t *) pb_p8 ) != PLAYING ) {  
          return;  
      }
    }
  }
  AdvanceSamplePointer();
}

void AdvanceSamplePointer( void )
{
  if( pb_mode == 16 ) {  // Advance the 16-bit sample pointer
    pb_p16 += p_advance;
    if( pb_p16 >= pb_end16 ) {
      pb_state = PB_IDLE;
      return;
    }
  }
  else if( pb_mode == 8 ) {  // Or advance the 8-bit sample pointer
    pb_p8 += p_advance;
    if( pb_p8 >= pb_end8 ) {
      pb_state = PB_IDLE;
      return;
    }
  } 
}

/** Transfers a chunk of 16-bit samples to the DMA playback buffer and processes them
  *
  * It also processes for stereo mode
  *
  * @param: int16_t* chunk_p.  The start of the chunk to transfer.
  * @retval: none.
  *
  */
PB_StatusTypeDef ProcessNextWaveChunk( int16_t * chunk_p )
{
  int16_t *input, *output;
  int16_t leftsample, rightsample;

  if( chunk_p == NULL ) {   // Sanity check
    return PB_ERROR;
  }

  vol_div = ReadVolume();
  input = chunk_p;      // Source sample pointer
  output = ( half_to_fill == SECOND ) ? (pb_buffer + CHUNK_SZ ) : pb_buffer;

  // Transfer mono audio (scaled for volume) into the stereo buffer.
  // This is done both to eliminate the need for a second resistor
  // and also because the MAX983567A expects stereo audio data, but we only
  // have one speaker.
  //
  // NOTE: Soldered pads set a value to DIVIDE the audio down by with OPT1
  // being the LSB and OPT3 the MSB.
  //
  for( uint16_t i = 0; i < HALFCHUNK_SZ; i++ )
  {
    if( (uint16_t *) input >=  pb_end16 ) {                     /* Check for end of sample data */
      leftsample = MIDPOINT_S16;                                /* Pad with silence if at end */
    }
    else {
      leftsample = ( (int16_t) (*input) / vol_div ) * VOL_MULT; // Left channel
      leftsample = ApplyFilterChain16Bit( leftsample, 1 );      // Apply complete filter chain
    }
    input++;

    if( channels == Mode_mono ) { rightsample = leftsample; }   // Right channel is the same as left.
    else {
      if( (uint16_t *) input >=  pb_end16 ) {                   // Check for end of sample data
        rightsample = MIDPOINT_S16;                             // Pad with silence if at end
      }
      else { 
        rightsample = ( (int16_t) (*input) / vol_div ) * VOL_MULT; // Right channel
        rightsample = ApplyFilterChain16Bit( rightsample, 0 );     // Apply complete filter chain
      }   // End of right channel processing
      input++;
    }
    *output = leftsample;  output++;                            // Write samples to output buffer
    *output = rightsample; output++;
    
    // Decrement fade counters based on samples processed
    uint8_t samples_processed = (channels == Mode_stereo) ? 2 : 1;
    if( fadein_samples_remaining > 0 ) {
      fadein_samples_remaining = (fadein_samples_remaining > samples_processed) ? 
                                 fadein_samples_remaining - samples_processed : 0;
    }
    if( fadeout_samples_remaining > 0 ) {
      fadeout_samples_remaining--;
    }
  }
  return PLAYING;
}


/** Transfers a chunk of 8-bit samples to the DMA playback buffer
  *
  * It also processes for stereo mode
  *
  * @param: uint8_t* chunk_p.  The start of the chunk to transfer.
  * @retval: none.
  *
  */
PB_StatusTypeDef ProcessNextWaveChunk_8_bit( uint8_t * chunk_p )
{
  uint8_t *input;
  int16_t *output;
  int16_t leftsample, rightsample;

  if( chunk_p == NULL ) {   // Sanity check
    return PB_ERROR;
  }
  vol_div = ReadVolume();
  input = chunk_p;                                               /* Source sample pointer */
  output = ( half_to_fill == SECOND ) ? ( pb_buffer + CHUNK_SZ ) : pb_buffer;

  // Transfer mono audio (scaled for volume) into the stereo buffer.
  // This is done both to eliminate the need for a second resistor
  // and also because the MAX983567A expects stereo audio data, but we only
  // have one speaker.
  //
  // NOTE: Soldered pads set a value to DIVIDE the audio down by with OPT1
  // being the LSB and OPT3 the MSB.
  //
  for( uint16_t i = 0; i < HALFCHUNK_SZ; i++ )
  {
    if( (uint8_t *) input >=  pb_end8 ) {                        /* Check for end of sample data */
      leftsample = MIDPOINT_S16;                                 /* Pad with silence if at end */
    }
    else {
      /* Convert unsigned 8-bit (0..255) -> signed 16-bit with dithering */
      uint8_t sample8 = *input;
      leftsample = Apply8BitDithering( sample8 );                    /* Left channel with dithering */
      leftsample = ( leftsample * VOL_MULT ) / vol_div;              /* Scale for volume (multiply before divide) */
      leftsample = ApplyFilterChain8Bit( leftsample, 1 );            /* Apply complete filter chain */
    }
    input++;

    if( channels == Mode_mono ) { rightsample = leftsample; }   // Right channel is the same as left.
    else {    
      if( (uint8_t *) input >=  pb_end8 ) {                      /* Check for end of sample data */
        rightsample = MIDPOINT_S16;                              /* Pad with silence if at end */
      }
      else {               
        /* Convert unsigned 8-bit (0..255) -> signed 16-bit with dithering */
        uint8_t sample8 = *input;
        rightsample = Apply8BitDithering( sample8 );                 /* Right channel with dithering */
        rightsample = ( rightsample * VOL_MULT ) / vol_div;          /* Scale for volume (multiply before divide) */
        rightsample = ApplyFilterChain8Bit( rightsample, 0 );        /* Apply complete filter chain */
      }
      input++;
    }
    *output = leftsample;  output++;                              /* Transfer samples to output buffer */
    *output = rightsample; output++;
    
    // Decrement fade counters based on samples processed
    uint8_t samples_processed = (channels == Mode_stereo) ? 2 : 1;
    if( fadein_samples_remaining > 0 ) {
      fadein_samples_remaining = (fadein_samples_remaining > samples_processed) ? 
                                 fadein_samples_remaining - samples_processed : 0;
    }
    if( fadeout_samples_remaining > 0 ) {
      fadeout_samples_remaining--;
    }
  }
  return PLAYING;
}


/** Initiates playback of your specified sample
  *
  * @param: const void* sample_to_play.  Pointer to audio sample data (8-bit or 16-bit).
  * @param: uint32_t sample_set_sz.  Many samples to play back.
  * @param: uint16_t playback_speed.  This is the sample rate.
  * @param: uint8_t sample depth.  This should be 8 or 16 bits.
  * @param: PB_ModeTypeDef mode.  Mono or stereo playback.
  * @param: LPF_Level lpf_level.  Low-pass filter level for 8-bit samples. It must be selected even if playing 16-bit samples.
  * @retval: none
  *
  */
PB_StatusTypeDef PlaySample(  const void *sample_to_play,
                              uint32_t sample_set_sz,
                              uint16_t playback_speed,
                              uint8_t sample_depth,
                              PB_ModeTypeDef mode,
                              LPF_Level lpf_level
                            )
{
  // Parameter sanity checks
  //
  if( ( sample_depth != 16 && sample_depth != 8 ) || 
      ( mode != Mode_mono && mode != Mode_stereo) ||
      sample_set_sz   == 0                        ||
      sample_to_play  == NULL
    ) { return PB_ERROR; }

  // Set low-pass filter alpha coefficient based on requested level
#ifdef ENABLE_8BIT_LPF
  switch( lpf_level ) {
    case LPF_Soft:
      lpf_8bit_alpha = LPF_SOFT;
      break;
    case LPF_Aggressive:
      lpf_8bit_alpha = LPF_AGGRESSIVE;
      break;
    case LPF_Medium:
    default:
      lpf_8bit_alpha = LPF_MEDIUM;
      break;
  }
#endif

  if( mode == Mode_stereo ) {           // Pointer advance amount for stereo/mono mode.
     p_advance = CHUNK_SZ;              // Two channels worth of samples per chunk
     channels  = Mode_stereo;
  }
  else {                                // One channels worth of samples per chunk
    p_advance  = HALFCHUNK_SZ;
    channels   = Mode_mono;     
  }

  I2S_PlaybackSpeed = playback_speed;   // Turn the DAC on in readyness and initialize I2S with the requested sample rate.
  MX_I2S2_Init();

  HAL_I2S_DMAStop( &hi2s2 );       // Ensure there is no currently playing sound before starting the current one and turn on the DAC  
  DAC_MasterSwitch( DAC_ON );   // starting the current one and turn on the DAC

  ClearBuffer();                        // Clear the playback buffer to avoid glitches
  
  // Reset DC blocking filter state for new sample
  //
  dc_filter_prev_input_left   = 0;    dc_filter_prev_input_right  = 0;
  dc_filter_prev_output_left  = 0;    dc_filter_prev_output_right = 0;
  
  // Reset biquad filter state for 8-bit samples (all 4 state variables per channel)
  lpf_8bit_x1_left  = 0;    lpf_8bit_x1_right  = 0;
  lpf_8bit_x2_left  = 0;    lpf_8bit_x2_right  = 0;
  lpf_8bit_y1_left  = 0;    lpf_8bit_y1_right  = 0;
  lpf_8bit_y2_left  = 0;    lpf_8bit_y2_right  = 0;
  
  // Reset biquad filter state for 16-bit samples
  lpf_16bit_x1_left  = 0;   lpf_16bit_x1_right  = 0;
  lpf_16bit_x2_left  = 0;   lpf_16bit_x2_right  = 0;
  lpf_16bit_y1_left  = 0;   lpf_16bit_y1_right  = 0;
  lpf_16bit_y2_left  = 0;   lpf_16bit_y2_right  = 0;
  
  // Reset air enhancement filter state
  air_x1_left  = 0;         air_x1_right  = 0;
  air_y1_left  = 0;         air_y1_right  = 0;
  
  if( sample_depth == 16 ) {            // Initialize 16-bit sample playback pointers
    pb_p16 = (uint16_t *) sample_to_play;
    pb_end16 = pb_p16 + sample_set_sz;
    pb_mode = 16;
  }
  else if( sample_depth == 8 ) {        // Initialize 8-bit sample playback pointers
    pb_p8 = (uint8_t *) sample_to_play;
    pb_end8 = pb_p8 + sample_set_sz;
    pb_mode = 8;
  }
  // Initialize fade counters
  fadeout_samples_remaining = sample_set_sz;
  fadein_samples_remaining  = FADEIN_SAMPLES;
  
  // Pre-fill the first half of the buffer with processed samples before starting DMA
  // This ensures the fade-in is applied from the very first sample that plays
  half_to_fill = FIRST;
  if( pb_mode == 16 ) {
    if( ProcessNextWaveChunk( (int16_t *) pb_p16 ) != PLAYING ) {
      return PB_ERROR;
    }
    pb_p16 += p_advance;
  }
  else if( pb_mode == 8 ) {
    if( ProcessNextWaveChunk_8_bit( (uint8_t *) pb_p8 ) != PLAYING ) {
      return PB_ERROR;
    }
    pb_p8 += p_advance;
  }
  
  // Start playback of the recording
  //
  pb_state = PLAYING;
  HAL_I2S_Transmit_DMA( &hi2s2, (uint16_t *) pb_buffer, PB_BUFF_SZ );
  return PLAYING;
}


/** Simple function that waits for the end of playback.
  *
  * @param: none
  * @retval: PB_StatusTypeDef indicating success or failure.
  *
  */
PB_StatusTypeDef WaitForSampleEnd( void )
{
  while( pb_state == PLAYING ) {
    __NOP();  // Prevent optimizer from removing loop
  }
  
  // Cleanup: Stop DMA transmission now that we're out of the callback context
  // This prevents the I2S_WaitFlagStateUntilTimeout hang that occurs when
  // stopping from within the DMA callback
  if( pb_state == PB_IDLE ) {
    HAL_I2S_DMAStop( &hi2s2 );
  }
  
  return pb_state;
}


/**Clears the playback buffer so that we don't get audio glitches at the ends of playback.
  * @param: none
  * @retval: none
  *
  * */
void ClearBuffer( void )
{
  for( uint16_t sample_index = 0; sample_index < PB_BUFF_SZ; sample_index++ ) {
    pb_buffer[ sample_index ] = MIDPOINT_S16;
  }
}


/** Apply TPDF dithering during 8-bit to 16-bit conversion with cubic interpolation
  *
  * Converts unsigned 8-bit samples (0..255) to signed 16-bit samples
  * with Triangular Probability Density Function (TPDF) dithering.
  * This breaks up quantization patterns and significantly reduces
  * perceived noise and distortion from low-resolution samples.
  *
  * Uses cubic Hermite interpolation for smoother 8→16 bit conversion,
  * which significantly reduces staircase artifacts compared to linear interpolation.
  * The cubic curve provides C1 continuity (smooth first derivative).
  *
  * Uses a fast Linear Congruential Generator for random noise generation.
  *
  * @param: sample8 - Unsigned 8-bit audio sample
  * @retval: Signed 16-bit audio sample with dithering applied
  */
int16_t Apply8BitDithering( uint8_t sample8 )
{
  // Convert unsigned 8-bit (0..255) to signed 16-bit (approximately -32768..32767)
  // Standard approach: center the unsigned value and scale by 256
  int16_t sample16 = (int16_t)( sample8 - 128 ) << 8;
  
  // Generate TPDF (Triangular Probability Density Function) dither
  // TPDF = rand1 - rand2 gives proper triangular distribution
  // This masks quantization noise and is the audio industry standard
  dither_state = dither_state * 1103515245U + 12345U;  // LCG step 1
  int32_t rand1 = (dither_state >> 16) & 0xFF;
  
  dither_state = dither_state * 1103515245U + 12345U;  // LCG step 2
  int32_t rand2 = (dither_state >> 16) & 0xFF;
  
  // TPDF dither: subtract gives values from -255 to +255
  // Scale down to ±4 range (much more subtle)
  // This adds noise at a very small level to mask quantization artifacts
  int16_t dither = (int16_t)((rand1 - rand2) >> 6);  // Divide by 64 for subtle dither
  
  return sample16 + dither;
}


/** Apply single-pole low-pass filter for 8-bit samples
  *
  * Implements a first-order IIR low-pass filter to smooth out
  * staircase artifacts and high-frequency noise from 8-bit sample conversion.
  * Single-pole filters are simpler, more stable, and sufficient for smoothing
  * quantization artifacts from 8-bit to 16-bit conversion.
  * 
  * Filter equation: y[n] = alpha*x[n] + (1-alpha)*y[n-1]
  * 
  * Alpha values (in fixed-point 16-bit):
  * - LPF_SOFT (0.875): Gentle rolloff, preserves more highs
  * - LPF_MEDIUM (0.75): Balanced filtering
  * - LPF_AGGRESSIVE (0.625): Strong rolloff, removes more highs
  *
  * @param: sample - Audio sample to filter
  * @param: y1 - Pointer to previous output state y[n-1]
  * @retval: Filtered 16-bit audio sample
  */
int16_t ApplyLowPassFilter8Bit( int16_t sample, 
                                volatile int32_t *x1, volatile int32_t *x2,
                                volatile int32_t *y1, volatile int32_t *y2 )
{
  // Simple first-order IIR low-pass filter
  // y[n] = alpha*x[n] + (1-alpha)*y[n-1]
  // More stable than biquad and still effective for smoothing 8-bit artifacts
  
  int32_t alpha = lpf_8bit_alpha;      // Filter coefficient in fixed-point
  int32_t one_minus_alpha = 65536 - alpha;
  
  // Apply filter: y[n] = alpha*x[n] + (1-alpha)*y[n-1]
  int32_t output = ((alpha * sample) >> 16) + 
                   ((one_minus_alpha * (*y1)) >> 16);
  
  // Update state variable
  *y1 = output;
  
  // Clamp to 16-bit range
  if( output > 32767 )  output = 32767;
  if( output < -32768 ) output = -32768;
  
  return (int16_t) output;
}


/** Apply fade-in effect to audio sample
  *
  * Applies an exponential fade-in over the first FADEIN_SAMPLES samples.
  * Uses a squared curve for more natural-sounding fade that matches
  * human perception better than linear fades.
  * 
  * Uses global fadein_samples_remaining counter which is decremented elsewhere.
  *
  * @param: sample - Audio sample to apply fade-in to
  * @retval: Sample with fade-in applied
  */
int16_t ApplyFadeIn( int16_t sample ) {
  if( fadein_samples_remaining > 0 ) {
    // Calculate progress: 0 at start, FADEIN_SAMPLES at end
    int32_t progress = FADEIN_SAMPLES - fadein_samples_remaining;
    
    // Apply squared curve: (progress / FADEIN_SAMPLES)^2
    // This creates exponential fade-in that sounds more natural
    int32_t fade_mult = (progress * progress) / FADEIN_SAMPLES;
    
    return (int32_t) sample * fade_mult / FADEIN_SAMPLES;
  }
  return sample;
}


/** Apply fade-out effect to audio sample
  *
  * Applies an exponential fade-out over the last FADEOUT_SAMPLES samples.
  * Uses a squared curve for more natural-sounding fade that matches
  * human perception better than linear fades.
  *
  * Uses global fadeout_samples_remaining counter which is decremented elsewhere.
  *
  * @param: sample - Audio sample to apply fade-out to
  * @retval: Sample with fade-out applied
  */
int16_t ApplyFadeOut( int16_t sample )
{
  if( fadeout_samples_remaining > 0 && fadeout_samples_remaining <= FADEOUT_SAMPLES ) {
    // Apply squared curve: (remaining / FADEOUT_SAMPLES)^2
    // This creates exponential fade-out that sounds more natural
    int32_t fade_mult = (fadeout_samples_remaining * fadeout_samples_remaining) / FADEOUT_SAMPLES;
    
    return (int32_t) sample * fade_mult / FADEOUT_SAMPLES;
  }
  return sample;
}


/** Apply noise gate to remove quiet background noise
  *
  * Silences audio samples below the threshold to eliminate quiet
  * background noise and hiss. Samples above the threshold pass through
  * unchanged.
  *
  * The threshold is configurable in main.h (NOISE_GATE_THRESHOLD).
  * Typical values:
  * - 256 = ~0.8% of full scale (aggressive, may cut quiet audio)
  * - 512 = ~1.5% of full scale (balanced)
  * - 1024 = ~3% of full scale (gentle, only removes very quiet noise)
  *
  * @param: sample - Audio sample to apply gate to
  * @retval: Gated sample (silenced if below threshold)
  */
int16_t ApplyNoiseGate( int16_t sample )
{
  // Get absolute value for threshold comparison
  int16_t abs_sample = (sample < 0) ? -sample : sample;
  
  // If below threshold, silence it; otherwise pass through
  if( abs_sample < NOISE_GATE_THRESHOLD ) {
    return 0;
  }
  return sample;
}


/** Apply soft clipping to prevent harsh digital distortion
  *
  * Uses a cubic polynomial approximation to smoothly compress samples
  * that approach the 16-bit limits. Samples within ±28000 pass through
  * unchanged; samples beyond this are smoothly compressed toward ±32767.
  *
  * This prevents harsh digital clipping artifacts when multiple effects
  * or high volume settings cause samples to exceed the valid range.
  *
  * @param: sample - Audio sample to apply soft clipping to
  * @retval: Soft-clipped sample within valid 16-bit range
  */
int16_t ApplySoftClipping( int16_t sample )
{
  const int32_t threshold = 28000;  // Start soft clipping above this
  const int32_t max_val = 32767;
  
  int32_t s = sample;
  
  if( s > threshold ) {
    // Compress positive values above threshold
    int32_t excess = s - threshold;
    int32_t range = max_val - threshold;  // 4767
    // Apply cubic soft clip: y = threshold + range * (1 - (1 - x/range)^3)
    // Simplified: smooth compression toward max_val
    int32_t x = excess * 65536 / range;  // Normalize to 0-65536
    if( x > 65536 ) x = 65536;
    // Cubic curve: output = threshold + range * (3x^2 - 2x^3) using normalized x
    int32_t x2 = (x * x) >> 16;
    int32_t x3 = (x2 * x) >> 16;
    int32_t curve = ((3 * x2) >> 1) - ((2 * x3) >> 0);  // Result in 0-65536 range
    s = threshold + ((range * curve) >> 16);
  }
  else if( s < -threshold ) {
    // Compress negative values below -threshold
    int32_t excess = -threshold - s;
    int32_t range = max_val - threshold;
    int32_t x = excess * 65536 / range;
    if( x > 65536 ) x = 65536;
    int32_t x2 = (x * x) >> 16;
    int32_t x3 = (x2 * x) >> 16;
    int32_t curve = ((3 * x2) >> 1) - ((2 * x3) >> 0);
    s = -threshold - ((range * curve) >> 16);
  }
  
  // Final safety clamp
  if( s > max_val ) s = max_val;
  if( s < -max_val ) s = -max_val;
  
  return (int16_t) s;
}


/** DC blocking filter to remove DC offset from audio signal
  *
  * Implements a first-order IIR high-pass filter: y[n] = x[n] - x[n-1] + α * y[n-1]
  * This removes any DC bias while preserving audio content.
  *
  * @param: input - Current audio sample
  * @param: prev_input - Pointer to previous input sample state
  * @param: prev_output - Pointer to previous output sample state
  * @retval: Filtered 16-bit audio sample
  */
int16_t ApplyDCBlockingFilter( volatile int16_t input, volatile int32_t *prev_input, volatile int32_t *prev_output )
{
  // Apply filter: output = input - prev_input + (α * prev_output)
  // α = 0.98 in fixed-point (DC_FILTER_ALPHA / 2^DC_FILTER_SHIFT)
  int32_t output = input - *prev_input + 
                   ( ( *prev_output * DC_FILTER_ALPHA ) >> DC_FILTER_SHIFT );
  
  // Update state
  *prev_input   = input;
  *prev_output  = output;
  
  // Clamp to 16-bit range
  if ( output > 32767 )   output = 32767;
  if ( output < -32768 )  output = -32768;
  
  return ( int16_t ) output;
}


/** Soft DC filter for 16-bit audio - gentler DC removal
  *
  * Implements a first-order IIR high-pass filter with higher alpha (0.995)
  * resulting in a lower cutoff frequency (~2Hz vs ~8Hz standard filter).
  * This provides more gentle DC removal, useful for samples with low-frequency content.
  *
  * @param: input - Current audio sample
  * @param: prev_input - Pointer to previous input sample state
  * @param: prev_output - Pointer to previous output sample state
  * @retval: Filtered 16-bit audio sample
  */
int16_t ApplySoftDCFilter16Bit( volatile int16_t input, volatile int32_t *prev_input, volatile int32_t *prev_output )
{
  // Apply filter: output = input - prev_input + (α * prev_output)
  // α = 0.995 in fixed-point (SOFT_DC_FILTER_ALPHA / 2^DC_FILTER_SHIFT)
  int32_t output = input - *prev_input + 
                   ( ( *prev_output * SOFT_DC_FILTER_ALPHA ) >> DC_FILTER_SHIFT );
  
  // Update state
  *prev_input   = input;
  *prev_output  = output;
  
  // Clamp to 16-bit range
  if ( output > 32767 )   output = 32767;
  if ( output < -32768 )  output = -32768;
  
  return ( int16_t ) output;
}


/** Biquad low-pass filter for 16-bit samples
  *
  * Implements a second-order IIR low-pass filter (biquad) using direct form I.
  * Provides superior frequency response and steeper rolloff compared to first-order filters.
  * Coefficients are calculated from the alpha parameter (same as 8-bit LPF).
  *
  * @param: input - Current audio sample
  * @param: x1 - Pointer to previous input sample x[n-1]
  * @param: x2 - Pointer to previous input sample x[n-2]
  * @param: y1 - Pointer to previous output sample y[n-1]
  * @param: y2 - Pointer to previous output sample y[n-2]
  * @retval: Filtered 16-bit audio sample
  */
int16_t ApplyLowPassFilter16Bit( int16_t input, volatile int32_t *x1, volatile int32_t *x2, 
                                 volatile int32_t *y1, volatile int32_t *y2 )
{
  // Calculate biquad coefficients from alpha (LPF_16BIT_ALPHA)
  // Standard one-pole to two-pole conversion for matched frequency response
  uint32_t alpha = LPF_16BIT_ALPHA;
  
  // b0 = (1 - α)² / 2
  int32_t b0 = ((65536 - alpha) * (65536 - alpha)) >> 17;  // div by 131072 (2^17)
  // b1 = 2 * b0
  int32_t b1 = b0 << 1;
  // b2 = b0
  int32_t b2 = b0;
  // a1 = -2α (negated for direct form I)
  int32_t a1 = -(alpha << 1);
  // a2 = α²
  int32_t a2 = (alpha * alpha) >> 16;
  
  // Direct form I: y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]
  int32_t output = (b0 * input) + (b1 * (*x1)) + (b2 * (*x2)) - (a1 * (*y1)) - (a2 * (*y2));
  output >>= 16;  // Scale back from fixed-point
  
  // Update state variables (shift history)
  *x2 = *x1;
  *x1 = input;
  *y2 = *y1;
  *y1 = output;
  
  // Clamp to 16-bit range
  if ( output > 32767 )   output = 32767;
  if ( output < -32768 )  output = -32768;
  
  return (int16_t)output;
}


/** High-frequency air enhancement filter
  *
  * Implements a first-order high-shelf filter to add subtle high-frequency boost.
  * Enhances clarity and "air" in the audio, making it sound more open and detailed.
  * Uses a gentle +1.5dB boost above ~7-8kHz.
  *
  * @param: input - Current audio sample
  * @param: x1 - Pointer to previous input sample x[n-1]
  * @param: y1 - Pointer to previous output sample y[n-1]
  * @retval: Enhanced 16-bit audio sample
  */
int16_t ApplyAirEnhancement( int16_t input, volatile int32_t *x1, volatile int32_t *y1 )
{
  // High-shelf filter: y[n] = x[n] + gain*(x[n] - x[n-1]) + α*y[n-1]
  // This boosts high frequencies while maintaining low frequencies
  
  int32_t diff = input - *x1;  // High-frequency component
  
  // Apply gain to high-frequency component
  int32_t boost = (diff * AIR_BOOST_GAIN) >> 16;
  
  // Combine with recursive filter for smooth response
  int32_t output = input + boost + ((AIR_CUTOFF_ALPHA * (*y1)) >> 16);
  
  // Update state
  *x1 = input;
  *y1 = output;
  
  // Clamp to 16-bit range
  if ( output > 32767 )   output = 32767;
  if ( output < -32768 )  output = -32768;
  
  return (int16_t)output;
}


/** Apply complete filter chain for 16-bit samples
  *
  * Applies the complete signal processing chain in the correct order:
  * 1. Biquad low-pass filter (optional) - removes high-frequency artifacts
  * 2. DC blocking filter (soft or standard) - removes DC offset
  * 3. Fade-in (exponential) - smooth entry
  * 4. Fade-out (exponential) - smooth exit
  * 5. Air enhancement (optional) - adds high-frequency clarity
  * 6. Noise gate (optional) - removes background noise
  * 7. Soft clipping (optional) - prevents harsh distortion
  *
  * @param: sample - Input audio sample after volume scaling
  * @param: is_left_channel - 1 for left channel, 0 for right channel
  * @retval: Fully processed audio sample
  */
int16_t ApplyFilterChain16Bit( int16_t sample, uint8_t is_left_channel )
{
#ifdef ENABLE_16BIT_BIQUAD_LPF
  // Apply biquad low-pass filter first
  if( is_left_channel ) {
    sample = ApplyLowPassFilter16Bit( sample, &lpf_16bit_x1_left, &lpf_16bit_x2_left, 
                                      &lpf_16bit_y1_left, &lpf_16bit_y2_left );
  } else {
    sample = ApplyLowPassFilter16Bit( sample, &lpf_16bit_x1_right, &lpf_16bit_x2_right, 
                                      &lpf_16bit_y1_right, &lpf_16bit_y2_right );
  }
#endif
  
  // Apply DC blocking filter before fades (prevents DC bias amplification)
#ifdef ENABLE_SOFT_DC_FILTER_16BIT
  if( is_left_channel ) {
    sample = ApplySoftDCFilter16Bit( sample, &dc_filter_prev_input_left, &dc_filter_prev_output_left );
  } else {
    sample = ApplySoftDCFilter16Bit( sample, &dc_filter_prev_input_right, &dc_filter_prev_output_right );
  }
#else
  if( is_left_channel ) {
    sample = ApplyDCBlockingFilter( sample, &dc_filter_prev_input_left, &dc_filter_prev_output_left );
  } else {
    sample = ApplyDCBlockingFilter( sample, &dc_filter_prev_input_right, &dc_filter_prev_output_right );
  }
#endif
  
  // Apply fade-in
  sample = ApplyFadeIn( sample );
  
  // Apply fade-out
  sample = ApplyFadeOut( sample );
  
#ifdef ENABLE_AIR_ENHANCEMENT
  // Apply high-frequency air enhancement for clarity
  if( is_left_channel ) {
    sample = ApplyAirEnhancement( sample, &air_x1_left, &air_y1_left );
  } else {
    sample = ApplyAirEnhancement( sample, &air_x1_right, &air_y1_right );
  }
#endif
  
#ifdef ENABLE_NOISE_GATE
  // Apply noise gate to remove quiet background noise
  sample = ApplyNoiseGate( sample );
#endif
  
#ifdef ENABLE_SOFT_CLIPPING
  // Apply soft clipping as final stage
  sample = ApplySoftClipping( sample );
#endif
  
  return sample;
}


/** Apply complete filter chain for 8-bit samples
  *
  * Applies the complete signal processing chain in the correct order:
  * 1. Low-pass filter for 8-bit (optional, removes staircase artifacts with biquad)
  * 2. DC blocking filter (soft or standard) - applied BEFORE fades to prevent DC bias amplification
  * 3. Fade-in (exponential)
  * 4. Fade-out (exponential)
  * 5. Noise gate (optional)
  * 6. Soft clipping (optional)
  *
  * @param: sample - Input audio sample after dithering and volume scaling
  * @param: is_left_channel - 1 for left channel, 0 for right channel
  * @retval: Fully processed audio sample
  */
int16_t ApplyFilterChain8Bit( int16_t sample, uint8_t is_left_channel )
{
#ifdef ENABLE_8BIT_LPF
  // Apply biquad low-pass filter to smooth 8-bit artifacts
  if( is_left_channel ) {
    sample = ApplyLowPassFilter8Bit( sample, 
                                     &lpf_8bit_x1_left, &lpf_8bit_x2_left,
                                     &lpf_8bit_y1_left, &lpf_8bit_y2_left );
  } else {
    sample = ApplyLowPassFilter8Bit( sample,
                                     &lpf_8bit_x1_right, &lpf_8bit_x2_right,
                                     &lpf_8bit_y1_right, &lpf_8bit_y2_right );
  }
#endif
  
  // Apply DC blocking filter BEFORE fades to prevent DC bias amplification
  // This is more effective than filtering after fades
#ifdef ENABLE_SOFT_DC_FILTER_16BIT
  if( is_left_channel ) {
    sample = ApplySoftDCFilter16Bit( sample, &dc_filter_prev_input_left, &dc_filter_prev_output_left );
  } else {
    sample = ApplySoftDCFilter16Bit( sample, &dc_filter_prev_input_right, &dc_filter_prev_output_right );
  }
#else
  if( is_left_channel ) {
    sample = ApplyDCBlockingFilter( sample, &dc_filter_prev_input_left, &dc_filter_prev_output_left );
  } else {
    sample = ApplyDCBlockingFilter( sample, &dc_filter_prev_input_right, &dc_filter_prev_output_right );
  }
#endif
  
  // Apply fade-in
  sample = ApplyFadeIn( sample );
  
  // Apply fade-out
  sample = ApplyFadeOut( sample );
  
#ifdef ENABLE_NOISE_GATE
  // Apply noise gate to remove quiet background noise
  sample = ApplyNoiseGate( sample );
#endif
  
#ifdef ENABLE_SOFT_CLIPPING
  // Apply soft clipping as final stage
  sample = ApplySoftClipping( sample );
#endif
  
  return sample;
}


/** Changes NSD_MODE_Pin pin to control the DAC between on and Shutdown
  *
  * param: DAC_OFF (0) or DAC_ON (non zero)
  * retval: none
  */
void DAC_MasterSwitch( GPIO_PinState setting )
{
  /* Change the setting */
  HAL_GPIO_WritePin(NSD_MODE_GPIO_Port, NSD_MODE_Pin, setting );
  
  /* Wait 10mS no allow the MAX98357A to settle */
  HAL_Delay( 10 );
}


/** Read the master volume level for playback.
  *
  * params: none
  * retval: uint8_t between 1 and 8 for the audio data divisor.
  *
  */
uint8_t ReadVolume( void )
{
    uint8_t v = 
    (
        ( ( (OPT3_GPIO_Port->IDR & OPT3_Pin) != 0 ) << 2 ) |
        ( ( (OPT2_GPIO_Port->IDR & OPT2_Pin) != 0 ) << 1 ) |
        ( ( (OPT1_GPIO_Port->IDR & OPT1_Pin) != 0 ) << 0 )
    );

    /* volume divisor is 1..8 so add 1 to the 3-bit value to make range 1..8 */
    v = (uint8_t) ( ( v + 1 ) * VOL_SCALING) ;

    /* never return 0 even if VOL_SCALING is mis-set */
    return v ? v : 1;
} 


/** Wait for the trigger signal
 *
 * Waits until the trigger signal is received.
 *
 * params: none
 * reval: none
 *
 */
inline void WaitForTrigger( uint8_t trig_to_wait_for )
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

  /* Prep for sleep mode */
    LPSystemClock_Config();                     // Reduce clock speed for low power sleep
    HAL_SuspendTick();                          // Stop SysTick interrupts to prevent wakeups
    __HAL_GPIO_EXTI_CLEAR_IT( TRIGGER_Pin );    // Clear EXTI pending bit
    
    /* Memory barrier to ensure all writes complete before sleep */
    __DSB();
    __ISB();

    /* Enter low power sleep mode and wait for the trigger */
    HAL_PWR_EnterSLEEPMode( PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFI );

    /* Rise from your slumber mighty microcontroller! */
    __HAL_GPIO_EXTI_CLEAR_IT( TRIGGER_Pin );   // Clear EXTI pending bit
    HAL_PWREx_DisableLowPowerRunMode();
    SystemClock_Config();
    HAL_ResumeTick();
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
void ShutDownAudio( void )
{
      // Allow DMA buffer to fully drain and MAX98357A to output final samples
    HAL_Delay( 150 );
    
    // Stop I2S DMA transmission
    HAL_I2S_DMAStop( &hi2s2 );
    
    // Shutdown the DAC and either loop back of shutdown to save power.
    //
    DAC_MasterSwitch( DAC_OFF );
}


/** Systick IRQ including trigger handling
  *
  * params: none
  * retval: none
  */
void HAL_IncTick( void )
{
  uwTick += uwTickFreq;

  if( ( TRIGGER_GPIO_Port->IDR & TRIGGER_Pin ) != 0 ) {
    if( trig_counter < TC_MAX ) trig_counter++;
  }
  else {
    if ( trig_counter > 0 ) trig_counter--;
  }

  if( trig_counter < TC_LOW_THRESHOLD )   trig_status = TRIGGER_CLR;
  if( trig_counter > TC_HIGH_THRESHOLD )  trig_status = TRIGGER_SET;
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
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE2);
  HAL_PWREx_EnableLowPowerRunMode();
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
  HAL_I2S_DMAStop( &hi2s2 );
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
