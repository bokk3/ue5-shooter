import os

path = "Source/NobleFermi/Character/FPSAnimInstance.cpp"
with open(path, "r", encoding="utf-8") as f:
    content = f.read()

# 1. Update ApplyRestingIdleAndLocomotion:
# Enhanced weight-transfer, grounded athletic contrapposto, natural right-leg stance flare
old_locomotion = """\t// Subtle lateral shift (±1.35 cm along CompRightDir) and tiny vertical breathing/weight settle (±0.45 cm)
\tPelvisPos += CompRightDir * (WeightSine * 1.35f * IdleWeightAlpha);
\tPelvisPos += CompUpDir * (-0.35f + FMath::Cos(WeightTransferPhase * 2.0f) * 0.35f) * IdleWeightAlpha;

\t// Subtle contrapposto hip roll (±1.4 deg around CompForwardDir) and natural tactical hip angle (+6 deg yaw)
\tconst FQuat HipRollQ(CompForwardDir, FMath::DegreesToRadians(WeightSine * 1.4f * IdleWeightAlpha));
\tconst FQuat StanceYawQ(CompUpDir, FMath::DegreesToRadians(5.0f * (1.0f - SmoothADSAlpha * 0.5f) * IdleWeightAlpha));
\tPelvisRot = (StanceYawQ * HipRollQ * PelvisRot).GetNormalized();"""

new_locomotion = """\t// Lifelike weight-transfer (±1.65 cm lateral sway) with grounding settle
\tconst float BreathSinVal = FMath::Sin(IdleBreathPhase);
\tPelvisPos += CompRightDir * (WeightSine * 1.65f * IdleWeightAlpha);
\tPelvisPos += CompUpDir * (-0.35f + FMath::Cos(WeightTransferPhase * 2.0f) * 0.35f + BreathSinVal * 0.30f) * IdleWeightAlpha;

\t// Natural contrapposto hip roll (±1.8 deg) and grounded tactical stance yaw
\tconst FQuat HipRollQ(CompForwardDir, FMath::DegreesToRadians(WeightSine * 1.8f * IdleWeightAlpha));
\tconst FQuat StanceYawQ(CompUpDir, FMath::DegreesToRadians(5.5f * (1.0f - SmoothADSAlpha * 0.5f) * IdleWeightAlpha));
\tPelvisRot = (StanceYawQ * HipRollQ * PelvisRot).GetNormalized();"""

if old_locomotion in content:
    content = content.replace(old_locomotion, new_locomotion)
    print("Replaced old_locomotion")
else:
    print("old_locomotion not found")

# Leg deltas: add relaxed right-leg flare
old_legs = """\tconst FQuat ThighRDelta = FQuat(CompRightDir, WalkHipPitchR) * ThighWeightDelta;
\tconst FQuat CalfRDelta  = FQuat(CompRightDir, WalkKneeFlexR);
\tconst FQuat FootRDelta  = FQuat(CompRightDir, -(WalkHipPitchR + WalkKneeFlexR) * 0.65f);"""

new_legs = """\t// Right leg: natural relaxed resting flare (+7.5 deg yaw, -4.0 deg knee ease) when stationary
\tconst FQuat RightStanceFlare = FQuat(CompUpDir, FMath::DegreesToRadians(7.5f * IdleWeightAlpha));
\tconst FQuat RightKneeEase    = FQuat(CompRightDir, FMath::DegreesToRadians(-4.0f * IdleWeightAlpha));

\tconst FQuat ThighRDelta = FQuat(CompRightDir, WalkHipPitchR) * ThighWeightDelta * RightStanceFlare;
\tconst FQuat CalfRDelta  = FQuat(CompRightDir, WalkKneeFlexR) * RightKneeEase;
\tconst FQuat FootRDelta  = FQuat(CompRightDir, -(WalkHipPitchR + WalkKneeFlexR) * 0.65f) * FQuat(CompUpDir, FMath::DegreesToRadians(-3.5f * IdleWeightAlpha));"""

if old_legs in content:
    content = content.replace(old_legs, new_legs)
    print("Replaced old_legs")
else:
    print("old_legs not found")

# 2. Update ApplySpineBreathingAndHeadLook:
# Lifelike thoracic chest expansion (+3.4 deg pitch, +0.75cm forward/up) and clavicle lift (+2.6 deg)
old_spine = """\t// 1. Thoracic Breathing (0.32 Hz sinusoidal chest expansion)
\tconst float BreathSin = FMath::Sin(IdleBreathPhase);
\tconst float BreathPitchDeg = BreathSin * 1.35f; // Gentle ribcage lift/settle

\t// 2. Aim Pitch & Yaw + Counter-rotation for Pelvis Stance Yaw
\tconst float EffectivePitch = FMath::Clamp(AimPitch, -75.0f, 75.0f);
\tconst float EffectiveYaw = FMath::Clamp(AimYaw, -75.0f, 75.0f);
\tconst float RecoilKickDeg = -RecoilImpulseAlpha * 4.5f;

\t// Counter the +5 deg pelvis yaw across the spine so the chest faces toward the target
\tconst float SpineCounterYawDeg = -1.6f * (1.0f - SmoothADSAlpha * 0.5f);

\tconst FQuat Spine01Delta =
\t\tFQuat(CompUpDir, FMath::DegreesToRadians(EffectiveYaw * 0.22f + SpineCounterYawDeg)) *
\t\tFQuat(CompRightDir, FMath::DegreesToRadians(EffectivePitch * 0.18f + BreathPitchDeg * 0.25f));

\tconst FQuat Spine02Delta =
\t\tFQuat(CompUpDir, FMath::DegreesToRadians(EffectiveYaw * 0.28f + SpineCounterYawDeg)) *
\t\tFQuat(CompRightDir, FMath::DegreesToRadians(EffectivePitch * 0.26f + BreathPitchDeg * 0.45f + RecoilKickDeg * 0.4f));

\tconst FQuat Spine03Delta =
\t\tFQuat(CompUpDir, FMath::DegreesToRadians(EffectiveYaw * 0.30f + SpineCounterYawDeg)) *
\t\tFQuat(CompRightDir, FMath::DegreesToRadians(EffectivePitch * 0.32f + BreathPitchDeg * 0.65f + RecoilKickDeg * 0.6f));

\t// Propagate Spine01 -> Spine02 -> Spine03 with accurate child position updates
\tPropagateSingleChildBoneCS(CSPose, RestCompBases, PelvisIdx, Spine01Idx, Spine01Delta);
\tPropagateSingleChildBoneCS(CSPose, RestCompBases, Spine01Idx, Spine02Idx, Spine02Delta);
\tPropagateSingleChildBoneCS(CSPose, RestCompBases, Spine02Idx, Spine03Idx, Spine03Delta);

\t// 3. Clavicle Breathing Micro-Lift
\tconst float ClavicleLiftRad = FMath::DegreesToRadians(BreathSin * 0.95f);
\tconst FQuat ClavLDelta(CompForwardDir, -ClavicleLiftRad);
\tconst FQuat ClavRDelta(CompForwardDir, ClavicleLiftRad);"""

new_spine = """\t// 1. Thoracic Breathing: 0.32 Hz deep tactical breathing with noticeable chest rise/fall
\tconst float BreathSin = FMath::Sin(IdleBreathPhase);
\tconst float BreathPitchDeg = BreathSin * 3.4f; // Lifelike chest lift & expansion on inhale

\t// 2. Aim Pitch & Yaw + Counter-rotation for Pelvis Stance Yaw
\tconst float EffectivePitch = FMath::Clamp(AimPitch, -75.0f, 75.0f);
\tconst float EffectiveYaw = FMath::Clamp(AimYaw, -75.0f, 75.0f);
\tconst float RecoilKickDeg = -RecoilImpulseAlpha * 4.5f;

\t// Counter the +5 deg pelvis yaw across the spine so the chest faces toward the target
\tconst float SpineCounterYawDeg = -1.6f * (1.0f - SmoothADSAlpha * 0.5f);

\tconst FQuat Spine01Delta =
\t\tFQuat(CompUpDir, FMath::DegreesToRadians(EffectiveYaw * 0.22f + SpineCounterYawDeg)) *
\t\tFQuat(CompRightDir, FMath::DegreesToRadians(EffectivePitch * 0.18f + BreathPitchDeg * 0.25f));

\tconst FQuat Spine02Delta =
\t\tFQuat(CompUpDir, FMath::DegreesToRadians(EffectiveYaw * 0.28f + SpineCounterYawDeg)) *
\t\tFQuat(CompRightDir, FMath::DegreesToRadians(EffectivePitch * 0.26f + BreathPitchDeg * 0.50f + RecoilKickDeg * 0.4f));

\tconst FQuat Spine03Delta =
\t\tFQuat(CompUpDir, FMath::DegreesToRadians(EffectiveYaw * 0.30f + SpineCounterYawDeg)) *
\t\tFQuat(CompRightDir, FMath::DegreesToRadians(EffectivePitch * 0.32f + BreathPitchDeg * 0.75f + RecoilKickDeg * 0.6f));

\t// Propagate Spine01 -> Spine02 -> Spine03 with accurate child position updates
\tPropagateSingleChildBoneCS(CSPose, RestCompBases, PelvisIdx, Spine01Idx, Spine01Delta);
\tPropagateSingleChildBoneCS(CSPose, RestCompBases, Spine01Idx, Spine02Idx, Spine02Delta);
\tPropagateSingleChildBoneCS(CSPose, RestCompBases, Spine02Idx, Spine03Idx, Spine03Delta);

\t// Anatomical chest expansion translation: ribcage expands up (+0.70cm) and forward (+0.55cm) on deep inhale
\tFTransform ExpandedSpine03CS = CSPose.Pose.GetComponentSpaceTransform(Spine03Idx);
\tExpandedSpine03CS.AddToTranslation(CompUpDir * (BreathSin * 0.70f) + CompForwardDir * (BreathSin * 0.55f));
\tCSPose.Pose.SetComponentSpaceTransform(Spine03Idx, ExpandedSpine03CS);

\t// 3. Clavicle Breathing Lift: shoulders rise +2.6 deg on inhale, drop on exhale
\tconst float ClavicleLiftRad = FMath::DegreesToRadians(BreathSin * 2.6f);
\tconst FQuat ClavLDelta(CompForwardDir, -ClavicleLiftRad);
\tconst FQuat ClavRDelta(CompForwardDir, ClavicleLiftRad);"""

if old_spine in content:
    content = content.replace(old_spine, new_spine)
    print("Replaced old_spine")
else:
    print("old_spine not found")

# Head look breathing nod
old_look = """\tconst float LookPitchDeg = (FMath::Cos(HeadLookTime * 0.85f) * 1.6f) * IdleLookScale;"""
new_look = """\tconst float LookPitchDeg = (FMath::Cos(HeadLookTime * 0.85f) * 1.6f + BreathSin * 1.2f) * IdleLookScale;"""

if old_look in content:
    content = content.replace(old_look, new_look)
    print("Replaced old_look")
else:
    print("old_look not found")

# 3. Update PropagateHandChildBones:
# Natural resting finger curl when unarmed
old_finger = """\t// Propagate all descendant bones of HandBoneIdx (e.g. thumb/index/middle/ring/pinky fingers and weapon_r)
\t// so fingers curl naturally with the palm instead of stretching back to the rest pose.
\tconst int32 HandSkelIdx = BoneContainer.GetSkeletonIndex(HandBoneIdx);
\tfor (int32 SkelIdx = HandSkelIdx + 1; SkelIdx < NumBones; ++SkelIdx)
\t{
\t\tif (RefSkel.BoneIsChildOf(SkelIdx, HandSkelIdx))
\t\t{
\t\t\tconst FCompactPoseBoneIndex ChildCompactIdx = BoneContainer.GetCompactPoseIndexFromSkeletonIndex(SkelIdx);
\t\t\tif (ChildCompactIdx != INDEX_NONE && RestCompBases.IsValidIndex(ChildCompactIdx.GetInt()))
\t\t\t{
\t\t\t\tconst FTransform LocalToHand = RestCompBases[ChildCompactIdx.GetInt()].GetRelativeTransform(RestHandCS);
\t\t\t\tCSPose.Pose.SetComponentSpaceTransform(ChildCompactIdx, LocalToHand * NewHandCS);
\t\t\t}
\t\t}
\t}"""

new_finger = """\t// Propagate all descendant bones of HandBoneIdx (e.g. thumb/index/middle/ring/pinky fingers and weapon_r)
\t// When unarmed at rest, curl fingers naturally into the palm instead of stiff open T-pose!
\tconst int32 HandSkelIdx = BoneContainer.GetSkeletonIndex(HandBoneIdx);
\tconst bool bIsLeft = (HandBoneIdx == HandLIdx);
\tconst float CurlSign = bIsLeft ? -1.0f : 1.0f;
\tconst float RelaxedCurlDeg = (bUnarmedRelaxedIdle && HighReadyAlpha < 0.05f) ? (22.0f * CurlSign) : 0.0f;
\tconst FQuat FingerCurlRot = FQuat(FVector::UpVector, FMath::DegreesToRadians(RelaxedCurlDeg));

\tfor (int32 SkelIdx = HandSkelIdx + 1; SkelIdx < NumBones; ++SkelIdx)
\t{
\t\tif (RefSkel.BoneIsChildOf(SkelIdx, HandSkelIdx))
\t\t{
\t\t\tconst FCompactPoseBoneIndex ChildCompactIdx = BoneContainer.GetCompactPoseIndexFromSkeletonIndex(SkelIdx);
\t\t\tif (ChildCompactIdx != INDEX_NONE && RestCompBases.IsValidIndex(ChildCompactIdx.GetInt()))
\t\t\t{
\t\t\t\tFTransform LocalToHand = RestCompBases[ChildCompactIdx.GetInt()].GetRelativeTransform(RestHandCS);
\t\t\t\tif (FMath::Abs(RelaxedCurlDeg) > 0.01f)
\t\t\t\t{
\t\t\t\t\tLocalToHand.SetRotation((LocalToHand.GetRotation() * FingerCurlRot).GetNormalized());
\t\t\t\t}
\t\t\t\tCSPose.Pose.SetComponentSpaceTransform(ChildCompactIdx, LocalToHand * NewHandCS);
\t\t\t}
\t\t}
\t}"""

if old_finger in content:
    content = content.replace(old_finger, new_finger)
    print("Replaced old_finger")
else:
    print("old_finger not found")

# 4. Update SolveRightArmRifleIK:
# Natural resting Two-Bone IK when unarmed
old_arm_r = """\t// When in Unarmed Relaxed Tactical Idle (Hangar default Stance 0 or Patrol Walk Stance 2),
\t// preserve the baked Mixamo/Vanguard arm & shoulder-pauldron rest pose and apply smooth breathing + walk swing deltas!
\tif (bUnarmedRelaxedIdle && HighReadyAlpha < 0.02f && !bIsReloading)
\t{
\t\tconst float BreathRad = FMath::DegreesToRadians(FMath::Sin(IdleBreathPhase) * 0.85f);
\t\tconst float WalkSwingRad = FMath::DegreesToRadians(FMath::Sin(LocomotionCyclePhase) * 16.0f * WalkMoveAlpha);
\t\tconst float ElbowFlexRad = FMath::DegreesToRadians(FMath::Max(0.0f, FMath::Sin(LocomotionCyclePhase)) * 10.0f * WalkMoveAlpha);

\t\tconst FQuat UpperDelta = FQuat(CompRightDir, WalkSwingRad) * FQuat(CompForwardDir, BreathRad);
\t\tconst FQuat LowerDelta = FQuat(CompRightDir, ElbowFlexRad);

\t\tPropagateSingleChildBoneCS(CSPose, RestCompBases, ClavicleRIdx, UpperArmRIdx, UpperDelta);
\t\tPropagateSingleChildBoneCS(CSPose, RestCompBases, UpperArmRIdx, LowerArmRIdx, LowerDelta);
\t\tPropagateSingleChildBoneCS(CSPose, RestCompBases, LowerArmRIdx, HandRIdx);
\t\tconst FTransform HandRCS = CSPose.Pose.GetComponentSpaceTransform(HandRIdx);
\t\tPropagateHandChildBones(CSPose, RestCompBases, HandRIdx, HandRCS);
\t\treturn;
\t}

\tconst FTransform Spine03CS = CSPose.Pose.GetComponentSpaceTransform(Spine03Idx);
\tconst FVector ChestPos = Spine03CS.GetLocation();
\tconst float BreathOffset = FMath::Sin(IdleBreathPhase) * 0.45f;
\tconst float RecoilBack = RecoilImpulseAlpha * 2.8f;

\t// Unarmed Relaxed Tactical Readiness (from rest pose hand location)
\tconst FVector RestHandRPos = CSPose.Pose.GetComponentSpaceTransform(HandRIdx).GetLocation();

\t// Low-Ready Patrol Cradle: pistol grip held comfortably near right ribcage/pecs
\tconst FVector LowReadyHandR =
\t\tChestPos
\t\t+ CompRightDir * 13.5f
\t\t+ CompForwardDir * 21.0f
\t\t+ CompUpDir * (-7.5f + BreathOffset);

\t// High-Ready / Combat Ready: pistol grip raised toward shoulder pocket
\tconst FVector HighReadyHandR =
\t\tChestPos
\t\t+ CompRightDir * (11.0f - SmoothADSAlpha * 4.5f)
\t\t+ CompForwardDir * (24.0f - SmoothADSAlpha * 3.0f - RecoilBack)
\t\t+ CompUpDir * (4.5f + SmoothADSAlpha * 3.5f + BreathOffset * 0.4f);

\tFVector TargetHandR = bUnarmedRelaxedIdle
\t\t? FMath::Lerp(RestHandRPos, HighReadyHandR, HighReadyAlpha)
\t\t: FMath::Lerp(LowReadyHandR, HighReadyHandR, HighReadyAlpha);

\t// During reload, tilt the rifle slightly inward so the magwell faces the left hand
\tif (bIsReloading)
\t{
\t\tTargetHandR = ChestPos + CompRightDir * 11.5f + CompForwardDir * 22.0f + CompUpDir * (-2.0f + BreathOffset * 0.5f);
\t}

\t// Elbow pole target: tucked naturally down, right, and behind the chest
\tconst FVector JointTargetR =
\t\tChestPos
\t\t+ CompRightDir * 30.0f
\t\t- CompForwardDir * 12.0f
\t\t- CompUpDir * 20.0f;"""

new_arm_r = """\tconst FTransform Spine03CS = CSPose.Pose.GetComponentSpaceTransform(Spine03Idx);
\tconst FVector ChestPos = Spine03CS.GetLocation();
\tconst float BreathOffset = FMath::Sin(IdleBreathPhase) * 0.45f;
\tconst float RecoilBack = RecoilImpulseAlpha * 2.8f;

\t// Natural Relaxed Resting Hand (beside outer thigh, natural ~22 deg elbow bend, breath rise/fall)
\tconst FVector RelaxedHandR =
\t\tChestPos
\t\t+ CompRightDir * 19.5f
\t\t+ CompForwardDir * 6.5f
\t\t- CompUpDir * 42.0f
\t\t+ CompUpDir * (BreathOffset * 0.85f)
\t\t+ CompForwardDir * (BreathOffset * 0.55f + FMath::Sin(LocomotionCyclePhase) * 16.0f * WalkMoveAlpha);

\tconst FVector RelaxedJointR =
\t\tChestPos
\t\t+ CompRightDir * 25.5f
\t\t- CompForwardDir * 7.0f
\t\t- CompUpDir * 19.0f;

\t// Low-Ready Patrol Cradle: pistol grip held comfortably near right ribcage/pecs
\tconst FVector LowReadyHandR =
\t\tChestPos
\t\t+ CompRightDir * 13.5f
\t\t+ CompForwardDir * 21.0f
\t\t+ CompUpDir * (-7.5f + BreathOffset);

\t// High-Ready / Combat Ready: pistol grip raised toward shoulder pocket
\tconst FVector HighReadyHandR =
\t\tChestPos
\t\t+ CompRightDir * (11.0f - SmoothADSAlpha * 4.5f)
\t\t+ CompForwardDir * (24.0f - SmoothADSAlpha * 3.0f - RecoilBack)
\t\t+ CompUpDir * (4.5f + SmoothADSAlpha * 3.5f + BreathOffset * 0.4f);

\tconst FVector ArmedJointR =
\t\tChestPos
\t\t+ CompRightDir * 30.0f
\t\t- CompForwardDir * 12.0f
\t\t- CompUpDir * 20.0f;

\tFVector TargetHandR = bUnarmedRelaxedIdle
\t\t? FMath::Lerp(RelaxedHandR, HighReadyHandR, HighReadyAlpha)
\t\t: FMath::Lerp(LowReadyHandR, HighReadyHandR, HighReadyAlpha);

\tFVector JointTargetR = bUnarmedRelaxedIdle
\t\t? FMath::Lerp(RelaxedJointR, ArmedJointR, HighReadyAlpha)
\t\t: ArmedJointR;

\t// During reload, tilt the rifle slightly inward so the magwell faces the left hand
\tif (bIsReloading)
\t{
\t\tTargetHandR = ChestPos + CompRightDir * 11.5f + CompForwardDir * 22.0f + CompUpDir * (-2.0f + BreathOffset * 0.5f);
\t}"""

if old_arm_r in content:
    content = content.replace(old_arm_r, new_arm_r)
    print("Replaced old_arm_r")
else:
    print("old_arm_r not found")

# 5. Update SolveLeftHandTwoBoneIK:
# Natural resting Two-Bone IK for left arm
old_arm_l = """\t// When in Unarmed Relaxed Tactical Idle (Hangar default Stance 0 or Patrol Walk Stance 2),
\t// preserve the baked Mixamo/Vanguard arm & shoulder-pauldron rest pose and apply smooth breathing + walk swing deltas!
\tif (bUnarmedRelaxedIdle && HighReadyAlpha < 0.02f)
\t{
\t\tconst float BreathRad = FMath::DegreesToRadians(FMath::Sin(IdleBreathPhase) * 0.85f);
\t\tconst float WalkSwingRad = FMath::DegreesToRadians(FMath::Sin(LocomotionCyclePhase + PI) * 16.0f * WalkMoveAlpha);
\t\tconst float ElbowFlexRad = FMath::DegreesToRadians(FMath::Max(0.0f, FMath::Sin(LocomotionCyclePhase + PI)) * 10.0f * WalkMoveAlpha);

\t\tconst FQuat UpperDelta = FQuat(CompRightDir, WalkSwingRad) * FQuat(CompForwardDir, -BreathRad);
\t\tconst FQuat LowerDelta = FQuat(CompRightDir, ElbowFlexRad);

\t\tPropagateSingleChildBoneCS(CSPose, RestCompBases, ClavicleLIdx, UpperArmLIdx, UpperDelta);
\t\tPropagateSingleChildBoneCS(CSPose, RestCompBases, UpperArmLIdx, LowerArmLIdx, LowerDelta);
\t\tPropagateSingleChildBoneCS(CSPose, RestCompBases, LowerArmLIdx, HandLIdx);
\t\tconst FTransform HandLCS = CSPose.Pose.GetComponentSpaceTransform(HandLIdx);
\t\tPropagateHandChildBones(CSPose, RestCompBases, HandLIdx, HandLCS);
\t\treturn;
\t}

\tconst FTransform HandRCS = CSPose.Pose.GetComponentSpaceTransform(HandRIdx);
\tconst FTransform Spine03CS = CSPose.Pose.GetComponentSpaceTransform(Spine03Idx);
\tconst FVector HandRPos = HandRCS.GetLocation();
\tconst FVector ChestPos = Spine03CS.GetLocation();
\tconst FVector RestHandLPos = CSPose.Pose.GetComponentSpaceTransform(HandLIdx).GetLocation();

\t// Low-Ready Patrol Cradle: Left hand supports the handguard across the chest,
\t// angling the barrel ~18 degrees downward and across the body toward the left hip/front.
\t// Foregrip distance from pistol grip (`HandRPos`) is ~31 cm.
\tconst FVector LowReadyBarrelDir = (
\t\t-CompRightDir * 0.62f
\t\t+ CompForwardDir * 0.72f
\t\t- CompUpDir * 0.31f // ~18 deg downward pitch
\t).GetSafeNormal();

\tconst float PitchRad = FMath::DegreesToRadians(FMath::Clamp(AimPitch, -65.0f, 65.0f));
\tconst FVector HighReadyBarrelDir = (
\t\t-CompRightDir * (0.18f * (1.0f - SmoothADSAlpha))
\t\t+ CompForwardDir * FMath::Cos(PitchRad)
\t\t+ CompUpDir * FMath::Sin(PitchRad)
\t).GetSafeNormal();

\tconst FVector BarrelDir = FMath::Lerp(LowReadyBarrelDir, HighReadyBarrelDir, HighReadyAlpha).GetSafeNormal();
\tconst FVector ArmedHandL = HandRPos + BarrelDir * 31.0f - CompUpDir * 1.5f;

\tconst FVector EffectorTargetCS = bUnarmedRelaxedIdle
\t\t? FMath::Lerp(RestHandLPos, ArmedHandL, HighReadyAlpha)
\t\t: ArmedHandL;

\t// Left elbow pole target: below and left of ribcage
\tconst FVector JointTargetCS =
\t\tChestPos
\t\t- CompRightDir * 28.0f
\t\t+ CompForwardDir * 4.0f
\t\t- CompUpDir * 24.0f;"""

new_arm_l = """\tconst FTransform HandRCS = CSPose.Pose.GetComponentSpaceTransform(HandRIdx);
\tconst FTransform Spine03CS = CSPose.Pose.GetComponentSpaceTransform(Spine03Idx);
\tconst FVector HandRPos = HandRCS.GetLocation();
\tconst FVector ChestPos = Spine03CS.GetLocation();
\tconst float BreathOffsetL = FMath::Sin(IdleBreathPhase) * 0.45f;

\t// Natural Relaxed Resting Hand for Left Arm (beside outer thigh/fast-mag pouch, natural ~22 deg bend)
\tconst FVector RelaxedHandL =
\t\tChestPos
\t\t- CompRightDir * 19.5f
\t\t+ CompForwardDir * 6.5f
\t\t- CompUpDir * 42.0f
\t\t+ CompUpDir * (BreathOffsetL * 0.85f)
\t\t+ CompForwardDir * (BreathOffsetL * 0.55f + FMath::Sin(LocomotionCyclePhase + PI) * 16.0f * WalkMoveAlpha);

\tconst FVector RelaxedJointL =
\t\tChestPos
\t\t- CompRightDir * 25.5f
\t\t- CompForwardDir * 7.0f
\t\t- CompUpDir * 19.0f;

\t// Low-Ready Patrol Cradle: Left hand supports the handguard across the chest,
\t// angling the barrel ~18 degrees downward and across the body toward the left hip/front.
\tconst FVector LowReadyBarrelDir = (
\t\t-CompRightDir * 0.62f
\t\t+ CompForwardDir * 0.72f
\t\t- CompUpDir * 0.31f // ~18 deg downward pitch
\t).GetSafeNormal();

\tconst float PitchRad = FMath::DegreesToRadians(FMath::Clamp(AimPitch, -65.0f, 65.0f));
\tconst FVector HighReadyBarrelDir = (
\t\t-CompRightDir * (0.18f * (1.0f - SmoothADSAlpha))
\t\t+ CompForwardDir * FMath::Cos(PitchRad)
\t\t+ CompUpDir * FMath::Sin(PitchRad)
\t).GetSafeNormal();

\tconst FVector BarrelDir = FMath::Lerp(LowReadyBarrelDir, HighReadyBarrelDir, HighReadyAlpha).GetSafeNormal();
\tconst FVector ArmedHandL = HandRPos + BarrelDir * 31.0f - CompUpDir * 1.5f;

\tconst FVector ArmedJointL =
\t\tChestPos
\t\t- CompRightDir * 28.0f
\t\t+ CompForwardDir * 4.0f
\t\t- CompUpDir * 24.0f;

\tconst FVector EffectorTargetCS = bUnarmedRelaxedIdle
\t\t? FMath::Lerp(RelaxedHandL, ArmedHandL, HighReadyAlpha)
\t\t: ArmedHandL;

\tconst FVector JointTargetCS = bUnarmedRelaxedIdle
\t\t? FMath::Lerp(RelaxedJointL, ArmedJointL, HighReadyAlpha)
\t\t: ArmedJointL;"""

if old_arm_l in content:
    content = content.replace(old_arm_l, new_arm_l)
    print("Replaced old_arm_l")
else:
    print("old_arm_l not found")

with open(path, "w", encoding="utf-8") as f:
    f.write(content)

print("Finished patching FPSAnimInstance.cpp")
