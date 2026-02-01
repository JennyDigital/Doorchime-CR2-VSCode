# Bug Hunt Report - STM32 Doorchime CR2 Project

**Last reviewed:** 2026-02-01

## Open Issues

None.

---

## Resolved Since Last Report

### ✅ Pause/Resume position tracking
**Fixed:** The pause/resume flow now stores `paused_sample_ptr` and restores it in `ResumePlayback()`.
See [Core/Libraries/audio_engine.c](Core/Libraries/audio_engine.c#L1605-L1688).

### ✅ Fade-in/out overflow safety
**Fixed:** Fade calculations now use 64-bit intermediates with clamping.
See [Core/Libraries/audio_engine.c](Core/Libraries/audio_engine.c#L802-L860).

### ✅ Soft clipping curve shift cleanup
**Fixed:** Soft clip curve math now uses consistent shifts.
See [Core/Libraries/audio_engine.c](Core/Libraries/audio_engine.c#L900-L943).

### ✅ Volume scaling range and behavior
**Fixed/Updated:** `ReadVolume()` now scales to 1–255 with optional non-linear response.
See [Core/Src/main.c](Core/Src/main.c#L646-L706).

### ✅ ReadVolume NULL guard
**Fixed:** `PlaySample()` now verifies `AudioEngine_ReadVolume` is initialized before starting playback.
See [Core/Libraries/audio_engine.c](Core/Libraries/audio_engine.c#L1475-L1525).

---

## Summary Table

| # | Severity | Status | File | Issue |
|---|----------|--------|------|-------|
| 1 | ✅ Fixed | Closed | audio_engine.c | `AudioEngine_ReadVolume()` NULL guard |
| 2 | ✅ Fixed | Closed | audio_engine.c | Pause/resume pointer tracking |
| 3 | ✅ Fixed | Closed | audio_engine.c | Fade-in/out overflow risk |
| 4 | ✅ Fixed | Closed | audio_engine.c | Soft clip curve shift |
| 5 | ✅ Fixed | Closed | main.c | Volume scaling range/behavior |

