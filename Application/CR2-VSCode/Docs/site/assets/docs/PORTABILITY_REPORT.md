# Audio Engine Portability Report

Date: 2026-03-08
Scope: `Core/Libraries/audio_engine.c` and `Core/Libraries/audio_engine.h`

This report highlights non-STM32-specific portability concerns for teams moving the audio engine to less compatible compilers, C runtimes, or CPU architectures.

## Executive Summary

The audio engine is mostly portable C99 code, but it assumes several GCC-style language extensions, ARM-friendly memory/alignment behavior, and simple ISR/main-loop concurrency semantics. On stricter or different targets (non-GNU compilers, unaligned-sensitive CPUs, weak C runtimes), these assumptions can cause build failures, undefined behavior, or subtle runtime bugs.

## Priority Risks

### 1) Unaligned and aliasing-sensitive sample access (High)
- Locations:
  - `Core/Libraries/audio_engine.c:1989`
  - `Core/Libraries/audio_engine.c:1992`
  - `Core/Libraries/audio_engine.c:1997`
- Pattern:
  - `const void *` sample data is cast to `int16_t *`/`uint16_t *` and dereferenced.
- Why this matters:
  - Some architectures fault on unaligned 16-bit loads.
  - Strict aliasing can be violated if backing data object type does not match the cast type.
- Porting guidance:
  - Enforce 2-byte alignment for 16-bit sample assets.
  - Consider replacing direct casts with `memcpy`-based read helpers when target may trap on unaligned access.
  - Document ownership/type rules for `sample_to_play` at API boundary.

### 2) Compiler extension dependence for attributes (High)
- Locations:
  - `Core/Libraries/audio_engine.c:55`
  - `Core/Libraries/audio_engine.c:57`
  - `Core/Libraries/audio_engine.c:127`
- Pattern:
  - `__attribute__((always_inline))`, `__attribute__((noinline))`, `__attribute__((weak))`.
- Why this matters:
  - Non-GNU compilers may reject these or interpret differently.
- Porting guidance:
  - Wrap attributes behind macros in the header, e.g. `AUDIO_WEAK`, `AUDIO_NOINLINE`, `AUDIO_ALWAYS_INLINE`.
  - Provide fallback empty definitions for unsupported compilers.

### 3) HAL type leakage in public API (Medium)
- Location:
  - `Core/Libraries/audio_engine.h:195`
- Pattern:
  - `DAC_SwitchFunc` takes `GPIO_PinState`, a platform-specific type.
- Why this matters:
  - Blocks portability even if function implementation is replaced.
- Porting guidance:
  - Introduce a project-local type (`AudioDacState` enum or `uint8_t`) in the audio API.
  - Keep platform mapping in platform glue layer, not in core header.

### 4) Volatile-based synchronization only (Medium)
- Locations:
  - Shared state declarations around `Core/Libraries/audio_engine.c:166` onward.
  - State read/write paths in playback callbacks and control functions.
- Pattern:
  - ISR/main-loop shared variables are mostly `volatile`, without atomic operations or barriers.
- Why this matters:
  - `volatile` does not guarantee atomicity or memory ordering.
  - Works on many MCU cases, but less robust on SMP/OOO systems or aggressive optimizers.
- Porting guidance:
  - Define a minimal synchronization policy:
    - single-core ISR model with explicit critical sections, or
    - C11 atomics for shared flags/counters.
  - Make update/read ownership explicit in comments and API docs.

### 5) C math library assumptions (Medium)
- Locations:
  - `Core/Libraries/audio_engine.c:521`
  - `Core/Libraries/audio_engine.c:542`
  - `Core/Libraries/audio_engine.c:729`
  - `Core/Libraries/audio_engine.c:749`
  - `Core/Libraries/audio_engine.c:2233`
- Pattern:
  - `powf`, `expf`, `log10f` used for runtime controls.
- Why this matters:
  - Some embedded toolchains need explicit `-lm`.
  - Accuracy/performance can vary widely.
- Porting guidance:
  - Confirm link flags (`-lm`) and acceptable precision/cycles.
  - Optional: provide fixed-point or LUT alternatives for constrained targets.

### 6) Byte-fill silence macro is value-sensitive (Low)
- Location:
  - `Core/Libraries/audio_engine.h:132`
- Pattern:
  - `MIDPOINT_FILL_BUFFER()` uses `memset` on `int16_t` buffer.
- Why this matters:
  - Safe now only because `SAMPLE16_MIDPOINT == 0`.
  - Would be incorrect for non-zero 16-bit silence values.
- Porting guidance:
  - Keep midpoint at zero, or replace with element-wise fill helper if non-zero silence is needed.

### 7) C language baseline is C99+ (Low)
- Locations:
  - Designated initializers and `<stdbool.h>` usage (e.g. `Core/Libraries/audio_engine.c:143`).
- Why this matters:
  - C90-only or non-conforming compilers will fail.
- Porting guidance:
  - Require C99 in build docs, or backport constructs for older compilers.

## Compatibility Checklist

Use this checklist before first port build:

1. Compiler supports C99 and `stdint.h`/`stdbool.h`.
2. Attribute macros are abstracted for non-GNU compilers.
3. Public API types do not expose platform HAL types.
4. 16-bit sample pointers are aligned and alias-safe.
5. Math library is linked and profiled (`powf/expf/log10f`).
6. Shared state policy documented (`volatile` + critical sections or atomics).
7. Endianness and sample packing assumptions are validated with test vectors.

## Recommended Porting Strategy

1. Create a platform shim layer:
   - Keep all platform-specific types and callbacks outside core audio headers.
2. Add compile-time portability macros:
   - Centralize weak/noinline/always_inline handling.
3. Add sample-access helpers:
   - Use alignment-safe reads for 16-bit sample data when needed.
4. Add a tiny portability test suite:
   - Build-only tests for compiler compatibility.
   - Runtime tests for filter output sanity and callback state transitions.

## Validation Suggestions for Porters

- Run with `-Wall -Wextra -Wconversion -Wsign-conversion` (or compiler equivalent).
- Add UB checks where available (`-fsanitize=undefined` on host builds).
- Compare reference output blocks (CRC or RMS/error metrics) across original and ported targets.

## Notes

- This report intentionally excludes STM32 HAL function call replacement details.
- Findings focus on code portability hazards that remain even after HAL abstraction.
