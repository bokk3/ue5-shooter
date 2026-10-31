import os

def patch_file(path, old_str, new_str):
    with open(path, "rb") as f:
        data = f.read().decode("utf-8")
    
    # Normalize line endings for replacement
    crlf = "\r\n" in data
    old_str_norm = old_str.replace("\r\n", "\n")
    data_norm = data.replace("\r\n", "\n")
    new_str_norm = new_str.replace("\r\n", "\n")

    if old_str_norm not in data_norm:
        raise ValueError(f"Target string not found in {path}")
    
    data_norm = data_norm.replace(old_str_norm, new_str_norm, 1)
    
    if crlf:
        final_data = data_norm.replace("\n", "\r\n")
    else:
        final_data = data_norm
        
    with open(path, "wb") as f:
        f.write(final_data.encode("utf-8"))
    print(f"Patched {path}")

# 1. FPSCharacter.h: Camera calibration
char_h = r"Source\NobleFermi\Character\FPSCharacter.h"
old_char_h = """\t// Tactical 3rd-Person Over-The-Shoulder (OTS) Camera Settings (Phase 8A Spec)
\tUPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Camera")
\tfloat DefaultArmLength = 210.0f;

\tUPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Camera")
\tfloat ADSArmLength = 95.0f;

\tUPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Camera")
\tFVector HipfireShoulderOffset = FVector(0.0f, 52.0f, 22.0f);

\tUPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Camera")
\tFVector ADSShoulderOffset = FVector(0.0f, 34.0f, 16.0f);

\tUPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Camera")
\tfloat DefaultFOV = 90.0f;

\tUPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Camera")
\tfloat ADSFOV = 62.0f;"""

new_char_h = """\t// Tactical 3rd-Person Over-The-Shoulder (OTS) Camera Settings (Phase 5 Spec)
\tUPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Camera")
\tfloat DefaultArmLength = 165.0f;

\tUPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Camera")
\tfloat ADSArmLength = 85.0f;

\tUPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Camera")
\tFVector HipfireShoulderOffset = FVector(0.0f, 44.0f, 18.0f);

\tUPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Camera")
\tFVector ADSShoulderOffset = FVector(0.0f, 30.0f, 14.0f);

\tUPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Camera")
\tfloat DefaultFOV = 80.0f;

\tUPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Camera")
\tfloat ADSFOV = 68.0f;"""

patch_file(char_h, old_char_h, new_char_h)

# 2. FPSCharacter.cpp: Camera probe size & Lissajous sway
char_cpp = r"Source\NobleFermi\Character\FPSCharacter.cpp"
old_probe = "CameraBoom->ProbeSize = 12.0f;"
new_probe = "CameraBoom->ProbeSize = 6.0f; // Calibrated for tight corner peeking without clipping jitter"
patch_file(char_cpp, old_probe, new_probe)

old_sway = """\t// Sway lag
\tFRotator TargetSway = FRotator(-DeltaRot.Pitch * 1.5f * ArmTraumaMultiplier, DeltaRot.Yaw * 1.5f * ArmTraumaMultiplier, DeltaRot.Yaw * 0.8f);
\tCurrentWeaponSwayRot = FMath::RInterpTo(CurrentWeaponSwayRot, TargetSway, DeltaTime, 10.0f);"""

new_sway = """\t// Procedural weapon sway: turning lag blended with figure-8 Lissajous breathing sway
\tconst float RunningTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
\tconst float AimingDampener = IsAiming() ? 0.15f : 1.0f;
\tconst float BreathPitch = FMath::Sin(RunningTime * 1.8f) * 0.28f * AimingDampener;
\tconst float BreathYaw = FMath::Cos(RunningTime * 0.9f) * 0.42f * AimingDampener;

\tFRotator TargetSway = FRotator(
\t\t(-DeltaRot.Pitch * 1.5f * ArmTraumaMultiplier * AimingDampener) + BreathPitch,
\t\t(DeltaRot.Yaw * 1.5f * ArmTraumaMultiplier * AimingDampener) + BreathYaw,
\t\tDeltaRot.Yaw * 0.8f * AimingDampener
\t);
\tCurrentWeaponSwayRot = FMath::RInterpTo(CurrentWeaponSwayRot, TargetSway, DeltaTime, 12.0f);"""

patch_file(char_cpp, old_sway, new_sway)

# 3. FPSWeaponBase.h: Second-order spring recoil vectors
weap_h = r"Source\NobleFermi\Weapons\FPSWeaponBase.h"
old_weap_state = """\t// Procedural Animation State
\tfloat CurrentADSAlpha = 0.0f;
\tFVector RecoilOffsetLoc = FVector::ZeroVector;
\tFRotator RecoilOffsetRot = FRotator::ZeroRotator;
\tFRotator CurrentSwayRot = FRotator::ZeroRotator;
\tFRotator LastControlRot = FRotator::ZeroRotator;"""

new_weap_state = """\t// Procedural Animation State & Second-Order Spring Recoil
\tfloat CurrentADSAlpha = 0.0f;
\tFVector RecoilOffsetLoc = FVector::ZeroVector;
\tFRotator RecoilOffsetRot = FRotator::ZeroRotator;
\tFVector RecoilSpringVel = FVector::ZeroVector;
\tFRotator RecoilSpringRotVel = FRotator::ZeroRotator;
\tFRotator CurrentSwayRot = FRotator::ZeroRotator;
\tFRotator LastControlRot = FRotator::ZeroRotator;"""

patch_file(weap_h, old_weap_state, new_weap_state)

# 4. FPSWeaponBase.cpp: Second-order spring integration in Tick and impulse in FireShot
weap_cpp = r"Source\NobleFermi\Weapons\FPSWeaponBase.cpp"

old_weap_recoil_tick = """\t// 3. Physical recoil kick recovery
\tRecoilOffsetLoc = FMath::VInterpTo(RecoilOffsetLoc, FVector::ZeroVector, DeltaTime, 22.0f);
\tRecoilOffsetRot = FMath::RInterpTo(RecoilOffsetRot, FRotator::ZeroRotator, DeltaTime, 22.0f);"""

new_weap_recoil_tick = """\t// 3. Physical Second-Order Spring-Damper Recoil Integration (Harmonic Oscillator)
\tconst float SpringStiffness = 185.0f;
\tconst float SpringDamping = 18.0f;

\tconst FVector LinearAccel = -SpringStiffness * RecoilOffsetLoc - SpringDamping * RecoilSpringVel;
\tRecoilSpringVel += LinearAccel * DeltaTime;
\tRecoilOffsetLoc += RecoilSpringVel * DeltaTime;

\tconst float PitchAccel = -SpringStiffness * RecoilOffsetRot.Pitch - SpringDamping * RecoilSpringRotVel.Pitch;
\tconst float YawAccel = -SpringStiffness * RecoilOffsetRot.Yaw - SpringDamping * RecoilSpringRotVel.Yaw;
\tconst float RollAccel = -SpringStiffness * RecoilOffsetRot.Roll - SpringDamping * RecoilSpringRotVel.Roll;
\tRecoilSpringRotVel += FRotator(PitchAccel, YawAccel, RollAccel) * DeltaTime;
\tRecoilOffsetRot += RecoilSpringRotVel * DeltaTime;"""

patch_file(weap_cpp, old_weap_recoil_tick, new_weap_recoil_tick)

old_weap_sway = """\t\t// Dampen weapon sway by 85% when aiming down sights
\t\tconst float SwayDampener = FMath::Lerp(1.0f, 0.15f, CurrentADSAlpha);
\t\tconst FRotator TargetSway = FRotator(-DeltaRot.Pitch * 0.7f * SwayDampener, DeltaRot.Yaw * 0.7f * SwayDampener, DeltaRot.Yaw * 0.4f * SwayDampener);
\t\tCurrentSwayRot = FMath::RInterpTo(CurrentSwayRot, TargetSway, DeltaTime, 12.0f);"""

new_weap_sway = """\t\t// Dampen weapon sway by 85% when aiming down sights and inject Lissajous breathing curve
\t\tconst float SwayDampener = FMath::Lerp(1.0f, 0.15f, CurrentADSAlpha);
\t\tconst float WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
\t\tconst float BreathPitch = FMath::Sin(WorldTime * 1.8f) * 0.22f * SwayDampener;
\t\tconst float BreathYaw = FMath::Cos(WorldTime * 0.9f) * 0.32f * SwayDampener;
\t\tconst FRotator TargetSway = FRotator(
\t\t\t(-DeltaRot.Pitch * 0.7f * SwayDampener) + BreathPitch,
\t\t\t(DeltaRot.Yaw * 0.7f * SwayDampener) + BreathYaw,
\t\t\tDeltaRot.Yaw * 0.4f * SwayDampener
\t\t);
\t\tCurrentSwayRot = FMath::RInterpTo(CurrentSwayRot, TargetSway, DeltaTime, 14.0f);"""

patch_file(weap_cpp, old_weap_sway, new_weap_sway)

old_weap_impulse = """\t\t\t// Procedural physical weapon recoil kick
\t\t\tconst float ADSRecoilDamp = bIsAimingDownSights ? 0.45f : 1.0f;
\t\t\tRecoilOffsetLoc += FVector(-1.2f * ADSRecoilDamp * RecoilMod, FMath::RandRange(-0.08f, 0.08f) * RecoilMod, 0.35f * ADSRecoilDamp * RecoilMod);
\t\t\tRecoilOffsetRot += FRotator(FMath::RandRange(1.2f, 1.8f) * ADSRecoilDamp * RecoilMod, FMath::RandRange(-0.3f, 0.3f) * RecoilMod, FMath::RandRange(-0.4f, 0.4f) * RecoilMod);"""

new_weap_impulse = """\t\t\t// Procedural physical weapon recoil kick via snappy spring impulse
\t\t\tconst float ADSRecoilDamp = bIsAimingDownSights ? 0.42f : 1.0f;
\t\t\tconst float ImpulseBackwards = -42.0f * ADSRecoilDamp * RecoilMod;
\t\t\tconst float ImpulseLateral = FMath::RandRange(-4.5f, 4.5f) * RecoilMod;
\t\t\tconst float ImpulseVertical = 11.0f * ADSRecoilDamp * RecoilMod;
\t\t\tRecoilSpringVel += FVector(ImpulseBackwards, ImpulseLateral, ImpulseVertical);

\t\t\tconst float ImpulsePitch = FMath::RandRange(60.0f, 90.0f) * ADSRecoilDamp * RecoilMod;
\t\t\tconst float ImpulseYaw = FMath::RandRange(-15.0f, 15.0f) * RecoilMod;
\t\t\tconst float ImpulseRoll = FMath::RandRange(-8.0f, 8.0f) * RecoilMod;
\t\t\tRecoilSpringRotVel += FRotator(ImpulsePitch, ImpulseYaw, ImpulseRoll);"""

patch_file(weap_cpp, old_weap_impulse, new_weap_impulse)

print("All Phase 5 changes successfully applied!")
