import os

path = r"Source/NobleFermi/Character/FPSCharacter.cpp"
with open(path, "r", encoding="utf-8") as f:
    content = f.read()

target1 = """\t// Tactical Locomotion Animations
\tstatic ConstructorHelpers::FObjectFinder<UAnimSequence> IdleAnimFinder(TEXT("/Game/Characters/BaseMale/Animations/Anim_BaseMale_Idle_ADS.Anim_BaseMale_Idle_ADS"));
\tif (IdleAnimFinder.Succeeded())
\t{
\t\tAnimIdleADS = IdleAnimFinder.Object;
\t}

\tstatic ConstructorHelpers::FObjectFinder<UAnimSequence> WalkAnimFinder(TEXT("/Game/Characters/BaseMale/Animations/Anim_BaseMale_Walk.Anim_BaseMale_Walk"));
\tif (WalkAnimFinder.Succeeded())
\t{
\t\tAnimWalk = WalkAnimFinder.Object;
\t}

\tstatic ConstructorHelpers::FObjectFinder<UAnimSequence> RunAnimFinder(TEXT("/Game/Characters/BaseMale/Animations/Anim_BaseMale_Run.Anim_BaseMale_Run"));
\tif (RunAnimFinder.Succeeded())
\t{
\t\tAnimRun = RunAnimFinder.Object;
\t}

\tstatic ConstructorHelpers::FObjectFinder<UAnimSequence> SprintAnimFinder(TEXT("/Game/Characters/BaseMale/Animations/Anim_BaseMale_Sprint.Anim_BaseMale_Sprint"));
\tif (SprintAnimFinder.Succeeded())
\t{
\t\tAnimSprint = SprintAnimFinder.Object;
\t}"""

replacement1 = """\t// Tactical Locomotion Animations (Authoritative Procedural Animation in UFPSAnimInstance)
\tAnimIdleADS = nullptr;
\tAnimWalk = nullptr;
\tAnimRun = nullptr;
\tAnimSprint = nullptr;"""

if target1 in content:
    content = content.replace(target1, replacement1)
    print("Replaced target1 successfully")
else:
    # Try with CRLF
    t1_crlf = target1.replace("\n", "\r\n")
    if t1_crlf in content:
        content = content.replace(t1_crlf, replacement1.replace("\n", "\r\n"))
        print("Replaced target1 (CRLF) successfully")
    else:
        print("Target1 not found")

target2 = """\t\tif (!AnimIdleADS)
\t\t{
\t\t\tAnimIdleADS = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/BaseMale/Animations/Anim_BaseMale_Idle_ADS.Anim_BaseMale_Idle_ADS"));
\t\t}
\t\tif (!AnimLowReady)
\t\t{
\t\t\tAnimLowReady = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/BaseMale/Animations/Anim_BaseMale_LowReady.Anim_BaseMale_LowReady"));
\t\t}
\t\tif (!AnimWalk)
\t\t{
\t\t\tAnimWalk = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/BaseMale/Animations/Anim_BaseMale_Walk.Anim_BaseMale_Walk"));
\t\t}
\t\tif (!AnimRun)
\t\t{
\t\t\tAnimRun = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/BaseMale/Animations/Anim_BaseMale_Run.Anim_BaseMale_Run"));
\t\t}
\t\tif (!AnimSprint)
\t\t{
\t\t\tAnimSprint = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/BaseMale/Animations/Anim_BaseMale_Sprint.Anim_BaseMale_Sprint"));
\t\t}
\t\tif (!AnimJumpLoop)
\t\t{
\t\t\tAnimJumpLoop = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/BaseMale/Animations/Anim_BaseMale_Jump_Loop.Anim_BaseMale_Jump_Loop"));
\t\t}
\t\tif (!AnimCrouchIdle)
\t\t{
\t\t\tAnimCrouchIdle = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/BaseMale/Animations/Anim_BaseMale_Crouch_Idle.Anim_BaseMale_Crouch_Idle"));
\t\t}
\t\tif (!AnimCrouchWalk)
\t\t{
\t\t\tAnimCrouchWalk = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/BaseMale/Animations/Anim_BaseMale_Crouch_Walk.Anim_BaseMale_Crouch_Walk"));
\t\t}
\t\tif (!AnimSlide)
\t\t{
\t\t\tAnimSlide = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/BaseMale/Animations/Anim_BaseMale_Slide.Anim_BaseMale_Slide"));
\t\t}
\t\tif (!AnimWalkBackwards)
\t\t{
\t\t\tAnimWalkBackwards = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/BaseMale/Animations/Anim_BaseMale_Walk_Backwards.Anim_BaseMale_Walk_Backwards"));
\t\t}
\t\tif (!AnimReload)
\t\t{
\t\t\tAnimReload = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/BaseMale/Animations/Anim_BaseMale_Reload.Anim_BaseMale_Reload"));
\t\t}"""

replacement2 = """\t\t// Authoritative Procedural Animation in UFPSAnimInstance (Resting Idle + Locomotion + IK)
\t\tAnimIdleADS = nullptr;
\t\tAnimLowReady = nullptr;
\t\tAnimWalk = nullptr;
\t\tAnimRun = nullptr;
\t\tAnimSprint = nullptr;
\t\tAnimJumpLoop = nullptr;
\t\tAnimCrouchIdle = nullptr;
\t\tAnimCrouchWalk = nullptr;
\t\tAnimSlide = nullptr;
\t\tAnimWalkBackwards = nullptr;
\t\tAnimReload = nullptr;"""

if target2 in content:
    content = content.replace(target2, replacement2)
    print("Replaced target2 successfully")
else:
    t2_crlf = target2.replace("\n", "\r\n")
    if t2_crlf in content:
        content = content.replace(t2_crlf, replacement2.replace("\n", "\r\n"))
        print("Replaced target2 (CRLF) successfully")
    else:
        print("Target2 not found")

with open(path, "w", encoding="utf-8") as f:
    f.write(content)
print("Finished update_fps_character_anims.py")
