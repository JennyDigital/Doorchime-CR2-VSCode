# Bug Hunt Report - STM32 Doorchime CR2 Project

## Issues Found

### 🔴 CRITICAL BUG #1: Logic Error in PausePlayback() - Incorrect paused_sample_index Calculation
**File:** [Core/Libraries/audio_engine.c](Core/Libraries/audio_engine.c#L1221)  
**Severity:** Critical  
**Type:** Logic Error

```c
paused_sample_index = (const uint8_t *)pb_p8 - (const uint8_t *)pb_p8;  // Will be restored via pb_p8
```

**Problem:**
- This line calculates `paused_sample_index` by subtracting `pb_p8` from itself, which always results in **zero**
- This means the pause/resume functionality will always resume from the beginning of the audio, not from where it was paused
- The variable is assigned but never used, and the pause state saving is ineffective

**Expected Behavior:**
- Should save the current position of the sample pointer for later restoration

**Fix:**
Should likely be one of:
```c
paused_sample_index = (const uint8_t *)pb_p8 - (const uint8_t *)paused_sample_ptr;
// OR (for 16-bit pointers)
paused_sample_index = (const uint8_t *)pb_p16 - (const uint8_t *)paused_sample_ptr;
// OR if this is the pointer itself:
paused_sample_ptr = pb_p8;
```

---

### 🟡 MEDIUM BUG #2: Potential Null Pointer Dereference in ProcessNextWaveChunk()
**File:** [Core/Libraries/audio_engine.c](Core/Libraries/audio_engine.c#L958-L1020)  
**Severity:** Medium  
**Type:** Null Pointer Dereference Risk

**Problem:**
- At line 958, `vol_div = AudioEngine_ReadVolume();` is called
- `AudioEngine_ReadVolume` is a function pointer that starts as `NULL` (line 27)
- While main.c assigns it at line 166, there's **no validation** that the assignment happened before `PlaySample()` is called
- If `PlaySample()` is called before the pointers are assigned, this will cause a NULL pointer dereference

**Similar Issues at:**
- Line 1021: `vol_div = AudioEngine_ReadVolume();` in `ProcessNextWaveChunk_8_bit()`
- Line 1131-1132: `AudioEngine_I2SInit` check
- Line 1136-1137: `AudioEngine_DACSwitch` check (has NULL check)
- Line 1238-1239: `AudioEngine_DACSwitch` check (has NULL check)

**Impact:**
- Lines 1131 and 1136 have NULL checks, but the audio processing functions don't
- If playback starts before initialization completes, the system will crash

**Recommendation:**
Add NULL checks before calling these function pointers:
```c
vol_div = (AudioEngine_ReadVolume != NULL) ? AudioEngine_ReadVolume() : 1;
```

---

### 🟡 MEDIUM BUG #3: Missing Bounds Check on vol_div
**File:** [Core/Src/main.c](Core/Src/main.c#L520)  
**Severity:** Medium  
**Type:** Integer Underflow

**Problem:**
```c
return v ? v : 1;
```

The safety check prevents returning 0, but there's a potential issue:
- `vol_scaling` can be 1-255
- `v` (the volume bits) can be 1-8
- Multiplying: `(v + 1) * vol_scaling` could produce values up to 9 × 255 = 2295

Then later:
```c
leftsample = ( (int16_t) (*input) / vol_div ) * VOL_MULT;
```

If `vol_div` becomes very large (>32767), the division could result in very small numbers, essentially muting the audio. This might be intentional, but it's worth documenting.

---

### 🟡 MEDIUM BUG #4: Potential Integer Overflow in ApplyFadeOut()
**File:** [Core/Libraries/audio_engine.c](Core/Libraries/audio_engine.c#L408-L415)  
**Severity:** Medium  
**Type:** Potential Overflow

**Problem:**
```c
int16_t ApplyFadeOut( int16_t sample )
{
  if( fadeout_samples_remaining > 0 && fadeout_samples_remaining <= FADEOUT_SAMPLES ) {
    int32_t fade_mult = (fadeout_samples_remaining * fadeout_samples_remaining) / FADEOUT_SAMPLES;
    return (int32_t) sample * fade_mult / FADEOUT_SAMPLES;
```

- `FADEOUT_SAMPLES = 2048`
- `fadeout_samples_remaining * fadeout_samples_remaining` could be up to 2048² = 4,194,304
- Multiplying this by `sample` (up to 32,767) could overflow `int32_t`

**Recommendation:**
```c
int64_t fade_mult = ((int64_t)fadeout_samples_remaining * fadeout_samples_remaining) / FADEOUT_SAMPLES;
return (int32_t)((int64_t)sample * fade_mult / FADEOUT_SAMPLES);
```

---

### 🟡 MEDIUM BUG #5: Similar Overflow in ApplyFadeIn()
**File:** [Core/Libraries/audio_engine.c](Core/Libraries/audio_engine.c#L395-L403)  
**Severity:** Medium  
**Type:** Potential Overflow

**Problem:**
```c
int32_t fade_mult = (progress * progress) / FADEIN_SAMPLES;
return (int32_t) sample * fade_mult / FADEIN_SAMPLES;
```

Same issue as ApplyFadeOut() - squaring the progress value could overflow.

**Recommendation:**
```c
int32_t progress = FADEIN_SAMPLES - fadein_samples_remaining;
int64_t fade_mult = ((int64_t)progress * progress) / FADEIN_SAMPLES;
return (int32_t)(((int64_t)sample * fade_mult) / FADEIN_SAMPLES);
```

---

### 🟡 MEDIUM BUG #6: Uninitialized paused_sample_ptr in main()
**File:** [Core/Src/main.c](Core/Src/main.c#L130-L240)  
**Severity:** Low to Medium  
**Type:** Unused/Incomplete Code

**Problem:**
- The external variable `paused_sample_ptr` is declared and used in `PausePlayback()` for saving state
- However, this value is **never actually used** in `ResumePlayback()` to restore playback position
- The pause/resume mechanism is incomplete

---

### 🟢 OBSERVATION #7: Type Mismatch in ApplySoftClipping()
**File:** [Core/Libraries/audio_engine.c](Core/Libraries/audio_engine.c#L436-L467)  
**Severity:** Low  
**Type:** Code Quality Issue

**Problem:**
```c
int32_t curve = ((3 * x2) >> 1) - ((2 * x3) >> 0);
```

The `>> 0` right shift has no effect and looks unintentional. Should probably be consistent:
```c
int32_t curve = ((3 * x2) >> 1) - (2 * x3);
```

---

## Summary Table

| # | Severity | Type | File | Issue |
|---|----------|------|------|-------|
| 1 | 🔴 Critical | Logic Error | audio_engine.c | paused_sample_index always 0 |
| 2 | 🟡 Medium | Null Pointer | audio_engine.c | ReadVolume() not null-checked |
| 3 | 🟡 Medium | Integer Overflow | audio_engine.c | vol_div could overflow |
| 4 | 🟡 Medium | Integer Overflow | audio_engine.c | ApplyFadeOut() overflow |
| 5 | 🟡 Medium | Integer Overflow | audio_engine.c | ApplyFadeIn() overflow |
| 6 | 🟡 Medium | Incomplete Code | main.c | Pause/resume not fully implemented |
| 7 | 🟢 Low | Code Quality | audio_engine.c | Unnecessary `>> 0` shift |

---

## Recommended Actions

1. **Immediate:** Fix the critical paused_sample_index bug (Bug #1)
2. **Priority:** Add null pointer checks for function pointers (Bug #2)
3. **Soon:** Fix integer overflow issues in fade functions (Bugs #4, #5)
4. **Review:** Complete the pause/resume implementation (Bug #6)
5. **Polish:** Clean up the unnecessary shift operation (Bug #7)

