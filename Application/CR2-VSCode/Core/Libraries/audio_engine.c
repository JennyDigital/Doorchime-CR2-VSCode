/**
  ******************************************************************************
  * @file           : audio_engine.c
  * @brief          : Audio playback engine and DSP filter chain implementation
  ******************************************************************************
  *
  * MIT License
  *
  * Copyright (c) 2024-2026 Jennifer A. Gunn (with technological assistance).
  *
  * Permission is hereby granted, free of charge, to any person obtaining a copy
  * of this software and associated documentation files (the "Software"), to deal
  * in the Software without restriction, including without limitation the rights
  * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  * copies of the Software, and to permit persons to whom the Software is
  * furnished to do so, subject to the following conditions:
  *
  * The above copyright notice and this permission notice shall be included in all
  * copies or substantial portions of the Software.
  *
  * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  * SOFTWARE.
  *
  * @attention
  *
  * Reusable audio playback engine with runtime-configurable DSP filters.
  * Supports 8-bit and 16-bit samples, mono/stereo playback, and various
  * audio processing effects (LPF, DC blocking, fade in/out, soft clipping).
  *
  ******************************************************************************
  */

#include "audio_engine.h"
#include <math.h>           // Some of our filter calculations need math functions
#include <stddef.h>
#include <stdint.h>         // We like things predictable in these here ports.
#include <string.h>         // Needed for memset

/* Forward declarations for internal helper functions */
/* Forward declarations for internal helper functions */

// Fade and volume helpers
static inline   void      UpdateFadeCounters          ( uint32_t samples_processed );
static          int16_t   ApplyFadeIn                 ( int16_t sample );
static          int16_t   ApplyFadeOut                ( int16_t sample );
static inline   int16_t   ApplyVolumeSetting          ( int16_t sample, uint8_t volume_setting );

// Noise reduction and gating
static          int16_t   ApplyNoiseGate              ( int16_t sample );

// Clipping
static          int16_t   ApplySoftClipping           ( int16_t sample );
static inline   int32_t   ComputeSoftClipCurve        ( int32_t excess, int32_t range );

// DC blocking filters
static          int16_t   ApplyDCBlockingFilter       (
                                                        volatile int16_t input,
                                                        volatile int32_t *prev_input,
                                                        volatile int32_t *prev_output
                                                      );
static          int16_t   ApplySoftDCFilter16Bit      (
                                                        volatile int16_t input,
                                                        volatile int32_t *prev_input,
                                                        volatile int32_t *prev_output
                                                      );
static          int16_t   ApplySoftDCFilter16Bit      (
                                                        volatile int16_t input,
                                                        volatile int32_t *prev_input,
                                                        volatile int32_t *prev_output
                                                      );

// Filtering
static          int16_t   Apply8BitDithering          ( uint8_t sample8 );
static inline   uint16_t  GetLpf8BitAlpha             ( LPF_Level lpf_level );
static          int16_t   ApplyLowPassFilter16Bit     ( 
                                                        int16_t input,
                                                        volatile int32_t *x1,
                                                        volatile int32_t *x2,
                                                        volatile int32_t *y1,
                                                        volatile int32_t *y2
                                                      );
static          int16_t   ApplyLowPassFilter8Bit      (
                                                        int16_t sample,
                                                        volatile int32_t *y1
                                                      );
static          int16_t   ApplyFilterChain16Bit       ( int16_t sample, uint8_t is_left_channel );
static          int16_t   ApplyFilterChain8Bit        ( int16_t sample, uint8_t is_left_channel );

/* External variables that need to be defined by the application */
extern I2S_HandleTypeDef AUDIO_ENGINE_I2S_HANDLE;

/* Hardware interface function pointers (set by application) */
DAC_SwitchFunc  AudioEngine_DACSwitch   = NULL;
ReadVolumeFunc  AudioEngine_ReadVolume  = NULL;
I2S_InitFunc    AudioEngine_I2SInit     = NULL;

/* Volume divisor (populated by hardware GPIO reading) */
volatile uint8_t vol_input;

/* Playback buffer */
int16_t pb_buffer[PB_BUFF_SZ] = {0};

/* Filter configuration (runtime-tunable) */
volatile FilterConfig_TypeDef filter_cfg = {
  .enable_16bit_biquad_lpf      = 1,
  .enable_soft_dc_filter_16bit  = 1,
  .enable_8bit_lpf              = 1,
  .enable_noise_gate            = 0,
  .enable_soft_clipping         = 1,
  .enable_air_effect            = 0,
  .lpf_makeup_gain_q16          = LPF_MAKEUP_GAIN_Q16,
  .lpf_16bit_level              = LPF_Custom,
  .lpf_16bit_custom_alpha       = LPF_16BIT_SOFT
};

/* Playback state variables */
volatile  uint8_t         *pb_p8;
volatile  uint8_t         *pb_end8;
volatile  uint16_t        *pb_p16;
volatile  uint16_t        *pb_end16;

volatile  uint8_t         pb_state                    = PB_Idle;
volatile  uint8_t         half_to_fill;
          uint8_t         pb_mode;
          uint32_t        I2S_PlaybackSpeed           = 22000;  // Default

/* Playback engine control variables */
          uint32_t        p_advance;
          PB_ModeTypeDef  channels                    = Mode_mono;
volatile  uint32_t        fadeout_samples_remaining   = 0;
volatile  uint32_t        fadein_samples_remaining    = 0;
volatile  uint8_t         fadeout_complete            = 0;

/* Fade time configuration (stored in seconds, converted to samples based on playback speed) */
          float           fadein_time_seconds         = 0.150f;  // 150ms default
          float           fadeout_time_seconds        = 0.150f;  // 150ms default
          float           pause_fadeout_time_seconds  = 0.100f;  // 100ms default
          float           pause_fadein_time_seconds   = 0.100f;  // 100ms default
          uint32_t        fadein_samples              = 3300;    // Calculated from time and speed
          uint32_t        fadeout_samples             = 3300;    // Calculated from time and speed
          uint32_t        pause_fadeout_samples       = 2200;    // Calculated from time and speed
          uint32_t        pause_fadein_samples        = 2200;    // Calculated from time and speed
/* DC filter state */
volatile  int32_t         dc_filter_prev_input_left   = 0;
volatile  int32_t         dc_filter_prev_input_right  = 0;
volatile  int32_t         dc_filter_prev_output_left  = 0;
volatile  int32_t         dc_filter_prev_output_right = 0;

/* Dither state */
volatile  uint32_t        dither_state                = 12345;

/* Biquad filter state for 8-bit samples */
volatile  int32_t         lpf_8bit_x1_left            = 0;
volatile  int32_t         lpf_8bit_x2_left            = 0;
volatile  int32_t         lpf_8bit_y1_left            = 0;
volatile  int32_t         lpf_8bit_y2_left            = 0;

volatile  int32_t         lpf_8bit_x1_right           = 0;
volatile  int32_t         lpf_8bit_x2_right           = 0;
volatile  int32_t         lpf_8bit_y1_right           = 0;
volatile  int32_t         lpf_8bit_y2_right           = 0;

          uint16_t        lpf_8bit_alpha              = LPF_MEDIUM;

/* Biquad filter state for 16-bit samples */
volatile  uint16_t        lpf_16bit_alpha             = LPF_16BIT_SOFT;

volatile  int32_t         lpf_16bit_x1_left           = 0;
volatile  int32_t         lpf_16bit_x2_left           = 0;
volatile  int32_t         lpf_16bit_y1_left           = 0;
volatile  int32_t         lpf_16bit_y2_left           = 0;

volatile  int32_t         lpf_16bit_x1_right          = 0;
volatile  int32_t         lpf_16bit_x2_right          = 0;
volatile  int32_t         lpf_16bit_y1_right          = 0;
volatile  int32_t         lpf_16bit_y2_right          = 0;

/* Air Effect (High-Shelf) filter state for 16-bit samples */
volatile  int32_t         air_effect_x1_left          = 0;
volatile  int32_t         air_effect_y1_left          = 0;

volatile  int32_t         air_effect_x1_right         = 0;
volatile  int32_t         air_effect_y1_right         = 0;

/* Air Effect runtime shelf gain (Q16). Defaults to AIR_EFFECT_SHELF_GAIN */
volatile  int32_t         air_effect_shelf_gain_q16   = AIR_EFFECT_SHELF_GAIN;

/* Air Effect preset table (dB) */
static const float        air_effect_presets_db[]     = { 1.0f, 2.0f, 3.0f };
#define AIR_EFFECT_PRESET_COUNT ( (uint8_t)( sizeof(air_effect_presets_db) / sizeof(air_effect_presets_db[0]) ) )
static volatile uint8_t   air_effect_preset_idx       = 1; // default +2 dB

/* Pause/resume state tracking */
volatile  PB_StatusTypeDef pb_paused_state            = PB_Idle;
volatile  uint32_t        paused_sample_index         = 0;
volatile  const void      *paused_sample_ptr          = NULL;
volatile  uint32_t        paused_total_samples        = 0;

/* ===== Filter State Reset Macros ===== */

#define RESET_DC_FILTER_STATE() \
  do { \
    dc_filter_prev_input_left   = 0;  dc_filter_prev_input_right  = 0; \
    dc_filter_prev_output_left  = 0;  dc_filter_prev_output_right = 0; \
  } while(0)

#define RESET_8BIT_BIQUAD_STATE() \
  do { \
    lpf_8bit_x1_left  = 0;  lpf_8bit_x1_right = 0; \
    lpf_8bit_x2_left  = 0;  lpf_8bit_x2_right = 0; \
    lpf_8bit_y1_left  = 0;  lpf_8bit_y1_right = 0; \
    lpf_8bit_y2_left  = 0;  lpf_8bit_y2_right = 0; \
  } while(0)

#define RESET_16BIT_BIQUAD_STATE() \
  do { \
    lpf_16bit_x1_left  = 0;  lpf_16bit_x1_right = 0; \
    lpf_16bit_x2_left  = 0;  lpf_16bit_x2_right = 0; \
    lpf_16bit_y1_left  = 0;  lpf_16bit_y1_right = 0; \
    lpf_16bit_y2_left  = 0;  lpf_16bit_y2_right = 0; \
  } while(0)

#define RESET_AIR_EFFECT_STATE() \
  do { \
    air_effect_x1_left  = 0;  air_effect_x1_right = 0; \
    air_effect_y1_left  = 0;  air_effect_y1_right = 0; \
  } while(0)

#define RESET_ALL_FILTER_STATE() \
  do { \
    RESET_DC_FILTER_STATE(); \
    RESET_8BIT_BIQUAD_STATE(); \
    RESET_16BIT_BIQUAD_STATE(); \
    RESET_AIR_EFFECT_STATE(); \
  } while(0)

/* ===== Audio Engine Initialization ===== */

/** Initialize the audio engine with hardware interface functions
  * 
  * @brief Initializes the audio engine and validates that all required hardware
  *        interface functions have been provided. Resets all filter state and
  *        playback variables to safe defaults.
  * 
  * @param: dac_switch - Function pointer for DAC on/off control
  * @param: read_volume - Function pointer for reading volume setting
  * @param: i2s_init - Function pointer for I2S initialization
  * @retval: PB_Playing if initialization successful, PB_Error if any pointer is NULL
  */
PB_StatusTypeDef AudioEngine_Init( DAC_SwitchFunc dac_switch,
                                   ReadVolumeFunc read_volume,
                                   I2S_InitFunc i2s_init )
{
  /* Validate that all required function pointers are provided */
  if( dac_switch == NULL || read_volume == NULL || i2s_init == NULL ) {
    return PB_Error;
  }
  
  /* Assign hardware interface functions */
  AudioEngine_DACSwitch = dac_switch;
  AudioEngine_ReadVolume = read_volume;
  AudioEngine_I2SInit = i2s_init;
  
  /* Reset all filter state variables to clean state */
  RESET_ALL_FILTER_STATE();
  
  /* Reset playback state variables */
  pb_state                                  = PB_Idle;
  pb_mode                                   = 0;
  fadeout_samples_remaining                 = 0;
  fadein_samples_remaining                  = 0;
  fadeout_complete                          = 0;
  paused_sample_ptr                         = NULL;
  vol_input                                   = 1;
  
  /* Reset dither state to non-zero seed */
  dither_state = 12345;
  
  /* Initialize default filter configuration */
  filter_cfg.enable_16bit_biquad_lpf        = 1;
  filter_cfg.enable_soft_dc_filter_16bit    = 1;
  filter_cfg.enable_8bit_lpf                = 1;
  filter_cfg.enable_noise_gate              = 0;
  filter_cfg.enable_soft_clipping           = 1;
  filter_cfg.enable_air_effect              = 0;
  filter_cfg.lpf_makeup_gain_q16            = LPF_MAKEUP_GAIN_Q16;
  filter_cfg.lpf_16bit_level                = LPF_Soft;
  
  return PB_Idle;  // Success - ready to play but not currently playing
}


/* ===== Filter Configuration Functions ===== */

/** Setup filter configuration 
  * 
  * @brief Sets the filter configuration parameters.
  * @param: cfg - Pointer to FilterConfig_TypeDef structure with desired settings.
  * @retval: none
  */
void SetFilterConfig( const FilterConfig_TypeDef *cfg )
{
  if( cfg != NULL ) {
    filter_cfg = *cfg;
    // Ensure a sane makeup gain (default if zero)
    if( filter_cfg.lpf_makeup_gain_q16 == 0 ) {
      filter_cfg.lpf_makeup_gain_q16 = LPF_MAKEUP_GAIN_Q16;
    }
  }
}


/** Get current filter configuration 
  * 
  * @brief Retrieves the current filter configuration parameters.
  * @param: cfg - Pointer to FilterConfig_TypeDef structure to populate.
  * @retval: none
  */
void GetFilterConfig( FilterConfig_TypeDef *cfg )
{
  if( cfg != NULL ) {
    *cfg = filter_cfg;
  }
}


/** Set whether or not to use soft clipping
  *
  * @brief Enables or disables the soft clipping filter.
  * @param: enabled - Non-zero to enable, zero to disable.
  * @retval: none
  */
void SetSoftClippingEnable( uint8_t enabled )
{
  filter_cfg.enable_soft_clipping = enabled ? 1 : 0;
}


/** Get whether or not soft clipping is enabled
  *
  * @brief Retrieves the current state of the soft clipping filter.
  * @param: none
  * @retval: non-zero if enabled, zero if disabled.
  */
uint8_t GetSoftClippingEnable( void )
{
  return filter_cfg.enable_soft_clipping;
}


/** Set the aggressiveness level for the 8-bit low-pass filter.
  * 
  * @brief Sets the filter level for the 8-bit biquad low-pass filter.
  * @param: level - Filter level (LPF_VerySoft, LPF_Soft, LPF_Medium, LPF_Firm, LPF_Aggressive, LPF_Custom).
  * @retval: none
  */
void SetLpf8BitLevel( LPF_Level level )
{
  if( level == LPF_Off ) {
    filter_cfg.enable_8bit_lpf = 0;
  } else {
    filter_cfg.enable_8bit_lpf = 1;
    if( level > LPF_Custom ) { // Prevent invalid off state
      level = LPF_VerySoft;
    }
  }
  filter_cfg.lpf_8bit_level = level;
}


/* Get the 8-bit low-pass filter aggressiveness level 
 * @param: none
 * @retval: current filter level
 */
LPF_Level GetLpf8BitLevel(void)
{
  return filter_cfg.lpf_8bit_level;
}


/** Sets whether to use the air effect or not
  * @param: enabled - Non-zero to enable, zero to disable.
  * @retval: none
  */
void SetAirEffectEnable( uint8_t enabled )
{
  filter_cfg.enable_air_effect = enabled ? 1 : 0;
}


/** Gets the state of the air effect enable flag
  * @param: none
  * @retval: non-zero if enabled, zero if disabled.
  */
uint8_t GetAirEffectEnable( void )
{
  return filter_cfg.enable_air_effect;
}


/** Air Effect runtime control: set shelf boost gain (Q16)
  * Clamps to AIR_EFFECT_SHELF_GAIN_MAX to avoid extreme boosts.
  * @param: gain_q16 - Desired shelf gain in Q16 format
  * @retval: none
  */
void SetAirEffectGainQ16( uint32_t gain_q16 )
{
  if( gain_q16 > AIR_EFFECT_SHELF_GAIN_MAX ) {
    gain_q16 = AIR_EFFECT_SHELF_GAIN_MAX;
  }
  air_effect_shelf_gain_q16 = (int32_t)gain_q16;
}


/** Air Effect runtime control: get current shelf boost gain (Q16)
  * @retval: current shelf gain in Q16
  */
uint32_t GetAirEffectGainQ16( void )
{
  return (uint32_t) air_effect_shelf_gain_q16;
}


/** Air Effect runtime control: set shelf boost using dB
  * Converts desired high-frequency boost (at ω=π) to internal G (Q16).
  * Formula: Hπ = 10^(db/20), G = (Hπ*(2-α) - α) / (2*(1-α))
  * @param: db - Desired boost in dB
  * @retval: none
  */
void SetAirEffectGainDb( float db )
{
  const float alpha               = (float)AIR_EFFECT_CUTOFF / 65536.0f;
  const float one_minus_alpha     = 1.0f - alpha;
  const float Hpi                 = powf( 10.0f, db / 20.0f );
  float G                         = (Hpi * ( 2.0f - alpha ) - alpha ) / ( 2.0f * one_minus_alpha );
  if( G < 0.0f ) G = 0.0f;
  uint32_t gain_q16               = (uint32_t)(G * 65536.0f + 0.5f);
  if( gain_q16 > AIR_EFFECT_SHELF_GAIN_MAX ) {
    gain_q16                      = AIR_EFFECT_SHELF_GAIN_MAX;
  }
  SetAirEffectGainQ16( gain_q16 );
}


/** Air Effect runtime control: get current shelf boost in dB (at ω=π)
  * Formula: Hπ = (α + 2(1-α)G) / (2-α), db = 20*log10(Hπ)
  * @retval: current boost in dB
  */
float GetAirEffectGainDb( void )
{
  const float alpha               = (float) AIR_EFFECT_CUTOFF / 65536.0f;
  const float one_minus_alpha     = 1.0f - alpha;
  const float G                   = (float) air_effect_shelf_gain_q16 / 65536.0f;
  const float Hpi                 = ( alpha + 2.0f * one_minus_alpha * G ) / ( 2.0f - alpha );
  return 20.0f * log10f( Hpi );
}


/** Air Effect preset selection by index
  * @param: preset_index - Index of desired preset (0 disables, >0 enables with that preset)
  * @retval: none
  */
void SetAirEffectPresetDb( uint8_t preset_index )
{
  if( preset_index >= AIR_EFFECT_PRESET_COUNT ) {
    preset_index = 0;
  }
  air_effect_preset_idx = preset_index;
  
  /* Auto-enable/disable air effect based on preset: 0 = off, >0 = on */
  SetAirEffectEnable( preset_index > 0 ? 1 : 0 );
  
  SetAirEffectGainDb( air_effect_presets_db[preset_index] );
}


/** Cycle to the next Air Effect preset. Returns the new preset index.
  * @retval: new preset index
  */
uint8_t CycleAirEffectPresetDb( void )
{
  uint8_t next = (uint8_t)( air_effect_preset_idx + 1 );
  if( next >= AIR_EFFECT_PRESET_COUNT ) {
    next = 0;
  }
  SetAirEffectPresetDb( next );
  return air_effect_preset_idx;
}


/** Get the current Air Effect preset index
  * @retval: current preset index
  */
uint8_t GetAirEffectPresetIndex( void )
{
  return air_effect_preset_idx;
}


/** Get the number of available Air Effect presets
 * @retval: preset count
  */
uint8_t GetAirEffectPresetCount( void )
{
  return AIR_EFFECT_PRESET_COUNT;
}

/** Get the dB value of a preset (clamps to current if OOB)
  * @param: preset_index - Index of desired preset
  * @retval: dB value of the preset
  */
float GetAirEffectPresetDb( uint8_t preset_index )
{
  if( preset_index  >=  AIR_EFFECT_PRESET_COUNT ) {
    preset_index    =   air_effect_preset_idx;
  }
  return air_effect_presets_db[preset_index];
}


/** Set LPF makeup gain 
  * 
  * @brief Sets the makeup gain applied after the low-pass filter.
  * @param: gain - Floating-point gain value (0.1 to 2.0).
  * @retval: none
  */
void SetLpfMakeupGain8Bit( float gain )
{
  if( gain < 0.1f ) {
    gain = 0.1f;
  } else if( gain > 2.0f ) {
    gain = 2.0f;
  }
  uint32_t q16 = (uint32_t)( gain * 65536.0f + 0.5f );
  filter_cfg.lpf_makeup_gain_q16 = q16;
}


/**
 * @brief Calculate Q16 alpha for 16-bit LPF from -3dB cutoff and sample rate
 *
 * 1-pole IIR: alpha = exp(-2*pi*fc/fs)
 * For biquad, this is a good starting point for the smoothing factor.
 *
 * @param cutoff_hz - Desired -3dB cutoff frequency in Hz
 * @param sample_rate_hz - Sample rate in Hz
 * @return Q16 alpha coefficient for SetLpf16BitCustomAlpha
 */
uint16_t CalcLpf16BitAlphaFromCutoff( float cutoff_hz, float sample_rate_hz )
{
  /* Validate input parameters */
  if( cutoff_hz <= 0.0f || sample_rate_hz <= 0.0f ) { return 0; }

  float alpha_f = expf( -2.0f * 3.14159265359f * cutoff_hz / sample_rate_hz );
  if( alpha_f < 0.0f ) alpha_f = 0.0f;
  if( alpha_f > 0.99998f ) alpha_f = 0.99998f; // avoid overflow
  return (uint16_t)( alpha_f * 65536.0f + 0.5f );
}


/**
 * @brief Get Q16 alpha for 16-bit LPF custom mode from -3dB cutoff and current sample rate
 * @param cutoff_hz - Desired -3dB cutoff frequency in Hz
 * @return Q16 alpha coefficient
 */
uint16_t GetLpf16BitCustomAlphaFromCutoff( float cutoff_hz )
{
  return CalcLpf16BitAlphaFromCutoff( cutoff_hz, (float)I2S_PlaybackSpeed );
}


/** Set 16-bit LPF filter level
  * 
  * @brief Sets the aggressiveness level for the 16-bit biquad low-pass filter.
  * @param: level - Filter level (LPF_VerySoft, LPF_Soft, LPF_Medium, LPF_Firm, LPF_Aggressive).
  * @retval: none
  */
void SetLpf16BitLevel( LPF_Level level )
{
  filter_cfg.lpf_16bit_level = level;
  switch( level ) {
    case LPF_Off:
      filter_cfg.enable_16bit_biquad_lpf = 0;
      break;
    case LPF_VerySoft:
      lpf_16bit_alpha = LPF_16BIT_VERY_SOFT;
      break;
    case LPF_Soft:
      lpf_16bit_alpha = LPF_16BIT_SOFT;
      break;
    case LPF_Medium:
      lpf_16bit_alpha = LPF_16BIT_MEDIUM;
      break;
    case LPF_Firm:
      lpf_16bit_alpha = LPF_16BIT_FIRM;
      break;
    case LPF_Aggressive:
      lpf_16bit_alpha = LPF_16BIT_AGGRESSIVE;
      break;
    case LPF_Custom:
      lpf_16bit_alpha = filter_cfg.lpf_16bit_custom_alpha;
      break;
    default:
      lpf_16bit_alpha = LPF_16BIT_SOFT;
      filter_cfg.lpf_16bit_level = LPF_Soft;
      break;
  }

  if( level != LPF_Off ) {
      filter_cfg.enable_16bit_biquad_lpf = 1;
  }
}

void SetLpf16BitCustomAlpha( uint16_t alpha )
{
    filter_cfg.lpf_16bit_custom_alpha = alpha;
    if( filter_cfg.lpf_16bit_level == LPF_Custom ) {
      lpf_16bit_alpha = filter_cfg.lpf_16bit_custom_alpha;
    }
}


/** Convert fade time in seconds to sample count
  * 
  * @brief Helper function to convert fade time to samples with bounds checking.
  * @param: seconds - Fade time in seconds (will be clamped to 0.001-5.0 range).
  * @retval: uint32_t - Number of samples (minimum 1).
  */
static uint32_t FadeTimeToSamples( float seconds )
{
  if( seconds < 0.001f ) seconds = 0.001f;
  if( seconds > 5.0f ) seconds = 5.0f;
  uint32_t samples = (uint32_t) ( seconds * (float) I2S_PlaybackSpeed + 0.5f );
  return ( samples == 0 ) ? 1 : samples;
}


/** Set fade-in time in seconds
  * 
  * @brief Sets the fade-in duration based on the current playback speed.
  * @param: seconds - Fade-in time in seconds (0.001 to 5.0).
  * @retval: none
  */
void SetFadeInTime( float seconds )
{
  fadein_time_seconds = seconds;
  fadein_samples      = FadeTimeToSamples( seconds );
}


/** Get fade-in time in seconds
  * 
  * @brief Returns the current fade-in duration in seconds.
  * @retval: float - Fade-in time in seconds.
  */
float GetFadeInTime( void )
{
  return fadein_time_seconds;
}


/** Set fade-out time in seconds
  * 
  * @brief Sets the fade-out duration based on the current playback speed.
  * @param: seconds - Fade-out time in seconds (0.001 to 5.0).
  * @retval: none
  */
void SetFadeOutTime( float seconds )
{
  fadeout_time_seconds = seconds;
  fadeout_samples      = FadeTimeToSamples( seconds );
}


/** Get fade-out time in seconds
  * 
  * @brief Returns the current fade-out duration in seconds.
  * @retval: float - Fade-out time in seconds.
  */
float GetFadeOutTime( void )
{
  return fadeout_time_seconds;
}


/** Set pause fade-out time in seconds
  * 
  * @brief Sets the pause fade-out duration based on the current playback speed.
  * @param: seconds - Pause fade-out time in seconds (0.001 to 5.0).
  * @retval: none
  */
void SetPauseFadeTime( float seconds )
{
  pause_fadeout_time_seconds = seconds;
  pause_fadeout_samples      = FadeTimeToSamples( seconds );
}


/** Get pause fade-out time in seconds
  * 
  * @brief Returns the current pause fade-out duration in seconds.
  * @retval: float - Pause fade-out time in seconds.
  */
float GetPauseFadeTime( void )
{
  return pause_fadeout_time_seconds;
}


/** Set resume fade-in time in seconds
  * 
  * @brief Sets the resume fade-in duration based on the current playback speed.
  * @param: seconds - Resume fade-in time in seconds (0.001 to 5.0).
  * @retval: none
  */
void SetResumeFadeTime( float seconds )
{
  pause_fadein_time_seconds = seconds;
  pause_fadein_samples      = FadeTimeToSamples( seconds );
}


/** Get resume fade-in time in seconds
  * 
  * @brief Returns the current resume fade-in duration in seconds.
  * @retval: float - Resume fade-in time in seconds.
  */
float GetResumeFadeTime( void )
{
  return pause_fadein_time_seconds;
}


/** Get the current playback speed
  * 
  * @param: none
  * @retval: uint32_t - Current playback speed in Hz
  */
uint32_t GetPlaybackSpeed( void )
{
    return I2S_PlaybackSpeed;
}


/* ===== DSP Filter Functions ===== */


/** Apply TPDF dithering during 8-bit to 16-bit conversion with cubic interpolation
  * 
  * Uses a fast Linear Congruential Generator for random noise generation.
  * 
  * @param: sample8 - Unsigned 8-bit audio sample
  * @retval: int16_t - Signed 16-bit dithered audio sample
  */
static int16_t Apply8BitDithering( uint8_t sample8 )
{
  // Convert unsigned 8-bit (0..255) to signed 16-bit
  int16_t sample16  = (int16_t)( sample8 - 128 ) << 8;
  
  // Generate TPDF (Triangular Probability Density Function) dither
  dither_state      = dither_state * 1103515245U + 12345U;
  int32_t rand1     = ( dither_state >> 16 ) & 0xFF;
  
  dither_state      = dither_state * 1103515245U + 12345U;
  int32_t rand2     = ( dither_state >> 16 ) & 0xFF;
  
  int16_t dither    = (int16_t)( ( rand1 - rand2 ) >> 6 );
  
  return sample16 + dither;
}


/** Apply low-pass biquad filter to 8-bit sample
  * 
  * @param: sample - Signed 16-bit audio sample (from 8-bit input)
  * @param: y2 - Pointer to previous output samples
  * @retval: int16_t - Filtered signed 16-bit audio sample
  */
static int16_t ApplyLowPassFilter8Bit( int16_t sample, volatile int32_t *y1 )
{
  int32_t alpha = lpf_8bit_alpha;
  int32_t one_minus_alpha = 65536 - alpha;
  int32_t output = ( ( alpha * sample) >> 16 ) + 
                   ( ( one_minus_alpha * ( *y1 ) ) >> 16 );
  // Apply makeup gain
  output = ( output * (int32_t)filter_cfg.lpf_makeup_gain_q16 ) >> 16;
  *y1 = output;
  if( output > 32767 )  output = 32767;
  if( output < -32768 ) output = -32768;
  return (int16_t) output;
}


/** Apply fade-in effect to audio sample
  * 
  * @param: sample - Signed 16-bit audio sample
  * @retval: int16_t - Faded-in signed 16-bit audio sample
  */
static int16_t ApplyFadeIn( int16_t sample ) 
{
  if( fadein_samples_remaining > 0 ) {
    int32_t progress    = fadein_samples - fadein_samples_remaining;

    // Use 64-bit intermediate to prevent overflow when squaring progress
    int64_t fade_mult   = ( (int64_t)progress * (int64_t)progress ) / fadein_samples;
    int64_t result      = ( (int64_t)sample * fade_mult ) / fadein_samples;

    // Clamp to valid 16-bit range
    if( result > 32767 )  result = 32767;
    if( result < -32768 ) result = -32768;

    return (int16_t)result;
  }
  return sample;
}


/** Apply fade-out effect to audio sample
  * 
  * @param: sample - Signed 16-bit audio sample
  * @retval: int16_t - Faded-out signed 16-bit audio sample
  */
static int16_t ApplyFadeOut( int16_t sample )
{
  if( fadeout_samples_remaining > 0 && fadeout_samples_remaining <= fadeout_samples ) {

    // Use 64-bit intermediate to prevent overflow when squaring fadeout_samples_remaining
    int64_t fade_mult   = ( (int64_t)fadeout_samples_remaining * (int64_t)fadeout_samples_remaining ) / fadeout_samples;
    int64_t result      = ( (int64_t)sample * fade_mult ) / fadeout_samples;

    // Clamp to valid 16-bit range
    if( result > 32767 )  result = 32767;
    if( result < -32768 ) result = -32768;

    return (int16_t)result;
  }
  return sample;
}


/** Apply noise gate to audio sample
  * 
  * @param: sample - Signed 16-bit audio sample
  * @retval: int16_t - Noise-gated signed 16-bit audio sample
  */
static int16_t ApplyNoiseGate( int16_t sample )
{
  int16_t abs_sample = ( sample < 0 ) ? -sample : sample;
  if( abs_sample < NOISE_GATE_THRESHOLD ) {
    // Soft gate: attenuate signal using fixed-point integer math (Q15)
    // Example: 0.1 attenuation = 3277 (0.1 * 32768)
    const int16_t attenuation_q15 = 3277; // ~0.1 in Q15
    return (int16_t)((sample * attenuation_q15) >> 15);
  }
  return sample;
}


/** Update fade in/out counters
  * 
  * @param: samples_processed - Number of samples processed (1 for mono, 2 for stereo)
  */
static inline void UpdateFadeCounters( uint32_t samples_processed )
{
  if( fadein_samples_remaining > 0 ) {
    fadein_samples_remaining = (fadein_samples_remaining > samples_processed) ? 
                               fadein_samples_remaining - samples_processed : 0;
  }
  if( fadeout_samples_remaining > 0 ) {
    fadeout_samples_remaining = ( fadeout_samples_remaining > samples_processed ) ? 
                                fadeout_samples_remaining - samples_processed : 0;
    // Set completion flag when fadeout reaches zero
    if( fadeout_samples_remaining == 0 ) {
      fadeout_complete = 1;
    }
  }
}


/** Warm up 16-bit biquad filter state to avoid startup transient
  * 
  * @param: sample - Initial sample value to use for warmup iterations
  */
static inline void WarmupBiquadFilter16Bit( int16_t sample )
{
  // Run multiple passes to let aggressive filters settle smoothly
  for( uint8_t i = 0; i < BIQUAD_WARMUP_CYCLES; i++ ) {
    ApplyLowPassFilter16Bit( sample, &lpf_16bit_x1_left, &lpf_16bit_x2_left,
                             &lpf_16bit_y1_left, &lpf_16bit_y2_left );
    ApplyLowPassFilter16Bit( sample, &lpf_16bit_x1_right, &lpf_16bit_x2_right,
                             &lpf_16bit_y1_right, &lpf_16bit_y2_right );
  }
}


/** Compute soft clipping curve response
  * 
  * Helper function that calculates the smooth clipping curve.
  * Uses a cubic smoothstep function to gradually limit excess signal.
  * 
  * @param: excess - Amount by which signal exceeds threshold
  * @param: range - Dynamic range available beyond threshold (max - threshold)
  * @retval: int32_t - Curve scaling factor (typically 0 to 65536)
  */
static inline int32_t ComputeSoftClipCurve( int32_t excess, int32_t range )
{
  int32_t x = excess * 65536 / range;
  if( x > 65536 ) x = 65536;
  int32_t x2 = ( x * x ) >> 16;
  int32_t x3 = ( x2 * x ) >> 16;

  return ( ( 3 * x2 ) >> 1 ) - ( ( 2 * x3 ) >> 1 );
}


/** Apply soft clipping to audio sample
  * 
  * @param: sample - Signed 16-bit audio sample
  * @retval: int16_t - Soft-clipped signed 16-bit audio sample
  */
static int16_t ApplySoftClipping( int16_t sample )
{
  const int32_t threshold = 28000;
  const int32_t max_val   = 32767;
  
  int32_t s = sample;
  
  if( s > threshold ) {
    int32_t excess   = s - threshold;
    int32_t range    = max_val - threshold;
    int32_t curve    = ComputeSoftClipCurve( excess, range );
    s                = threshold + ( ( range * curve ) >> 16 );
  }
  else if( s < -threshold ) {
    int32_t excess   = -threshold - s;
    int32_t range    = max_val - threshold;
    int32_t curve    = ComputeSoftClipCurve( excess, range );
    s                = -threshold - ( ( range * curve ) >> 16 );
  }
  
  if( s > max_val )   s = max_val;
  if( s < -max_val )  s = -max_val;
  
  return (int16_t) s;
}


/** Apply DC blocking filter to audio sample
  * 
  * @param: input - Signed 16-bit audio sample
  * @param: prev_input - Pointer to previous input sample
  * @param: prev_output - Pointer to previous output sample
  * @retval: int16_t - DC-blocked signed 16-bit audio sample
  */
static int16_t ApplyDCBlockingFilter( volatile int16_t input, volatile int32_t *prev_input, volatile int32_t *prev_output )
{
  int32_t output = input - *prev_input + 
                   ( ( *prev_output * DC_FILTER_ALPHA ) >> DC_FILTER_SHIFT );
  
  *prev_input   = input;
  *prev_output  = output;
  
  if ( output > 32767 )   output = 32767;
  if ( output < -32768 )  output = -32768;
  
  return ( int16_t ) output;
}


/** Apply soft DC filter to audio sample
  * 
  * @param: input -lpf_16bit_alpha;  // Use runtime-configurable alphaudio sample
  * @param: prev_input - Pointer to previous input sample
  * @param: prev_output - Pointer to previous output sample
  * @retval: int16_t - Soft DC-filtered signed 16-bit audio sample
  */
static int16_t ApplySoftDCFilter16Bit( volatile int16_t input, volatile int32_t *prev_input, volatile int32_t *prev_output )
{
  int32_t output = input - *prev_input + 
                   ( ( *prev_output * SOFT_DC_FILTER_ALPHA ) >> DC_FILTER_SHIFT );
  
  *prev_input   = input;
  *prev_output  = output;
  
  if ( output > 32767 )   output = 32767;
  if ( output < -32768 )  output = -32768;
  
  return ( int16_t ) output;
}


/** Apply low-pass biquad filter to 16-bit sample
  * 
  * @param: input - Signed 16-bit audio sample
  * @param: x1, x2 - Pointers to previous input samples
  * @param: y1, y2 - Pointers to previous output samples
  * @retval: int16_t - Filtered signed 16-bit audio sample
  */
static int16_t ApplyLowPassFilter16Bit( int16_t input, volatile int32_t *x1, volatile int32_t *x2, volatile int32_t *y1, volatile int32_t *y2 )
{
  uint32_t alpha = lpf_16bit_alpha;
  int32_t b0 = ((65536 - alpha) * (65536 - alpha)) >> 17;
  int32_t b1 = b0 << 1;
  int32_t b2 = b0;
  int32_t a1 = -(alpha << 1);
  int32_t a2 = (alpha * alpha) >> 16;
  /* Use 64-bit accumulation to avoid overflow on aggressive coefficients */
  int64_t acc = ((int64_t)b0 * input) +
                ((int64_t)b1 * (*x1)) +
                ((int64_t)b2 * (*x2)) -
                ((int64_t)a1 * (*y1)) -
                ((int64_t)a2 * (*y2));
  int32_t output = (int32_t)(acc >> 16);
  *x2 = *x1;
  *x1 = input;
  *y2 = *y1;
  *y1 = output;
  if ( output > 32767 )   output = 32767;
  if ( output < -32768 )  output = -32768;
  return (int16_t)output;
}


/** Apply Air Effect (High-Shelf Brightening) to 16-bit sample
  * 
  * One-pole high-shelf filter to add brightness/presence to audio.
  * Boosts frequencies above cutoff (~5-6 kHz @ 22 kHz sample rate).
  * 
  * @param: input - Signed 16-bit audio sample
  * @param: x1 - Pointer to previous input sample
  * @param: y1 - Pointer to previous output sample
  * @retval: int16_t - Filtered signed 16-bit audio sample
  */
static int16_t ApplyAirEffect( int16_t input, volatile int32_t *x1, volatile int32_t *y1 )
{
  // Air effect uses high-shelf filter to brighten treble
  int32_t alpha               = AIR_EFFECT_CUTOFF;              // ~0.75
  int32_t one_minus_alpha     = 65536 - alpha;                  // ~0.25
  int32_t shelf_gain          = air_effect_shelf_gain_q16;      // runtime-adjustable boost
  
  // High-pass portion: amplify high frequencies
  // Use 64-bit intermediates to prevent overflow when multiplying Q16 terms
  int32_t high_freq           = (int32_t)input - *x1;
  int64_t air_boost64         = ( ( (int64_t)high_freq * (int64_t)one_minus_alpha ) >> 16 );
  air_boost64                 = ( ( air_boost64 * (int64_t)shelf_gain ) >> 16 );
  int32_t air_boost           = (int32_t)air_boost64;
  
  // Output = low-pass + high-pass boost
  int64_t out64   = ( ( (int64_t)alpha * (int64_t)input ) >> 16 ) +
                  ( ( ( (int64_t)(65536 - alpha) * (int64_t)(*y1) ) >> 16 ) ) +
                  (int64_t)air_boost;
  int32_t output  = (int32_t)out64;
  
  *x1 = input;
  *y1 = output;
  
  if ( output > 32767 )  output = 32767;
  if ( output < -32768 ) output = -32768;
  
  return (int16_t)output;
}


/** Apply full filter chain to 16-bit sample
  * 
  * @param: sample - Signed 16-bit audio sample
  * @param: is_left_channel - 1 if left channel, 0 if right channel
  * @retval: int16_t - Processed signed 16-bit audio sample
  */
static int16_t ApplyFilterChain16Bit( int16_t sample, uint8_t is_left_channel )
{
  if( filter_cfg.enable_16bit_biquad_lpf ) {
    if( is_left_channel ) {
      sample = ApplyLowPassFilter16Bit( sample, &lpf_16bit_x1_left, &lpf_16bit_x2_left, 
                                        &lpf_16bit_y1_left, &lpf_16bit_y2_left );
    } else {
      sample = ApplyLowPassFilter16Bit( sample, &lpf_16bit_x1_right, &lpf_16bit_x2_right, 
                                        &lpf_16bit_y1_right, &lpf_16bit_y2_right );
    }
  }
  
  if( filter_cfg.enable_soft_dc_filter_16bit ) {
    if( is_left_channel ) {
      sample = ApplySoftDCFilter16Bit( sample, &dc_filter_prev_input_left, &dc_filter_prev_output_left );
    } else {
      sample = ApplySoftDCFilter16Bit( sample, &dc_filter_prev_input_right, &dc_filter_prev_output_right );
    }
  } else {
    if( is_left_channel ) {
      sample = ApplyDCBlockingFilter( sample, &dc_filter_prev_input_left, &dc_filter_prev_output_left );
    } else {
      sample = ApplyDCBlockingFilter( sample, &dc_filter_prev_input_right, &dc_filter_prev_output_right );
    }
  }
  
  if( filter_cfg.enable_air_effect ) {
    if( is_left_channel ) {
      sample = ApplyAirEffect( sample, &air_effect_x1_left, &air_effect_y1_left );
    } else {
      sample = ApplyAirEffect( sample, &air_effect_x1_right, &air_effect_y1_right );
    }
  }
  
  sample = ApplyFadeIn( sample );
  sample = ApplyFadeOut( sample );
  
  if( filter_cfg.enable_noise_gate ) {
    sample = ApplyNoiseGate( sample );
  }
  
  if( filter_cfg.enable_soft_clipping ) {
    sample = ApplySoftClipping( sample );
  }
  
  return sample;
}


/** Apply full filter chain to 8-bit sample
  * 
  * @param: sample - Signed 16-bit audio sample
  * @param: is_left_channel - 1 if left channel, 0 if right channel
  * @retval: int16_t - Processed signed 16-bit audio sample
  */
static int16_t ApplyFilterChain8Bit( int16_t sample, uint8_t is_left_channel )
{
  if( filter_cfg.enable_8bit_lpf ) {
    if( is_left_channel ) {
      sample = ApplyLowPassFilter8Bit( sample, &lpf_8bit_y1_left );
    } else {
      sample = ApplyLowPassFilter8Bit( sample, &lpf_8bit_y1_right );
    }
  }
  
  if( filter_cfg.enable_soft_dc_filter_16bit ) {
    if( is_left_channel ) {
      sample = ApplySoftDCFilter16Bit( sample, &dc_filter_prev_input_left, &dc_filter_prev_output_left );
    } else {
      sample = ApplySoftDCFilter16Bit( sample, &dc_filter_prev_input_right, &dc_filter_prev_output_right );
    }
  } else {
    if( is_left_channel ) {
      sample = ApplyDCBlockingFilter( sample, &dc_filter_prev_input_left, &dc_filter_prev_output_left );
    } else {
      sample = ApplyDCBlockingFilter( sample, &dc_filter_prev_input_right, &dc_filter_prev_output_right );
    }
  }
  
  if( filter_cfg.enable_air_effect ) {
    if( is_left_channel ) {
      sample = ApplyAirEffect( sample, &air_effect_x1_left, &air_effect_y1_left );
    } else {
      sample = ApplyAirEffect( sample, &air_effect_x1_right, &air_effect_y1_right );
    }
  }
  
  sample = ApplyFadeIn( sample );
  sample = ApplyFadeOut( sample );
  
  if( filter_cfg.enable_noise_gate ) {
    sample = ApplyNoiseGate( sample );
  }
  
  if( filter_cfg.enable_soft_clipping ) {
    sample = ApplySoftClipping( sample );
  }
  
  return sample;
}


/* ===== State Accessors ===== */

/** Get current playback state
  * 
  * @param: none
  * @retval: uint8_t - Current playback state (PB_Idle, PB_Playing, PB_Paused)
  */
uint8_t GetPlaybackState( void )
{
  return pb_state;
}


/** Set current playback state
  * 
  * @param: state - Desired playback state (PB_Idle, PB_Playing, PB_Paused)
  * @retval: none
  */
void SetPlaybackState( uint8_t state )
{
  pb_state = state;
}


/** Get which half of the buffer to fill
  * 
  * @param: none
  * @retval: uint8_t - FIRST or SECOND half indicator
  */
uint8_t GetHalfToFill( void )
{
  return half_to_fill;
}


/** Set which half of the buffer to fill
  * 
  * @param: half - FIRST or SECOND half indicator
  * @retval: none
  */
void SetHalfToFill( uint8_t half )
{
  half_to_fill = half;
}


/** Set current playback speed
  * 
  * @brief Sets the playback speed for sample processing.  Not to be use mid-playback.
  * @param: speed - Desired playback speed in Hz
  * @retval: none
  */
void SetPlaybackSpeed( uint32_t speed )
{
  I2S_PlaybackSpeed = speed;
}


/* ============================================================================
 * DMA Callbacks
 * ============================================================================
 */

/** Common DMA callback processing logic
  *
  * Helper function to handle buffer refilling for both half-complete and 
  * complete DMA callbacks. Eliminates duplication between the two callbacks.
  *
  * @param: which_half - Which half of buffer to fill (FIRST or SECOND)
  * @retval: none
  */
static inline void ProcessDMACallback( uint8_t which_half )
{
  /* If pausing and fadeout complete, transition to paused and fill buffer with silence */
  if( pb_state == PB_Pausing && fadeout_complete ) {
    memset( pb_buffer, MIDPOINT_S16, sizeof( pb_buffer) );
    pb_state = PB_Paused;
    return;
  }

  // /* If pausing but fadeout not complete, let ProcessNextWaveChunk handle it */
  if( pb_state == PB_Paused ) {
    return;
  }

  half_to_fill = which_half;

  if( pb_mode == 16 ) {
    if( pb_p16 >= pb_end16 ) {
      pb_state = PB_Idle;
      return;
    }
    if( ProcessNextWaveChunk( (int16_t *) pb_p16 ) != PB_Playing ) {
      return;
    }
  }
  else if( pb_mode == 8 ) {
    if( pb_p8 >= pb_end8 ) {
      pb_state = PB_Idle;
      return;
    }
    if( ProcessNextWaveChunk_8_bit( (uint8_t *) pb_p8 ) != PB_Playing ) {
      return;
    }
  }
  AdvanceSamplePointer();
}


/** Handle refilling the first half of the buffer whilst the second half is playing
  *
  * params: hi2s_p I2S port handle.
  * retval: none.
  *
  * NOTE: Also shuts down the playback when the recording is done.
  *
  */
void HAL_I2S_TxHalfCpltCallback( I2S_HandleTypeDef *hi2s_p )
{
  UNUSED( hi2s_p );
  ProcessDMACallback( FIRST );
}


/** Handle refilling the second half of the buffer whilst the first half is playing
  *
  * params: hi2s_p I2S port handle.
  * retval: none.
  *
  * NOTE: Also shuts down the playback when the recording is done.
  *
  */
void HAL_I2S_TxCpltCallback( I2S_HandleTypeDef *hi2s_p )
{
  UNUSED( hi2s_p );
  ProcessDMACallback( SECOND );
}


/** Advance the sample pointer based on playback mode and advance value
  *
  * params: none.
  * retval: none.
  *
  */
void AdvanceSamplePointer( void )
{
  if( pb_mode == 16 ) {  // Advance the 16-bit sample pointer
    pb_p16 += p_advance;
    if( pb_p16 >= pb_end16 ) {
      pb_state = PB_Idle;
      return;
    }
  }
  else if( pb_mode == 8 ) {  // Or advance the 8-bit sample pointer
    pb_p8 += p_advance;
    if( pb_p8 >= pb_end8 ) {
      pb_state = PB_Idle;
      return;
    }
  } 
}


/* ============================================================================
 * Chunk Processing
 * ============================================================================
 */

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
    return PB_Error;
  }

  vol_input = AudioEngine_ReadVolume();
  vol_input = vol_input ? vol_input : 1;
  input   = chunk_p;      // Source sample pointer
  output  = ( half_to_fill == SECOND ) ? (pb_buffer + CHUNK_SZ ) : pb_buffer;

  // Transfer mono audio (scaled for volume) into the stereo buffer.
  // This is done both to eliminate the need for a second resistor
  // and also because the MAX983567A expects stereo audio data, but we only
  // have one speaker.
  //
  for( uint16_t i = 0; i < HALFCHUNK_SZ; i++ )
  {
    if( (uint16_t *) input >=  pb_end16 ) {                                   // Check for end of sample data
      leftsample = MIDPOINT_S16;                                              // Pad with silence if at end 
    }
    else {
      leftsample = ApplyVolumeSetting( *input, vol_input );                     // Apply volume setting
      leftsample = ApplyFilterChain16Bit( leftsample, 1 );                    // Apply complete filter chain
    }
    input++;

    if( channels == Mode_mono ) { rightsample = leftsample; }                 // Right channel is the same as left.
    else {
      if( (uint16_t *) input >=  pb_end16 ) {                                 // Check for end of sample data
        rightsample = MIDPOINT_S16;                                           // Pad with silence if at end
      }
      else { 
        rightsample = ApplyVolumeSetting( *input, vol_input );                  // Right channel
        rightsample = ApplyFilterChain16Bit( rightsample, 0 );                // Apply complete filter chain
      }   // End of right channel processing
      input++;
    }
    *output = leftsample;  output++;                                          // Write samples to output buffer
    *output = rightsample; output++;
    
    // Update fade counters based on samples processed
    uint32_t samples_processed = (channels == Mode_stereo) ? 2 : 1;
    UpdateFadeCounters(samples_processed);
  }
  return PB_Playing;;
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
    return PB_Error;
  }
  vol_input = AudioEngine_ReadVolume();
  vol_input = vol_input ? vol_input : 1;
  input = chunk_p;                                               /* Source sample pointer */
  output = ( half_to_fill == SECOND ) ? ( pb_buffer + CHUNK_SZ ) : pb_buffer;

  // Transfer mono audio (scaled for volume) into the stereo buffer.
  // This is done both to eliminate the need for a second resistor
  // and also because the MAX983567A expects stereo audio data, but we only
  // have one speaker.

  for( uint16_t i = 0; i < HALFCHUNK_SZ; i++ )
  {
    if( (uint8_t *) input >=  pb_end8 ) {                             /* Check for end of sample data */
      leftsample = MIDPOINT_S16;                                      /* Pad with silence if at end */
    }
    else {
      /* Convert unsigned 8-bit (0..255) -> signed 16-bit with dithering */
      uint8_t sample8 = *input;
      leftsample = Apply8BitDithering( sample8 );                     /* Left channel with dithering */
      leftsample = ApplyVolumeSetting( leftsample, vol_input );
      leftsample = ApplyFilterChain8Bit( leftsample, 1 );             /* Apply complete filter chain */
    }
    input++;

    if( channels == Mode_mono ) { rightsample = leftsample; }         // Right channel is the same as left.
    else {    
      if( (uint8_t *) input >=  pb_end8 ) {                           /* Check for end of sample data */
        rightsample = MIDPOINT_S16;                                   /* Pad with silence if at end */
      }
      else {               
        /* Convert unsigned 8-bit (0..255) -> signed 16-bit with dithering */
        uint8_t sample8 = *input;
        rightsample = Apply8BitDithering( sample8 );                  /* Right channel with dithering */
        rightsample = ApplyVolumeSetting( rightsample, vol_input );
        rightsample = ApplyFilterChain8Bit( rightsample, 0 );         /* Apply complete filter chain */
      }
      input++;
    }
    *output = leftsample;  output++;                                  /* Transfer samples to output buffer */
    *output = rightsample; output++;
    
    // Update fade counters based on samples processed
    uint32_t samples_processed = ( channels == Mode_stereo ) ? 2 : 1;
    UpdateFadeCounters( samples_processed );
  }
  return PB_Playing;
}


/* ============================================================================
 * Playback Control Functions
 * ============================================================================
 */

/** Initiates playback of your specified sample
  *
  * @param: const void* sample_to_play.  Pointer to audio sample data (8-bit or 16-bit).
  * @param: uint32_t sample_set_sz.  Many samples to play back.
  * @param: uint32_t playback_speed.  This is the sample rate.
  * @param: uint8_t sample depth.  This should be 8 or 16 bits.
  * @param: PB_ModeTypeDef mode.  Mono or stereo playback.
  * @param: LPF_Level lpf_level.  Low-pass filter level for 8-bit samples. It must be selected even if playing 16-bit samples.
  * @retval: PB_StatusTypeDef.  Indicates success or failure.
  *
  */
PB_StatusTypeDef PlaySample (  
                              const void *sample_to_play, 
                              uint32_t sample_set_sz, 
                              uint32_t playback_speed, 
                              uint8_t sample_depth, 
                              PB_ModeTypeDef mode 
                            ) 
{
  // Parameter sanity checks
  //
  if( ( sample_depth != 16 && sample_depth != 8 ) || 
      ( mode != Mode_mono && mode != Mode_stereo) ||
      sample_set_sz   == 0                        ||
      sample_to_play  == NULL
    ) { return PB_Error; }

  // Ensure volume callback is initialized before starting playback
  if( AudioEngine_ReadVolume == NULL ) {
    return PB_Error;
  }

  // Set low-pass filter alpha coefficient based on filter config
  lpf_8bit_alpha = GetLpf8BitAlpha( filter_cfg.lpf_8bit_level );
  
  if( mode == Mode_stereo ) {                             // Pointer advance amount for stereo/mono mode.
     p_advance = CHUNK_SZ;                                // Two channels worth of samples per chunk
     channels  = Mode_stereo;
  }
  else {                                                  // Or one channels worth of samples per chunk... One lump or two vicar?
    p_advance  = HALFCHUNK_SZ;
    channels   = Mode_mono;     
  }

  I2S_PlaybackSpeed = playback_speed;                     // Set our playback speed.
  
  // Recalculate fade sample counts based on new playback speed
  fadein_samples = (uint32_t)( fadein_time_seconds * (float)I2S_PlaybackSpeed + 0.5f);
  if( fadein_samples == 0 ) fadein_samples = 1;
  fadeout_samples = (uint32_t)(fadeout_time_seconds * (float)I2S_PlaybackSpeed + 0.5f);
  if( fadeout_samples == 0 ) fadeout_samples = 1;
  pause_fadeout_samples = (uint32_t)(pause_fadeout_time_seconds * (float)I2S_PlaybackSpeed + 0.5f);
  if( pause_fadeout_samples == 0 ) pause_fadeout_samples = 1;
  pause_fadein_samples = (uint32_t)(pause_fadein_time_seconds * (float)I2S_PlaybackSpeed + 0.5f);
  if( pause_fadein_samples == 0 ) pause_fadein_samples = 1;
  
  if( AudioEngine_I2SInit ) { AudioEngine_I2SInit(); }  // Initialize I2S peripheral with our chosen sample rate.

  HAL_I2S_DMAStop( &AUDIO_ENGINE_I2S_HANDLE );          // Ensure there is no currently playing sound before starting a new one.
  
  // Reset all filter state for new sample
  RESET_ALL_FILTER_STATE();
  
  // Warm up 16-bit biquad filter state from first sample to avoid startup transient
  if( sample_depth == 16 && filter_cfg.enable_16bit_biquad_lpf ) {
    int16_t first_sample = *((int16_t *)sample_to_play);
    WarmupBiquadFilter16Bit( first_sample );
  }
  
  if( sample_depth == 16 ) {            // For 16-bit, initialize 16-bit sample playback pointers
    pb_p16    = (uint16_t *) sample_to_play;
    pb_end16  = pb_p16 + sample_set_sz;
    pb_mode   = 16;
  }
  else if( sample_depth == 8 ) {        // For 8-bit, initialize 8-bit sample playback pointers
    pb_p8     = (uint8_t *) sample_to_play;
    pb_end8   = pb_p8 + sample_set_sz;
    pb_mode   = 8;
  }
  // Initialize fade counters
  fadeout_samples_remaining = sample_set_sz;
  fadein_samples_remaining  = fadein_samples;
  fadeout_complete          = 0;
  
  // Pre-fill the buffer with processed samples before starting DMA
  // This ensures the fade-in is applied from the very first sample that plays
  half_to_fill = FIRST;
  if( pb_mode == 16 ) {
    if( ProcessNextWaveChunk( (int16_t *) pb_p16 ) != PB_Playing ) { return PB_Error; }
    pb_p16 += p_advance;

    half_to_fill = SECOND;
        if( ProcessNextWaveChunk( (int16_t *) pb_p16 ) != PB_Playing ) { return PB_Error; }
    pb_p16 += p_advance;
    half_to_fill = FIRST;
  }
  else if( pb_mode == 8 ) {
    if( ProcessNextWaveChunk_8_bit( (uint8_t *) pb_p8 ) != PB_Playing ) { return PB_Error; }
    half_to_fill = SECOND;
    if( ProcessNextWaveChunk_8_bit( (uint8_t *) pb_p8 ) != PB_Playing ) { return PB_Error; }
    pb_p8 += p_advance;
    half_to_fill = FIRST;
  }
  
  // Start playback of the recording
  //
  pb_state = PB_Playing;
  HAL_I2S_Transmit_DMA( &AUDIO_ENGINE_I2S_HANDLE, (uint16_t *) pb_buffer, PB_BUFF_SZ );
  return PB_Playing;
}


/** Simple function that waits for the end of playback.
  *
  * @param: none
  * @retval: PB_StatusTypeDef indicating success or failure.
  *
  */
PB_StatusTypeDef WaitForSampleEnd( void )
{
  while( pb_state == PB_Playing ) {
    __NOP();  // Prevent optimizer from removing loop
  }
  
  // Cleanup: Stop DMA transmission now that we're out of the callback context
  // This prevents the I2S_WaitFlagStateUntilTimeout hang that occurs when
  // stopping from within the DMA callback
  if( pb_state != PB_Playing ) {
    HAL_I2S_DMAStop( &AUDIO_ENGINE_I2S_HANDLE );
  }
  
  return pb_state;
}


/**
  * @brief Pause playback with a smooth fade-out
  * 
  * Saves the current playback position and initiates a fade-out over PAUSE_FADEOUT_SAMPLES.
  * After fade-out completes, playback halts without stopping DMA (ready for resume).
  * Call ResumePlayback() to continue from where it paused.
  * 
  * @retval PB_StatusTypeDef - Current playback state (PB_Playing while fading, PB_Paused when done)
  */
PB_StatusTypeDef PausePlayback( void )
{
  if( pb_state != PB_Playing ) {
    return pb_state;  // Can only pause while actively playing
  }
  
  /* Initiate smooth fade-out by transitioning to PB_Pausing state */
  pb_state = PB_Pausing;
  fadeout_samples_remaining = pause_fadeout_samples;
  fadeout_complete          = 0;
  
  /* Save current playback state for resuming (done after setting PB_Pausing to prevent DMA race) */
  if( pb_mode == 16 ) {
    paused_sample_ptr = (const void *)pb_p16;
  } else {
    paused_sample_ptr = (const void *)pb_p8;
  }
  pb_paused_state = PB_Pausing;
  
  return PB_Paused;
}


/**
  * @brief Resume playback from pause with a smooth fade-in
  * 
  * Resumes playback from where it was paused and fades in over FADEIN_SAMPLES.
  * The audio will smoothly ramp up from silence to full volume.
  * 
  * @retval PB_StatusTypeDef - PLAYING if successfully resumed, error state otherwise
  */
PB_StatusTypeDef ResumePlayback( void )
{
  if( pb_state != PB_Paused ) {
    return pb_state;  // Can only resume from paused state
  }
  
  /* Restore playback position from where it was paused */
  if( paused_sample_ptr != NULL ) {
    if( pb_mode == 16 ) {
      pb_p16 = (uint16_t *)paused_sample_ptr;
    } else {
      pb_p8 = (uint8_t *)paused_sample_ptr;
    }
  }
  

  /* Resume playback from where it was paused */
  pb_state = PB_Playing;
  
  /* Reset fade-out counter and initiate smooth fade-in */
  fadeout_samples_remaining = 0;
  fadein_samples_remaining  = pause_fadein_samples;
  
  return PB_Playing;
}


/** Apply volume setting to sample
  * 
  * @param: sample - Signed 16-bit audio sample
  * @param: volume_setting - Volume division factor (1 to 255)
  * @retval: int16_t - Volume-adjusted signed 16-bit audio sample
  */
static inline int16_t ApplyVolumeSetting( int16_t sample, uint8_t volume_setting )
{
  int32_t sample32 = (int32_t) sample;

  return  (int16_t)( sample32 ) * volume_setting / 255;
}


void ShutDownAudio( void )
{
    // Calculate delay needed to drain the DMA buffer based on playback speed
    extern uint32_t I2S_PlaybackSpeed;
    uint32_t buffer_drain_ms = ( PB_BUFF_SZ * 1000 + I2S_PlaybackSpeed - 1 ) / I2S_PlaybackSpeed;
    HAL_Delay( buffer_drain_ms );

    // Stop I2S DMA transmission
    HAL_I2S_DMAStop( &AUDIO_ENGINE_I2S_HANDLE );
}


/* Return the selected alpha value
*
* @param: LPF_Level lpf_level - Desired low-pass filter level
* @retval: uint16_t - Corresponding alpha coefficient for 8-bit LPF
*/
static inline uint16_t GetLpf8BitAlpha( LPF_Level lpf_level )
{
    switch (lpf_level) {
        case LPF_Custom:
            return filter_cfg.lpf_16bit_custom_alpha;
            
        case LPF_VerySoft:
            return LPF_VERY_SOFT;

        case LPF_Soft:
            return LPF_SOFT;

        case LPF_Firm:
            return LPF_FIRM;
            
        case LPF_Aggressive:
            return LPF_AGGRESSIVE;

        case LPF_Medium:
        default:
            return LPF_MEDIUM;
    }
}
