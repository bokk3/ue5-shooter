import os
import numpy as np
from scipy.io import wavfile
from scipy import signal

# Target parameters
TARGET_SR = 44100
TOTAL_DURATION = 4.15
TOTAL_SAMPLES = int(TOTAL_DURATION * TARGET_SR)

stems_dir = r"C:\Users\Boris\.gemini\antigravity\brain\c135ce77-4bef-4380-b69a-99d73fc7b5a7\scratch\audio_downloads"

def load_resample(filename, target_sr=TARGET_SR):
    path = os.path.join(stems_dir, filename)
    sr, data = wavfile.read(path)
    # Convert to float [-1.0, 1.0]
    if data.dtype == np.int16:
        data = data.astype(np.float32) / 32768.0
    elif data.dtype == np.int32:
        data = data.astype(np.float32) / 2147483648.0
    elif data.dtype == np.float32:
        data = data.astype(np.float32)
    
    # Ensure 2D stereo (samples, 2)
    if data.ndim == 1:
        data = np.column_stack((data, data))
    
    # Resample to target_sr if needed
    if sr != target_sr:
        num_target = int(len(data) * target_sr / sr)
        data = signal.resample(data, num_target, axis=0)
    
    return data

print("Loading audio stems...")
sub_drone = load_resample("bass_bed_heavy_001.wav")
flyby1 = load_resample("bullet_flyby_fast_05.wav")
flyby2 = load_resample("bullet_flyby_fast_06.wav")
blade_gleam = load_resample("Sword Ring Scrape Gleam 02.wav")
sword_scrapes = load_resample("Sword Scrapes Multiple, Battle, Metal, Slash, Draw.wav")

# Master stereo canvas (left, right)
master = np.zeros((TOTAL_SAMPLES, 2), dtype=np.float32)

# --- 1. TRACK 1: SUB-DRONE BED (0.0s to 4.15s) ---
# Low-pass filter the drone below 220Hz for massive cinematic weight
b, a = signal.butter(4, 220.0 / (TARGET_SR / 2), btype='low')
drone_filtered = signal.filtfilt(b, a, sub_drone[:TOTAL_SAMPLES], axis=0)
# Envelope: 0.3s fade-in, swell at 2.4s (flash), 0.7s fade-out at end
t_arr = np.linspace(0, TOTAL_DURATION, TOTAL_SAMPLES, endpoint=False)
drone_env = np.ones(TOTAL_SAMPLES, dtype=np.float32)
fade_in_samples = int(0.35 * TARGET_SR)
drone_env[:fade_in_samples] = np.linspace(0, 1.0, fade_in_samples) ** 2
fade_out_start = int(3.40 * TARGET_SR)
drone_env[fade_out_start:] = np.linspace(1.0, 0.0, TOTAL_SAMPLES - fade_out_start) ** 2
# Swell slightly during thermal flash
flash_center = 2.40
flash_width = 0.40
swell = 0.35 * np.exp(-((t_arr - flash_center) / flash_width) ** 2)
drone_env = (drone_env + swell.astype(np.float32))

drone_gain = 0.50
master += drone_filtered * drone_env[:, None] * drone_gain

# --- 2. TRACK 2: SWORD SCRAPING / METAL FRICTION (0.05s to 2.40s) ---
# High-frequency blade scrape ("tssscccchiiiiiiingggggg")
scrape_len = int(2.35 * TARGET_SR)
scrape_start = int(0.05 * TARGET_SR)
scrape_segment = sword_scrapes[:scrape_len].copy()

# High-pass filter above 1200Hz to isolate crisp singing steel friction
b_hp, a_hp = signal.butter(3, 1100.0 / (TARGET_SR / 2), btype='high')
scrape_hp = signal.filtfilt(b_hp, a_hp, scrape_segment, axis=0)

# Gleam ring harmonic layer
gleam_len = min(scrape_len, len(blade_gleam))
gleam_segment = blade_gleam[:gleam_len].copy()

# Scrape envelope: progressive buildup ramping up into the cut completion at 2.40s
ramp_env = (np.linspace(0.15, 1.0, scrape_len) ** 1.3).astype(np.float32)
# Slight stereo widening: delay right channel by 8ms
widened_scrape = np.zeros_like(scrape_hp)
widened_scrape[:, 0] = scrape_hp[:, 0] * 1.05
delay_samples = int(0.008 * TARGET_SR)
widened_scrape[delay_samples:, 1] = scrape_hp[:-delay_samples, 1] * 0.95

# Combine gritty scrape + metallic gleam ring
metal_cut = (widened_scrape * 0.58 + gleam_segment[:scrape_len] * 0.42) * ramp_env[:, None]
# Cutoff right before thermal flash with quick 25ms duck
duck_len = int(0.025 * TARGET_SR)
metal_cut[-duck_len:] *= np.linspace(1.0, 0.0, duck_len)[:, None]
master[scrape_start:scrape_start + scrape_len] += metal_cut * 0.72

# --- 3. TRACK 3: THERMAL FLASH & BLADE CLASH IMPACT (2.40s) ---
# Instant steel sword clash impact ("CLANG-SHIIING")
flash_idx = int(2.40 * TARGET_SR)
# Extract impact transient from blade_gleam / sword clash
impact_stem = blade_gleam.copy()
impact_len = min(int(1.40 * TARGET_SR), len(impact_stem))
impact_segment = impact_stem[:impact_len].copy()
# Boost impact transient at 0-80ms
impact_env = np.exp(-np.linspace(0, 3.8, impact_len)).astype(np.float32)
impact_sound = impact_segment * impact_env[:, None] * 1.15

# Add deep sub-bass thump on impact (synthetic 55Hz kick with pitch bend for physical blast)
kick_t = np.linspace(0, 0.45, int(0.45 * TARGET_SR))
kick_freq = 68.0 * np.exp(-kick_t * 9.0) + 38.0
kick_wave = np.sin(2.0 * np.pi * np.cumsum(kick_freq) / TARGET_SR) * np.exp(-kick_t * 7.5)
kick_stereo = np.column_stack((kick_wave, kick_wave)) * 0.70

master[flash_idx:flash_idx + impact_len] += impact_sound
master[flash_idx:flash_idx + len(kick_stereo)] += kick_stereo

# --- 4. TRACK 4: WIDE STEREO BULLET FLYBYS (2.68s & 2.98s) ---
# Bullet Flyby 1: Cracks Left -> Center (2.68s)
fb1_idx = int(2.68 * TARGET_SR)
fb1_len = min(len(flyby1), TOTAL_SAMPLES - fb1_idx)
fb1_seg = flyby1[:fb1_len].copy()
# Panning trajectory: -0.85 (Left) to +0.10 (Center-Right)
pan1 = np.linspace(-0.85, 0.10, fb1_len)
pan1_l = np.cos((pan1 + 1.0) * np.pi / 4.0)
pan1_r = np.sin((pan1 + 1.0) * np.pi / 4.0)
fb1_panned = np.zeros_like(fb1_seg)
fb1_panned[:, 0] = fb1_seg[:, 0] * pan1_l
fb1_panned[:, 1] = fb1_seg[:, 1] * pan1_r
master[fb1_idx:fb1_idx + fb1_len] += fb1_panned * 0.88

# Bullet Flyby 2: Whips Center -> Hard Right (2.98s)
fb2_idx = int(2.98 * TARGET_SR)
fb2_len = min(len(flyby2), TOTAL_SAMPLES - fb2_idx)
fb2_seg = flyby2[:fb2_len].copy()
# Panning trajectory: -0.15 (Center-Left) to +0.92 (Hard Right)
pan2 = np.linspace(-0.15, 0.92, fb2_len)
pan2_l = np.cos((pan2 + 1.0) * np.pi / 4.0)
pan2_r = np.sin((pan2 + 1.0) * np.pi / 4.0)
fb2_panned = np.zeros_like(fb2_seg)
fb2_panned[:, 0] = fb2_seg[:, 0] * pan2_l
fb2_panned[:, 1] = fb2_seg[:, 1] * pan2_r
master[fb2_idx:fb2_idx + fb2_len] += fb2_panned * 0.92

# --- 5. SPATIAL REVERB & STEREO PING-PONG DELAY FOR BULLETS & CLANG ---
# Create lush stereo diffuse reflections for the flybys and flash clang
delay_1 = int(0.072 * TARGET_SR) # 72ms left reflection
delay_2 = int(0.135 * TARGET_SR) # 135ms right reflection
delay_3 = int(0.240 * TARGET_SR) # 240ms cross reflection
reverb_tail = np.zeros_like(master)
reverb_tail[delay_1:, 0] += master[:-delay_1, 1] * 0.22
reverb_tail[delay_2:, 1] += master[:-delay_2, 0] * 0.18
reverb_tail[delay_3:, 0] += master[:-delay_3, 0] * 0.12
reverb_tail[delay_3:, 1] += master[:-delay_3, 1] * 0.12
master += reverb_tail

# Clean fadeout at the end (3.80s to 4.15s)
tail_fade_start = int(3.80 * TARGET_SR)
master[tail_fade_start:] *= np.linspace(1.0, 0.0, TOTAL_SAMPLES - tail_fade_start)[:, None] ** 2

# --- 6. MASTERING LIMITER & PEAK NORMALIZATION ---
# Soft saturation clipper to prevent harsh digital clipping while maintaining punch
peak_val = np.max(np.abs(master))
print(f"Raw mix peak: {peak_val:.3f} ({20*np.log10(peak_val + 1e-9):.1f} dBFS)")

# Soft tanh saturation limiter
master = np.tanh(master * 1.12) / 1.12
target_peak = 0.944 # -0.5 dBFS
current_peak = np.max(np.abs(master))
master = master * (target_peak / current_peak)
print(f"Mastered peak: {np.max(np.abs(master)):.3f} ({20*np.log10(np.max(np.abs(master))):.1f} dBFS)")

# Convert to 16-bit integer PCM
master_16bit = np.clip(master * 32767.0, -32768.0, 32767.0).astype(np.int16)

# Save destinations
out_paths = [
    r"Content\Splash\tachyon_splash.wav",
    r"Content\Audio\Splash\A_Tachyon_Splash_Cinematic_Stereo.wav",
    r"Packaged\Windows\NobleFermi\Content\Splash\tachyon_splash.wav",
    r"Packaged\Windows\NobleFermi\Content\Audio\Splash\A_Tachyon_Splash_Cinematic_Stereo.wav"
]

for out_p in out_paths:
    os.makedirs(os.path.dirname(out_p), exist_ok=True)
    wavfile.write(out_p, TARGET_SR, master_16bit)
    print(f"Saved master WAV to: {out_p} ({os.path.getsize(out_p)} bytes, {TOTAL_DURATION}s)")

print("Mastering complete!")
