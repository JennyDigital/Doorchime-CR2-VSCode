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
#include <stdint.h>
#include "lock.h"

//#include "newchallenger.h"
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
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

// Playback engine buffer defines
//
#define PB_BUFF_SZ        512U
#define CHUNK_SZ          ( PB_BUFF_SZ / 2 )
#define HALFCHUNK_SZ      ( CHUNK_SZ / 2 )
#define FIRST             0
#define SECOND            1U

// Bitfields for options
//
#define OPT_AUTO_TRIG         0b1000
#define OPT_VOLUME            0b0111


// DAC Switch defines
//
#define DAC_ON            1
#define DAC_OFF           0

// Stereo file playback option
//#define STEREO_INPUT_MODE

// Trigger Option macros
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
//
//#define TEST_CYCLING
//#define FORCE_TRIGGER_OPT
//#define CUSTOMER_BUILD


// Volume special config
//
#define VOL_SCALING           1       // This is the master volume divider multiplication factor
#define VOL_MULT              1       // Sets the multiplication back so we can use integers.

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2S_HandleTypeDef hi2s2;
DMA_HandleTypeDef hdma_spi2_tx;

/* USER CODE BEGIN PV */

          int16_t     pb_buffer[ PB_BUFF_SZ ] = {0};

volatile  uint8_t  *  pb_p8;
volatile  uint8_t  *  pb_end8;
volatile  uint16_t *  pb_p16;
volatile  uint16_t *  pb_end16;
          uint8_t     pb_mode;
volatile  uint8_t     playing                 = 0;
volatile  uint8_t     half_to_fill;
volatile  uint8_t     vol_div;
          uint8_t     no_channels             = 1;
volatile  uint16_t    trig_counter            = 0;
volatile  uint8_t     trig_status             = TRIGGER_CLR;
          uint16_t    I2S_PlaybackSpeed       = I2S_AUDIOFREQ_22K;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config                 ( void );
static  void    MX_GPIO_Init            ( void );
static  void    MX_DMA_Init             ( void );
static  void    MX_I2S2_Init            ( void );
/* USER CODE BEGIN PFP */

        void    CopyNextWaveChunk       ( int16_t * chunk_p );
        void    CopyNextWaveChunk_8_bit ( uint8_t * chunk_p );
        void    DAC_MasterSwitch        ( GPIO_PinState setting );
        uint8_t ReadVolume              ( void );
        void    WaitForTrigger          ( uint8_t trig_to_wait_for );
        uint8_t GetTriggerOption        ( void );
        void    PlaySample              ( uint16_t * sample_to_play, uint32_t sample_set_sz,
                                          uint16_t playback_speed,  uint8_t sample_depth
                                        );
        void    WaitForSampleEnd        ( void );
        void    ClearBuffer             ( void );

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
  #ifdef CUSTOMER_BUILD
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
  MX_I2S2_Init();
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
    if( GetTriggerOption() == AUTO_TRIG_ENABLED )
    {
      WaitForTrigger( TRIGGER_SET );
    }
#endif

    // Setup for playback
    //
    vol_div = ReadVolume();

    //PlaySample( (uint16_t *) harmony8b, HARMONY8B_SZ, I2S_AUDIOFREQ_11K, 8 );
  
    //PlaySample( (uint16_t *) quencho_flute11k, QUENCHO_FLUTE11K_SZ, I2S_AUDIOFREQ_11K, 16 );
    PlaySample( (uint16_t *) KillBill11k, KILLBILL11K_SZ, I2S_AUDIOFREQ_11K, 16 );
    WaitForSampleEnd();

    // Shutdown the DAC and either loop back of shutdown to save power.
    //
    DAC_MasterSwitch( DAC_OFF );

    if( GetTriggerOption() == AUTO_TRIG_DISABLED )
    {
      HAL_SuspendTick();
      HAL_PWR_EnterSLEEPMode( PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFE );
    }
    else
    {
#ifndef TEST_CYCLING
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
  * @brief System Clock Configuration
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 12;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
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

  /*Configure GPIO pins : TRIGGER_Pin OPT4_Pin OPT3_Pin OPT2_Pin
                           OPT1_Pin */
  GPIO_InitStruct.Pin   = TRIGGER_Pin | OPT4_Pin | OPT3_Pin | OPT2_Pin
                        | OPT1_Pin;
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

  if( pb_mode == 16 )
  {
    pb_p16 += HALFCHUNK_SZ;

    if( pb_p16 >= pb_end16 )
    {
      playing = 0;
      HAL_I2S_DMAStop( &hi2s2 );
    }
    else
    {
      half_to_fill = FIRST;
      CopyNextWaveChunk( (int16_t *) pb_p16 );
    }
  }
  else if( pb_mode == 8 )
  {
    pb_p8 += HALFCHUNK_SZ;

    if( pb_p8 >= pb_end8 )
    {
      playing = 0;
      HAL_I2S_DMAStop( &hi2s2 );
    }
    else
    {
      half_to_fill = FIRST;
      CopyNextWaveChunk_8_bit( (uint8_t *) pb_p8 );
    }
  }
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

  if( pb_mode == 16 )
  {
    pb_p16 += HALFCHUNK_SZ;

    if( pb_p16 >= pb_end16 )
    {
      playing = 0;
      HAL_I2S_DMAStop( &hi2s2 );
    }
    else
    {
      half_to_fill = SECOND;
      CopyNextWaveChunk( (int16_t *) pb_p16 );
    }
  }
  else if( pb_mode == 8 )
  {
    pb_p8 += HALFCHUNK_SZ;

    if( pb_p8 >= pb_end8 )
    {
      playing = 0;
      HAL_I2S_DMAStop( &hi2s2 );
    }
    else
    {
      half_to_fill = SECOND;
      CopyNextWaveChunk_8_bit( (uint8_t *) pb_p8 );
    }
  }
}


/** Transfers a chunk of 16-bit samples to the DMA playback buffer
  *
  * It also processes for stereo mode
  *
  * @param: uint8_t* chunk_p.  The start of the chunk to transfer.
  * @retval: none.
  *
  */
void CopyNextWaveChunk( int16_t * chunk_p )
{
  int16_t *input, *output;

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
    *output = ( (int16_t) (*input) / vol_div ) * VOL_MULT; // Left channel
    output++;
#ifndef STEREO_INPUT_MODE
    *output = ( (int16_t) (*input) / vol_div ) * VOL_MULT; // Right channel
    output++;
#endif
        
    input++;
  }
}


/** Transfers a chunk of 8-bit samples to the DMA playback buffer
  *
  * It also processes for stereo mode
  *
  * @param: uint8_t* chunk_p.  The start of the chunk to transfer.
  * @retval: none.
  *
  */
void CopyNextWaveChunk_8_bit( uint8_t * chunk_p )
{
  uint8_t *input;
  int16_t *output;

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
    /* Convert unsigned 8-bit (0..255) -> signed 16-bit centered around 0 */
    {
      int16_t tmp = ( (int16_t)(*input) - 128 ) << 8;
      tmp = tmp / vol_div * VOL_MULT; /* Left channel */
      *output = tmp;
    }
    output++;
#ifndef STEREO_INPUT_MODE
    {
      int16_t tmp2 = ( (int16_t)(*input) - 128 ) << 8;
      tmp2 = tmp2 / vol_div * VOL_MULT; /* Right channel */
      *output = tmp2;
    }
    output++;
#endif
        
    input++;
  }
}


/** Initiates playback of your specified sample
  *
  * @param: uint16_t* sample_to_play.  It doesn't matter if your sample is 8-bit, it will still work.
  * @param: uint32_t sample_set_sz.  Many samples to play back.
  * @param: uint16_t playback_speed.  This is the sample rate.
  * @param: uint8_t sample depth.  This should b4e 8 or 16 bits.
  * @retval: none
  *
  */
void PlaySample( uint16_t *sample_to_play, uint32_t sample_set_sz, uint16_t playback_speed, uint8_t sample_depth )
{
  // Ignore invalid depths.  This could expand to 32-bit samples later
  // but it's unlikely to be of real use other than convenience.
  //
  if( sample_depth != 16 && sample_depth != 8 )
  {
     return;
  }

  // Ensure there is no currently playing sound before
  // starting the current one.
  //
  HAL_I2S_DMAStop(  &hi2s2 );

  // Turn the DAC on in readyness.
  //
  I2S_PlaybackSpeed = playback_speed;
  MX_I2S2_Init();
  DAC_MasterSwitch( DAC_ON );
  ClearBuffer();
  if( sample_depth == 16 )
  {
    pb_p16 = (uint16_t *) sample_to_play;
    pb_end16 = pb_p16 + sample_set_sz;
    pb_mode = 16;
  }
  else if( sample_depth == 8 )
  {
    pb_p8 = (uint8_t *) sample_to_play;
    pb_end8 = pb_p8 + sample_set_sz;
    pb_mode = 8;
  }
  
  // Start Playing the recording
  //
  playing = 1;
  HAL_I2S_Transmit_DMA( &hi2s2, (uint16_t *) pb_buffer, PB_BUFF_SZ );
}


/** Simple function that waits for the end of playback.
  *
  * @param: none
  * @retval: none
  *
  */
void WaitForSampleEnd( void )
{
  while( playing );
}


/**Clears the playback buffer so that we don't get audio glitches at the ends of playback.
  * @param: none
  * @retval: none
  *
  * */
void ClearBuffer( void )
{
  for( uint16_t sample_index = 0; sample_index < PB_BUFF_SZ; sample_index++ )
  {
    pb_buffer[ sample_index ] = 0;
  }
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


/** Read the master volumr level for playback.
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
    v = (uint8_t)((v + 1) * VOL_SCALING);

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
  while ( trig_status != trig_to_wait_for );
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


void HAL_IncTick( void )
{
  uwTick += uwTickFreq;

  if( ( TRIGGER_GPIO_Port->IDR & TRIGGER_Pin ) != 0 )
  {
    if( trig_counter < TC_MAX ) trig_counter++;
  }
  else
  {
    if (trig_counter > 0 ) trig_counter--;
  }

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
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
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
