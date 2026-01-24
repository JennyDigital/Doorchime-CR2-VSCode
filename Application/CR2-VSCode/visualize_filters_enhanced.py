#!/usr/bin/env python3
"""
Enhanced Filter Characteristics Visualization for Audio Engine
Generates comprehensive frequency response and transfer function plots for DSP filters.
"""

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.gridspec import GridSpec

# Filter coefficients from audio_engine.h
DC_FILTER_ALPHA = 64225 / 65536  # 0.98
SOFT_DC_FILTER_ALPHA = 65216 / 65536  # 0.995

# 16-bit Biquad LPF levels
LPF_16BIT_VERY_SOFT = 49152 / 65536  # 0.75
LPF_16BIT_SOFT = 32768 / 65536  # 0.5
LPF_16BIT_MEDIUM = 19660 / 65536  # 0.3
LPF_16BIT_AGGRESSIVE = 6554 / 65536  # 0.1

# 8-bit LPF levels
LPF_VERY_SOFT = 61440 / 65536  # 0.9375
LPF_SOFT = 57344 / 65536  # 0.875
LPF_MEDIUM = 49152 / 65536  # 0.75
LPF_AGGRESSIVE = 40960 / 65536  # 0.625

# Sampling frequency
FS = 22000  # Hz (default playback speed)

def dc_blocking_filter_response(alpha, fs=FS):
    """
    Calculate frequency response of DC blocking filter (high-pass).
    H(z) = (1 - z^-1) / (1 - alpha*z^-1)
    """
    frequencies = np.logspace(0, np.log10(fs/2), 1000)
    omega = 2 * np.pi * frequencies / fs
    
    # Calculate magnitude response
    z = np.exp(1j * omega)
    numerator = 1 - z**(-1)
    denominator = 1 - alpha * z**(-1)
    H = numerator / denominator
    magnitude_db = 20 * np.log10(np.abs(H))
    
    return frequencies, magnitude_db

def lpf_8bit_response(alpha, fs=FS):
    """
    Calculate frequency response of simple 1-pole LPF for 8-bit samples.
    H(z) = alpha / (1 - (1-alpha)*z^-1)
    """
    frequencies = np.logspace(0, np.log10(fs/2), 1000)
    omega = 2 * np.pi * frequencies / fs
    
    z = np.exp(1j * omega)
    numerator = alpha
    denominator = 1 - (1 - alpha) * z**(-1)
    H = numerator / denominator
    magnitude_db = 20 * np.log10(np.abs(H))
    
    return frequencies, magnitude_db

def lpf_16bit_biquad_response(alpha, fs=FS):
    """
    Calculate frequency response of biquad LPF for 16-bit samples.
    Implementation from ApplyLowPassFilter16Bit():
    b0 = ((1-alpha)^2) / 2
    b1 = 2*b0
    b2 = b0
    a1 = -2*alpha
    a2 = alpha^2
    """
    frequencies = np.logspace(0, np.log10(fs/2), 1000)
    omega = 2 * np.pi * frequencies / fs
    
    # Calculate coefficients
    b0 = ((1 - alpha)**2) / 2
    b1 = 2 * b0
    b2 = b0
    a0 = 1
    a1 = -2 * alpha
    a2 = alpha**2
    
    z = np.exp(1j * omega)
    numerator = b0 + b1 * z**(-1) + b2 * z**(-2)
    denominator = a0 + a1 * z**(-1) + a2 * z**(-2)
    H = numerator / denominator
    magnitude_db = 20 * np.log10(np.abs(H))
    phase_rad = np.angle(H)
    
    return frequencies, magnitude_db, phase_rad

def find_cutoff_frequency(frequencies, magnitude_db):
    """Find the -3dB cutoff frequency."""
    # Find where magnitude crosses -3dB
    idx = np.argmin(np.abs(magnitude_db - (-3.0)))
    return frequencies[idx]

def soft_clipping_transfer():
    """
    Generate the soft clipping transfer function.
    Uses cubic smoothstep curve above/below threshold.
    """
    threshold = 28000
    max_val = 32767
    
    input_range = np.linspace(-32768, 32767, 2000)
    output = np.zeros_like(input_range)
    
    for i, s in enumerate(input_range):
        if s > threshold:
            excess = s - threshold
            range_val = max_val - threshold
            x = excess / range_val
            if x > 1.0:
                x = 1.0
            curve = 1.5 * x**2 - x**3
            output[i] = threshold + range_val * curve
        elif s < -threshold:
            excess = -threshold - s
            range_val = max_val - threshold
            x = excess / range_val
            if x > 1.0:
                x = 1.0
            curve = 1.5 * x**2 - x**3
            output[i] = -threshold - range_val * curve
        else:
            output[i] = s
    
    return input_range, output

# Create the figure with more subplots
fig = plt.figure(figsize=(16, 12))
gs = GridSpec(4, 2, figure=fig, hspace=0.35, wspace=0.3)

# Plot 1: DC Blocking Filters
ax1 = fig.add_subplot(gs[0, 0])
freq_dc, mag_dc = dc_blocking_filter_response(DC_FILTER_ALPHA)
freq_soft_dc, mag_soft_dc = dc_blocking_filter_response(SOFT_DC_FILTER_ALPHA)

ax1.semilogx(freq_dc, mag_dc, 'b-', linewidth=2, label=f'DC Block (α={DC_FILTER_ALPHA:.4f})')
ax1.semilogx(freq_soft_dc, mag_soft_dc, 'r--', linewidth=2, label=f'Soft DC Block (α={SOFT_DC_FILTER_ALPHA:.4f})')
ax1.grid(True, alpha=0.3, which='both')
ax1.set_xlabel('Frequency (Hz)')
ax1.set_ylabel('Magnitude (dB)')
ax1.set_title('DC Blocking Filter (High-Pass)')
ax1.legend()
ax1.set_ylim([-40, 5])

# Plot 2: 16-bit Biquad LPF with all levels
ax2 = fig.add_subplot(gs[0, 1])
freq_16vs, mag_16vs, _ = lpf_16bit_biquad_response(LPF_16BIT_VERY_SOFT)
freq_16s, mag_16s, _ = lpf_16bit_biquad_response(LPF_16BIT_SOFT)
freq_16m, mag_16m, _ = lpf_16bit_biquad_response(LPF_16BIT_MEDIUM)
freq_16a, mag_16a, _ = lpf_16bit_biquad_response(LPF_16BIT_AGGRESSIVE)

ax2.semilogx(freq_16vs, mag_16vs, 'c-', linewidth=2, label=f'Very Soft (α={LPF_16BIT_VERY_SOFT:.4f})')
ax2.semilogx(freq_16s, mag_16s, 'g-', linewidth=2, label=f'Soft (α={LPF_16BIT_SOFT:.4f})')
ax2.semilogx(freq_16m, mag_16m, 'orange', linewidth=2, label=f'Medium (α={LPF_16BIT_MEDIUM:.2f})')
ax2.semilogx(freq_16a, mag_16a, 'r-', linewidth=2, label=f'Aggressive (α={LPF_16BIT_AGGRESSIVE:.3f})')
ax2.grid(True, alpha=0.3, which='both')
ax2.set_xlabel('Frequency (Hz)')
ax2.set_ylabel('Magnitude (dB)')
ax2.set_title('16-bit Biquad Low-Pass Filter (Various Levels)')
ax2.legend(fontsize=8)
ax2.set_ylim([-60, 5])
ax2.axhline(-3, color='k', linestyle='--', alpha=0.3, linewidth=1)

# Plot 3: 8-bit LPF with different aggressiveness levels
ax3 = fig.add_subplot(gs[1, 0])
freq_vs, mag_vs = lpf_8bit_response(LPF_VERY_SOFT)
freq_s, mag_s = lpf_8bit_response(LPF_SOFT)
freq_m, mag_m = lpf_8bit_response(LPF_MEDIUM)
freq_a, mag_a = lpf_8bit_response(LPF_AGGRESSIVE)

ax3.semilogx(freq_vs, mag_vs, 'c-', linewidth=2, label=f'Very Soft (α={LPF_VERY_SOFT:.4f})')
ax3.semilogx(freq_s, mag_s, 'b-', linewidth=2, label=f'Soft (α={LPF_SOFT:.4f})')
ax3.semilogx(freq_m, mag_m, 'orange', linewidth=2, label=f'Medium (α={LPF_MEDIUM:.2f})')
ax3.semilogx(freq_a, mag_a, 'r-', linewidth=2, label=f'Aggressive (α={LPF_AGGRESSIVE:.3f})')
ax3.grid(True, alpha=0.3, which='both')
ax3.set_xlabel('Frequency (Hz)')
ax3.set_ylabel('Magnitude (dB)')
ax3.set_title('8-bit Low-Pass Filter (Various Levels)')
ax3.legend(fontsize=8)
ax3.set_ylim([-40, 5])
ax3.axhline(-3, color='k', linestyle='--', alpha=0.3, linewidth=1)

# Plot 4: Soft Clipping Transfer Function
ax4 = fig.add_subplot(gs[1, 1])
input_clip, output_clip = soft_clipping_transfer()

ax4.plot(input_clip, output_clip, 'purple', linewidth=2, label='Soft Clipping')
ax4.plot([-32768, 32767], [-32768, 32767], 'k--', alpha=0.3, label='Linear (no clipping)')
ax4.axvline(28000, color='r', linestyle=':', alpha=0.5, label='Threshold')
ax4.axvline(-28000, color='r', linestyle=':', alpha=0.5)
ax4.grid(True, alpha=0.3)
ax4.set_xlabel('Input Sample Value')
ax4.set_ylabel('Output Sample Value')
ax4.set_title('Soft Clipping Transfer Function')
ax4.legend()
ax4.set_xlim([-33000, 33000])
ax4.set_ylim([-33000, 33000])

# Plot 5: Combined Frequency Response (All Filters)
ax5 = fig.add_subplot(gs[2, :])
freq_dc, mag_dc = dc_blocking_filter_response(SOFT_DC_FILTER_ALPHA)
freq_16bit, mag_16bit, _ = lpf_16bit_biquad_response(LPF_16BIT_SOFT)

# Calculate combined response (DC block + 16-bit LPF)
combined_mag = mag_dc + mag_16bit

ax5.semilogx(freq_dc, mag_dc, 'b-', linewidth=1.5, alpha=0.7, label='Soft DC Block')
ax5.semilogx(freq_16bit, mag_16bit, 'g-', linewidth=1.5, alpha=0.7, label='16-bit Biquad LPF (Soft)')
ax5.semilogx(freq_dc, combined_mag, 'r-', linewidth=2.5, label='Combined Response (DC Block + LPF)')
ax5.grid(True, alpha=0.3, which='both')
ax5.set_xlabel('Frequency (Hz)')
ax5.set_ylabel('Magnitude (dB)')
ax5.set_title('Combined Filter Chain Response (16-bit path)')
ax5.legend()
ax5.set_ylim([-80, 5])
ax5.axhline(0, color='k', linestyle='-', alpha=0.2)
ax5.axhline(-3, color='k', linestyle='--', alpha=0.2)

# Plot 6: Phase Response for 16-bit Biquad LPF
ax6 = fig.add_subplot(gs[3, 0])
freq_16vs, _, phase_16vs = lpf_16bit_biquad_response(LPF_16BIT_VERY_SOFT)
freq_16s, _, phase_16s = lpf_16bit_biquad_response(LPF_16BIT_SOFT)
freq_16m, _, phase_16m = lpf_16bit_biquad_response(LPF_16BIT_MEDIUM)
freq_16a, _, phase_16a = lpf_16bit_biquad_response(LPF_16BIT_AGGRESSIVE)

ax6.semilogx(freq_16vs, np.degrees(phase_16vs), 'c-', linewidth=2, label='Very Soft')
ax6.semilogx(freq_16s, np.degrees(phase_16s), 'g-', linewidth=2, label='Soft')
ax6.semilogx(freq_16m, np.degrees(phase_16m), 'orange', linewidth=2, label='Medium')
ax6.semilogx(freq_16a, np.degrees(phase_16a), 'r-', linewidth=2, label='Aggressive')
ax6.grid(True, alpha=0.3, which='both')
ax6.set_xlabel('Frequency (Hz)')
ax6.set_ylabel('Phase (degrees)')
ax6.set_title('16-bit Biquad LPF Phase Response')
ax6.legend(fontsize=8)

# Plot 7: Cutoff Frequency Summary
ax7 = fig.add_subplot(gs[3, 1])
ax7.axis('off')

# Calculate cutoff frequencies
cutoff_8vs = find_cutoff_frequency(*lpf_8bit_response(LPF_VERY_SOFT))
cutoff_8s = find_cutoff_frequency(*lpf_8bit_response(LPF_SOFT))
cutoff_8m = find_cutoff_frequency(*lpf_8bit_response(LPF_MEDIUM))
cutoff_8a = find_cutoff_frequency(*lpf_8bit_response(LPF_AGGRESSIVE))

freq_temp, mag_temp, _ = lpf_16bit_biquad_response(LPF_16BIT_VERY_SOFT)
cutoff_16vs = find_cutoff_frequency(freq_temp, mag_temp)
freq_temp, mag_temp, _ = lpf_16bit_biquad_response(LPF_16BIT_SOFT)
cutoff_16s = find_cutoff_frequency(freq_temp, mag_temp)
freq_temp, mag_temp, _ = lpf_16bit_biquad_response(LPF_16BIT_MEDIUM)
cutoff_16m = find_cutoff_frequency(freq_temp, mag_temp)
freq_temp, mag_temp, _ = lpf_16bit_biquad_response(LPF_16BIT_AGGRESSIVE)
cutoff_16a = find_cutoff_frequency(freq_temp, mag_temp)

info_text = f"""Filter Characteristics Summary
Sample Rate: {FS} Hz (Nyquist: {FS/2} Hz)

8-bit LPF -3dB Cutoff Frequencies:
  • Very Soft:    {cutoff_8vs:>6.0f} Hz ({cutoff_8vs/FS*100:>5.2f}% of Fs)
  • Soft:         {cutoff_8s:>6.0f} Hz ({cutoff_8s/FS*100:>5.2f}% of Fs)
  • Medium:       {cutoff_8m:>6.0f} Hz ({cutoff_8m/FS*100:>5.2f}% of Fs)
  • Aggressive:   {cutoff_8a:>6.0f} Hz ({cutoff_8a/FS*100:>5.2f}% of Fs)

16-bit Biquad LPF -3dB Cutoff Frequencies:
  • Very Soft:    {cutoff_16vs:>6.0f} Hz ({cutoff_16vs/FS*100:>5.2f}% of Fs)
  • Soft:         {cutoff_16s:>6.0f} Hz ({cutoff_16s/FS*100:>5.2f}% of Fs)
  • Medium:       {cutoff_16m:>6.0f} Hz ({cutoff_16m/FS*100:>5.2f}% of Fs)
  • Aggressive:   {cutoff_16a:>6.0f} Hz ({cutoff_16a/FS*100:>5.2f}% of Fs)

DC Blocking Filter:
  • Standard (α=0.98):   High-pass, ~44 Hz cutoff
  • Soft (α=0.995):      High-pass, ~22 Hz cutoff

Soft Clipping:
  • Threshold: ±28000 (±85% of full scale)
  • Maximum: ±32767 (16-bit signed range)
  
Filter Implementation:
  • All filters use fixed-point integer math
  • No floating-point operations (efficient for MCU)
  • Biquad filters provide steeper rolloff
  • DC blocking prevents output offset drift
"""

ax7.text(0.05, 0.95, info_text, transform=ax7.transAxes, 
         fontsize=9, verticalalignment='top', family='monospace',
         bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.3))

# Add overall title
fig.suptitle('Audio Engine DSP Filter Characteristics - Complete Analysis\nSTM32 Audio Engine @ 22 kHz Sample Rate', 
             fontsize=16, fontweight='bold', y=0.995)

# Save the figure
output_file = 'filter_characteristics_enhanced.png'
plt.savefig(output_file, dpi=300, bbox_inches='tight')
print(f"Enhanced filter characteristics plot saved to: {output_file}")

# Also display
plt.show()
