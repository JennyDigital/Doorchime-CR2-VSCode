/**
  ******************************************************************************
  * @file           : audio_engine.h
  * @brief          : Audio playback engine and DSP filter chain
  ******************************************************************************
  * @attention
  *
  * Reusable audio playback engine with runtime-configurable DSP filters.
  * Supports 8-bit and 16-bit samples, mono/stereo playback, and various
  * audio processing effects (LPF, DC blocking, fade in/out, soft clipping).
  *
  ******************************************************************************
  */

#ifndef __AUDIO_ENGINE_H
#define __AUDIO_ENGINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "stm32g4xx_hal.h"

/* I2S handle selection (default: hi2s2). Override in application if needed. */
#ifndef AUDIO_ENGINE_I2S_HANDLE
#define AUDIO_ENGINE_I2S_HANDLE hi2s2
#endif

/* DAC control values */
#define DAC_OFF               0
#define DAC_ON                1

/* Playback engine buffer configuration */
#define PB_BUFF_SZ            2048U
#define CHUNK_SZ              ( PB_BUFF_SZ / 2 )
#define HALFCHUNK_SZ          ( CHUNK_SZ / 2 )
#define FIRST                 0U
#define SECOND                1U

/* Volume configuration */
#define VOL_MULT              3         // Volume multiplication factor for integer math

/* Fade configuration */
#define FADEOUT_SAMPLES       2048U    // About 100ms at 22kHz
#define PAUSE_FADEOUT_SAMPLES 1024U    // About 50ms at 22kHz
#define FADEIN_SAMPLES        2048U    // About 93ms at 22kHz

/* DC blocking filter configuration */
#define DC_FILTER_ALPHA       64225    // 0.98 in fixed-point (64225/65536)
#define DC_FILTER_SHIFT       16       // Right shift for fixed-point division
#define SOFT_DC_FILTER_ALPHA  65216    // 0.995 in fixed-point (65216/65536)

/* Biquad low-pass filter for 16-bit samples */
#define LPF_16BIT_ALPHA       52429    // ~0.80 - gentle filtering (default)

/* 16-bit biquad low-pass filter aggressiveness levels (alpha coefficients) */
/* Higher alpha is heavier filtering for this biquad, so values are ordered light -> heavy. */
#define LPF_16BIT_VERY_SOFT   40960    // 0.625 - light filtering / highest cutoff
#define LPF_16BIT_SOFT        52429    // ~0.80 - gentle filtering
#define LPF_16BIT_MEDIUM      57344    // 0.875 - balanced filtering
#define LPF_16BIT_AGGRESSIVE  63488    // ~0.97 - strong filtering / lowest cutoff

/* Number of cycles to warm up biquad filter state */
#define BIQUAD_WARMUP_CYCLES  16

/* Low-pass filter for 8-bit samples */
#define LPF_8BIT_SHIFT        16       // Right shift for fixed-point division
#define LPF_MAKEUP_GAIN_Q16   70779    // ~1.08x post-LPF makeup (default)

/* 8-bit low-pass filter aggressiveness levels (alpha coefficients in fixed-point) */
#define LPF_VERY_SOFT         61440    // 0.9375 - very gentle filtering
#define LPF_SOFT              57344    // 0.875 - gentle filtering
#define LPF_MEDIUM            49152    // 0.75 - balanced filtering
#define LPF_AGGRESSIVE        40960    // 0.625 - strong filtering

/* Noise gate configuration */
#define NOISE_GATE_THRESHOLD  512      // ~1.5% of full scale

/* Audio silence midpoints */
#define MIDPOINT_U8           127U
#define MIDPOINT_S16          0

/* Playback status type */
typedef enum {
  PB_Idle,
  PB_Error,
  PB_Playing,
  PB_Paused,
  PB_PlayingFailed
} PB_StatusTypeDef;

/* Playback mode type */
typedef enum {
  Mode_stereo,
  Mode_mono
} PB_ModeTypeDef;

/* Low-pass filter aggressiveness type */
typedef enum {
  LPF_VerySoft,
  LPF_Soft,
  LPF_Medium,
  LPF_Aggressive
} LPF_Level;

/* Filter chain runtime configuration */
typedef struct {
  uint8_t enable_16bit_biquad_lpf;
  uint8_t enable_soft_dc_filter_16bit;
  uint8_t enable_8bit_lpf;
  uint8_t enable_noise_gate;
  uint8_t enable_soft_clipping;
  uint32_t lpf_makeup_gain_q16;  // Q16 gain applied after LPF
  LPF_Level lpf_16bit_level;     // Filter level for 16-bit biquad LPF
} FilterConfig_TypeDef;

/* Global audio engine state exposed for hardware initialization */
extern uint16_t I2S_PlaybackSpeed;

/* Audio engine state structure */
typedef struct {
  I2S_HandleTypeDef *hi2s;
  int16_t *pb_buffer;
  uint16_t playback_speed;
} AudioEngine_HandleTypeDef;

/* Filter configuration functions */
void                SetFilterConfig             ( const FilterConfig_TypeDef *cfg );
void                GetFilterConfig             ( FilterConfig_TypeDef *cfg );
void                SetLpfMakeupGain8Bit        ( float gain );
void                SetLpf16BitLevel            ( LPF_Level level );

/* Playback control functions */
PB_StatusTypeDef    PlaySample                  (
                                                  const void *sample_to_play,
                                                  uint32_t sample_set_sz,
                                                  uint16_t playback_speed,
                                                  uint8_t sample_depth,
                                                  PB_ModeTypeDef mode,
                                                  LPF_Level lpf_level
                                                );
PB_StatusTypeDef    WaitForSampleEnd            ( void );
PB_StatusTypeDef    PausePlayback               ( void );
PB_StatusTypeDef    ResumePlayback              ( void );

/* Hardware interface functions (to be implemented by application) */
void ShutDownAudio( void );

/* Chunk processing callbacks (call from DMA callbacks) */
PB_StatusTypeDef    ProcessNextWaveChunk        ( int16_t *chunk_p );
PB_StatusTypeDef    ProcessNextWaveChunk_8_bit  ( uint8_t *chunk_p );
void                AdvanceSamplePointer        ( void );

/* DSP filter functions */
int16_t             Apply8BitDithering          ( uint8_t sample8 );
int16_t             ApplyLowPassFilter16Bit     ( int16_t input,
                                                  volatile int32_t *x1,
                                                  volatile int32_t *x2,
                                                  volatile int32_t *y1,
                                                  volatile int32_t *y2
                                                );
int16_t             ApplyLowPassFilter8Bit      ( 
                                                  int16_t sample,
                                                  volatile int32_t *x1,
                                                  volatile int32_t *x2,
                                                  volatile int32_t *y1,
                                                  volatile int32_t *y2
                                                );
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
int16_t             ApplySoftDCFilter16Bit      (
                                                  volatile int16_t input,
                                                  volatile int32_t *prev_input,
                                                  volatile int32_t *prev_output
                                                );
int16_t             ApplyFilterChain16Bit       ( int16_t sample, uint8_t is_left_channel );
int16_t             ApplyFilterChain8Bit        ( int16_t sample, uint8_t is_left_channel );

/* Hardware callbacks (to be called from I2S DMA callbacks) */
void                HAL_I2S_TxHalfCpltCallback  ( I2S_HandleTypeDef *hi2s );
void                HAL_I2S_TxCpltCallback      ( I2S_HandleTypeDef *hi2s );

/* Hardware interface functions (implemented by application) */
typedef void        ( *DAC_SwitchFunc )         ( GPIO_PinState setting );
typedef uint8_t     ( *ReadVolumeFunc )         ( void );
typedef void        ( *I2S_InitFunc )           ( void );

extern DAC_SwitchFunc AudioEngine_DACSwitch;
extern ReadVolumeFunc AudioEngine_ReadVolume;
extern I2S_InitFunc   AudioEngine_I2SInit;

/* Internal chunk processing functions */
void                AdvanceSamplePointer        ( void );
PB_StatusTypeDef    ProcessNextWaveChunk        ( int16_t * chunk_p );
PB_StatusTypeDef    ProcessNextWaveChunk_8_bit  ( uint8_t * chunk_p );

/* Playback control functions */
PB_StatusTypeDef    PlaySample                  (
                                                  const void *sample_to_play,
                                                  uint32_t sample_set_sz,
                                                  uint16_t playback_speed,
                                                  uint8_t sample_depth,
                                                  PB_ModeTypeDef mode,
                                                  LPF_Level lpf_level
                                                );
PB_StatusTypeDef    WaitForSampleEnd            ( void );
PB_StatusTypeDef    PausePlayback               ( void );
PB_StatusTypeDef    ResumePlayback              ( void );

/* Accessors for global state (needed by application) */
uint8_t             GetPlaybackState            ( void );
void                SetPlaybackState            ( uint8_t state );
uint8_t             GetHalfToFill               ( void );
void                SetHalfToFill               ( uint8_t half );
uint16_t            GetPlaybackSpeed            ( void );
void                SetPlaybackSpeed            ( uint16_t speed );

#ifdef __cplusplus
}
#endif

#endif /* __AUDIO_ENGINE_H */
