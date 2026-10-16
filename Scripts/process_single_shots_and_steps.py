import wave
import struct
import math
import os

def process_file(in_path, out_path, start_sec, dur_sec, fade_in_ms=5, fade_out_ms=100):
    with wave.open(in_path, 'rb') as r:
        nch = r.getnchannels()
        sr = r.getframerate()
        sw = r.getsampwidth()
        total_frames = r.getnframes()
        
        start_frame = int(start_sec * sr)
        num_frames = int(dur_sec * sr)
        if start_frame + num_frames > total_frames:
            num_frames = total_frames - start_frame
            
        r.setpos(start_frame)
        raw = r.readframes(num_frames)
        
    print(f"Processing {in_path} -> {out_path} ({num_frames/sr:.2f}s, {nch}ch, {sr}Hz)...")
    
    fade_in_frames = int((fade_in_ms / 1000.0) * sr)
    fade_out_frames = int((fade_out_ms / 1000.0) * sr)
    
    # Read samples
    samples = []
    if sw == 2:
        samples = list(struct.unpack(f'<{len(raw)//2}h', raw))
    elif sw == 3:
        samples = [int.from_bytes(raw[j:j+3], 'little', signed=True) for j in range(0, len(raw), 3)]
    elif sw == 4:
        samples = list(struct.unpack(f'<{len(raw)//4}i', raw))
        
    # Apply fade in and fade out
    total_samples = len(samples) // nch
    for i in range(total_samples):
        gain = 1.0
        if i < fade_in_frames and fade_in_frames > 0:
            gain = i / float(fade_in_frames)
        elif i >= total_samples - fade_out_frames and fade_out_frames > 0:
            remaining = total_samples - i
            gain = remaining / float(fade_out_frames)
            # exponential smoothing
            gain = gain * gain
            
        for c in range(nch):
            idx = i * nch + c
            samples[idx] = int(samples[idx] * gain)
            
    # Pack back
    out_raw = bytearray()
    if sw == 2:
        out_raw = struct.pack(f'<{len(samples)}h', *samples)
    elif sw == 3:
        for s in samples:
            # clamp to 24-bit
            s = max(-8388608, min(8388607, s))
            out_raw.extend(s.to_bytes(3, 'little', signed=True))
    elif sw == 4:
        out_raw = struct.pack(f'<{len(samples)}i', *samples)
        
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    with wave.open(out_path, 'wb') as w:
        w.setnchannels(nch)
        w.setsampwidth(sw)
        w.setframerate(sr)
        w.writeframes(out_raw)
    print(f"  [OK] Saved {out_path} ({os.path.getsize(out_path):,} bytes)")

def main():
    # 1. Carbine 5.56 Single Shot (authentic H&K 416 London warehouse recording)
    carbine_in = 'Content/Audio/Weapons/Fire/A_Carbine_556_Single_Shot.wav'
    if os.path.exists(carbine_in):
        # Backup original
        orig_backup = 'Content/Audio/Weapons/Fire/A_Carbine_556_Single_Shot_FullTake.wav'
        if not os.path.exists(orig_backup):
            os.rename(carbine_in, orig_backup)
            process_file(orig_backup, carbine_in, start_sec=3.807, dur_sec=1.8, fade_in_ms=3, fade_out_ms=180)

    # 2. Shotgun Single Shot
    shotgun_in = 'Content/Audio/Weapons/Fire/A_Shotgun_Single_Shot.wav'
    if os.path.exists(shotgun_in):
        shotgun_backup = 'Content/Audio/Weapons/Fire/A_Shotgun_Single_Shot_FullTake.wav'
        if not os.path.exists(shotgun_backup):
            os.rename(shotgun_in, shotgun_backup)
            process_file(shotgun_backup, shotgun_in, start_sec=0.4625, dur_sec=1.8, fade_in_ms=2, fade_out_ms=150)

    # 3. Water puddle footsteps
    water_in = 'Content/Audio/Footsteps/Water/FS_Water_Puddle_01.wav'
    if os.path.exists(water_in):
        process_file(water_in, 'Content/Audio/Footsteps/Water/FS_Water_01.wav', start_sec=0.20, dur_sec=0.65, fade_in_ms=5, fade_out_ms=60)
        process_file(water_in, 'Content/Audio/Footsteps/Water/FS_Water_02.wav', start_sec=0.85, dur_sec=0.65, fade_in_ms=5, fade_out_ms=60)
        process_file(water_in, 'Content/Audio/Footsteps/Water/FS_Water_03.wav', start_sec=1.55, dur_sec=0.65, fade_in_ms=5, fade_out_ms=60)

    print("\nAudio processing and trimming complete!")

if __name__ == '__main__':
    main()
