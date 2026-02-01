# Changelog

All notable changes to the CR2-VSCode Audio Engine project are documented here.

## [2026-02-01] - Complete Documentation & API Reference

### Fixed

#### Biquad Filter Level Documentation
- **Corrected**: 16-bit biquad filter alpha-to-filtering relationship in all documentation
  - Previous documentation incorrectly stated "higher α values provide stronger filtering"
  - Actual implementation uses `b0 = ((65536 - alpha) * (65536 - alpha)) >> 17`, meaning **lower alpha = more filtering**
  - Updated [audio_engine.h](Core/Libraries/audio_engine.h) comments to reflect correct behavior
  - Updated [README.md](Docs/README.md) biquad table with corrected descriptions
  - Updated [AUDIO_ENGINE_MANUAL.md](Docs/AUDIO_ENGINE_MANUAL.md) with correct explanation
  - Clarified that both 16-bit biquad and 8-bit one-pole have the same alpha-to-filtering direction
  - Regenerated [Audio_Engine_Manual.pdf](Docs/Audio_Engine_Manual.pdf) with corrections

### Added

#### Comprehensive API Documentation
- **New File**: [API_REFERENCE.md](Docs/API_REFERENCE.md) - Complete API reference with 40+ functions
  - 8 major categories: Initialization, Playback Control, Filter Configuration, LPF Control, Air Effect, Fade Times, Chunk Processing, Status & Queries
  - Every function documented with parameters, returns, notes, and examples
  - Alphabetical function index
  - Complete working example demonstrating interactive control
  - Organized by use case and category

- **New File**: [QUICK_REFERENCE.md](Docs/QUICK_REFERENCE.md) - Fast lookup guide
  - Common patterns: Simple play, non-blocking, pause/resume, configure-then-play, interactive
  - Function categories with counts
  - Quick lookup by task
  - Cross-references to comprehensive documentation

- **New File**: [HEADER_DOCUMENTATION.md](Docs/HEADER_DOCUMENTATION.md) - Guide to Doxygen header documentation
  - Explains how to use the documented audio_engine.h header
  - IDE integration instructions
  - Doxygen generation guide
  - Documentation format standards
  - Function organization overview

- **New File**: [FUNCTION_INDEX.md](Docs/FUNCTION_INDEX.md) - Complete function catalog
  - Alphabetical index of all 40+ functions
  - Organized by category (8 categories)
  - Quick lookup by task
  - Usage frequency ratings
  - Function signature quick reference
  - Cross-references to comprehensive documentation

- **New File**: [DOCUMENTATION_GUIDE.md](Docs/DOCUMENTATION_GUIDE.md) - Documentation navigation guide
  - "What do I need?" quick navigation by use case
  - Documentation files organized by type (Tutorial, API Reference, Technical, Feature-Specific)
  - Recommended reading paths (Beginner, Intermediate, Advanced, Quick Implementation)
  - Use case workflows with estimated implementation times
  - IDE integration instructions
  - PDF generation guide
  - Search strategies for finding functions, examples, and concepts

- **Updated**: [audio_engine.h](Core/Libraries/audio_engine.h) - Added comprehensive Doxygen documentation
  - All 40+ public functions now have full Doxygen-style documentation
  - `@brief` tags for one-line summaries
  - `@param[in]` / `@param[out]` tags for all parameters with types and ranges
  - `@return` tags explaining return values
  - `@note` tags for special usage notes, preconditions, and alternatives
  - IDE integration: Hover tooltips and IntelliSense now show documentation
  - Doxygen-ready: Can generate HTML/PDF documentation with `doxygen Doxyfile`

- **Updated**: [README.md](Docs/README.md)
  - Expanded Documentation section with comprehensive documentation suite breakdown
  - Organized into User Guides, API Documentation, Technical Reports, Air Effect Documentation, and Change History
  - Added usage guide: First-time users → Quick lookup → API details → IDE integration → Technical deep-dive

- **Updated**: [AUDIO_ENGINE_MANUAL.md](Docs/AUDIO_ENGINE_MANUAL.md)
  - Added reference link to comprehensive API_REFERENCE.md
  - Maintained quick-start and architecture sections
  - All examples updated to reflect latest API

#### Documentation Structure
```
Docs/
├── DOCUMENTATION_GUIDE.md    (NEW) - Navigation guide for all documentation
├── API_REFERENCE.md          (NEW) - 40+ functions with full details
├── QUICK_REFERENCE.md        (NEW) - Fast lookup guide
├── FUNCTION_INDEX.md         (NEW) - Complete function catalog
├── HEADER_DOCUMENTATION.md   (NEW) - Guide to Doxygen header docs
├── AUDIO_ENGINE_MANUAL.md    - Quick start, architecture, examples
├── AIR_EFFECT_QUICK_REFERENCE.md - Air Effect usage
├── AIR_EFFECT_INTEGRATION.md - Detailed air effect technical info
├── README.md                 - Project overview
├── Audio_Engine_Manual.pdf   - PDF version of manual
└── Filter_Report_Enhanced.pdf - Filter analysis with graphs

Core/Libraries/
└── audio_engine.h            (UPDATED) - All functions now Doxygen-documented
```

#### Getter/Setter Functions Documented

**Filter Configuration (6 functions):**
- `SetFilterConfig()` / `GetFilterConfig()`
- `SetSoftClippingEnable()` / `GetSoftClippingEnable()`
- `SetLpfMakeupGain8Bit()`

**8-bit LPF (2 functions):**
- `SetLpf8BitLevel()` / `GetLpf8BitLevel()`

**16-bit LPF (4 functions):**
- `SetLpf16BitLevel()`
- `SetLpf16BitCustomAlpha()`
- `CalcLpf16BitAlphaFromCutoff()`
- `GetLpf16BitCustomAlphaFromCutoff()`

**Air Effect (8 functions):**
- `SetAirEffectEnable()` / `GetAirEffectEnable()`
- `SetAirEffectGainQ16()` / `GetAirEffectGainQ16()`
- `SetAirEffectGainDb()` / `GetAirEffectGainDb()`
- `SetAirEffectPresetDb()` / `CycleAirEffectPresetDb()`
- `GetAirEffectPresetIndex()` / `GetAirEffectPresetCount()`
- `GetAirEffectPresetDb()`

**Fade Control (8 functions):**
- `SetFadeInTime()` / `GetFadeInTime()`
- `SetFadeOutTime()` / `GetFadeOutTime()`
- `SetPauseFadeTime()` / `GetPauseFadeTime()`
- `SetResumeFadeTime()` / `GetResumeFadeTime()`

**Status Queries (2 functions):**
- `GetPlaybackSpeed()`
- `GetPlaybackState()` (external)

### Documentation Improvements

- **Detailed Parameter Descriptions**: Every function parameter documented with type, range, and usage
- **Return Value Documentation**: Clear explanation of what each function returns
- **Code Examples**: Working examples for every major function
- **Use Case Mapping**: Functions organized by use case, not just alphabetically
- **Cross-References**: Links between related functions and concepts
- **Best Practices**: Recommendations for typical workflows

### Files Updated

- `Docs/API_REFERENCE.md` - NEW comprehensive API reference (2000+ lines)
- `Docs/AUDIO_ENGINE_MANUAL.md` - Added API reference link and updated TOC
- `Docs/README.md` - Already reflects latest features
- `Docs/AIR_EFFECT_QUICK_REFERENCE.md` - Already updated in previous release

### Volume Control (from previous update)

- **Non-Linear Volume Response**: Human-perception-matched volume scaling with configurable gamma curve
  - `VOLUME_RESPONSE_NONLINEAR` #define to enable/disable feature
  - `VOLUME_RESPONSE_GAMMA` configurable exponent (default: 2.0 for quadratic curve)
  
- **Digital Volume Control**: Fixed GPIO bit packing for 3-bit digital volume inputs
  - Proper scaling to 1–255 range for audio engine

- **Air Effect Auto-Control**: `SetAirEffectPresetDb()` now auto-enables/disables based on preset

---

## [2026-02-01] - Volume Control & Air Effect Auto-Control

### Added

#### Volume Control System
- **Non-Linear (Logarithmic) Volume Response**: Implemented human-perception-matched volume scaling with configurable gamma curve
  - `VOLUME_RESPONSE_NONLINEAR` #define to enable/disable feature
  - `VOLUME_RESPONSE_GAMMA` configurable exponent (default: 2.0 for quadratic curve)
  - Applied to both digital GPIO and analog ADC volume pathways
  - Gamma=2.0 provides intuitive volume feel: small movements at low volume, large movements at high volume

#### Air Effect Auto-Control
- `SetAirEffectPresetDb()` now auto-enables/disables air effect based on preset index
  - Preset 0 → Air effect OFF
  - Preset >0 → Air effect ON with selected preset
  - Eliminates need for separate `SetAirEffectEnable()` calls

### Fixed

#### Digital Volume GPIO Control
- **Critical Fix**: Corrected GPIO bit packing in `ReadVolume()` 
  - Previous: Bits shifted to MSB positions (7,6,5), returned values 31–255
  - Fixed: Bits packed as 0–2, inverted against 7, scaled to 1–255 range
  - Now properly supports 8 distinct volume levels
  
- **Range Compatibility**: Scaled 3-bit digital input (0–7) to full 1–255 audio engine range
  - Prevents near-silent output when digital volume control used
  - Matches analog ADC pathway output range

### Documentation

- Updated `README.md` with non-linear volume response feature description
- Updated `AUDIO_ENGINE_MANUAL.md`:
  - Added comprehensive "Volume Control" section (pre-Hardware Integration)
  - Documented digital GPIO encoding table
  - Documented analog ADC configuration
  - Explained non-linear gamma curve with perception table
  - Updated `ReadVolume()` example to show both digital and analog paths
- Updated `AIR_EFFECT_QUICK_REFERENCE.md` to show auto-enable/disable behavior
- Regenerated all PDF documentation:
  - `Audio_Engine_Manual.pdf` - Enhanced user manual with volume control section
  - `Filter_Report_Enhanced.pdf` - Technical specifications report

### Technical Details

#### Non-Linear Volume Response Implementation
```c
// In main.c
static inline uint8_t ApplyVolumeResponse( uint8_t linear_volume )
{
  #ifdef VOLUME_RESPONSE_NONLINEAR
    float normalized = (float)linear_volume / 255.0f;
    float curved = powf( normalized, 1.0f / VOLUME_RESPONSE_GAMMA );
    return (uint8_t)( curved * 255.0f + 0.5f );
  #else
    return linear_volume;
  #endif
}
```

#### Digital Volume Scaling
```c
// OPT1 (bit 0), OPT2 (bit 1), OPT3 (bit 2)
// Reads 3-bit GPIO value (0–7), inverts, scales to 1–255
uint8_t v = (gpio_bits);
v = 7 - v;  // Invert so 0b000 = max
uint16_t scaled = ( (uint16_t)v * 255U ) / 7U;
return scaled ? (uint8_t)scaled : 1U;
```

#### Audio Engine Integration
In `audio_engine.c`, `ProcessNextWaveChunk()` and `ProcessNextWaveChunk_8_bit()`:
- `vol_div = AudioEngine_ReadVolume()` fetches volume with non-linear response already applied
- `ApplyVolumeSetting(sample, vol_div)` scales: `sample * vol_div / 255`

### Files Modified

- `Core/Inc/main.h` - Added `VOLUME_RESPONSE_NONLINEAR` and `VOLUME_RESPONSE_GAMMA` defines
- `Core/Src/main.c` - Added `ApplyVolumeResponse()`, fixed `ReadVolume()`
- `Core/Libraries/audio_engine.c` - Updated `SetAirEffectPresetDb()` for auto-control
- `Docs/README.md` - Updated feature list
- `Docs/AUDIO_ENGINE_MANUAL.md` - Added comprehensive volume control section
- `Docs/AIR_EFFECT_QUICK_REFERENCE.md` - Updated enable/disable examples
- `Docs/Audio_Engine_Manual.pdf` - Regenerated with volume section
- `Docs/Filter_Report_Enhanced.pdf` - Regenerated for consistency

### Backwards Compatibility

⚠️ **Breaking Change**: `ReadVolume()` range changed from 1–8 to 1–255
- Application code reading volume directly may need updating
- Audio engine itself handles the range correctly
- Recommend using `ApplyVolumeResponse()` wrapper for consistent behavior

### Performance

- Non-linear response adds one `powf()` call per DMA chunk (negligible CPU impact)
- Volume scaling already inlined in audio engine (no performance change)

### Testing Recommendations

1. Test digital volume control with all 8 GPIO combinations (OPT1–OPT3)
2. Verify audio level changes smoothly with non-linear curve enabled/disabled
3. Test air effect preset cycling (0 should disable, 1–3 should enable with presets)
4. Compare perceived loudness between linear (GAMMA=1.0) and quadratic (GAMMA=2.0)
5. Verify ADC-based analog volume control tracks properly with scaling

---

## Previous Versions

See BUG_REPORT.md for known issues and baseline feature list.
