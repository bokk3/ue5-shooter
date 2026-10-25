import os
import re

print("=== PATCHING FPSCHARACTERPREVIEWACTOR.CPP ===")

cpp_path = r"Source\NobleFermi\Character\FPSCharacterPreviewActor.cpp"
with open(cpp_path, "rb") as f:
    cpp_bytes = f.read()

cpp_text = cpp_bytes.decode("utf-8")

# Let's inspect constructor around Pedestal and BodyMesh
old_pedestal_and_body = """\t// 5.5 Master Anatomical Player Body (SK_BaseMale with connected limbs & realistic muscle deformation)
\tBodyMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BodyMesh"));
\tBodyMesh->SetupAttachment(TurntableRoot);
\tBodyMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
\tBodyMesh->SetRelativeRotation(FRotator(0.0f, -10.0f, 0.0f));
\tBodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
\tBodyMesh->SetCastShadow(true);

\tstatic ConstructorHelpers::FObjectFinder<USkeletalMesh> BaseMaleFinder(TEXT("/Game/Characters/BaseMale/SK_BaseMale.SK_BaseMale"));
\tif (BaseMaleFinder.Succeeded())
\t{
\t\tBodyMesh->SetSkeletalMesh(BaseMaleFinder.Object);
\t}

\tstatic ConstructorHelpers::FObjectFinder<UAnimSequence> IdleAnimFinder(TEXT("/Game/Characters/BaseMale/Animations/Anim_BaseMale_Idle_ADS.Anim_BaseMale_Idle_ADS"));
\tif (IdleAnimFinder.Succeeded())
\t{
\t\tAnimIdleADS = IdleAnimFinder.Object;
\t}

\tstatic ConstructorHelpers::FObjectFinder<UAnimSequence> LowReadyAnimFinder(TEXT("/Game/Characters/BaseMale/Animations/Anim_BaseMale_LowReady.Anim_BaseMale_LowReady"));
\tif (LowReadyAnimFinder.Succeeded())
\t{
\t\tAnimLowReady = LowReadyAnimFinder.Object;
\t}

\tstatic ConstructorHelpers::FObjectFinder<UAnimSequence> WalkAnimFinder(TEXT("/Game/Characters/BaseMale/Animations/Anim_BaseMale_Walk.Anim_BaseMale_Walk"));
\tif (WalkAnimFinder.Succeeded())
\t{
\t\tAnimWalk = WalkAnimFinder.Object;
\t}

\t// SK_BaseMale natural Spec-Ops pose with continuous skinning evaluation
\tBodyMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
\tBodyMesh->SetForcedLOD(1);

\tstatic ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseBodyMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_Operator_BaseBody.M_Operator_BaseBody"));
\tif (BaseBodyMatFinder.Succeeded())
\t{
\t\tBodyMesh->SetMaterial(0, BaseBodyMatFinder.Object);
\t}

\t// 5.6 7-Zone Socket-Driven 3D Tactical Gear & Carbine Rig Component
\tOperatorGearComponent = CreateDefaultSubobject<UFPSOperatorGearComponent>(TEXT("OperatorGearComponent"));

\t// 5.7 Master 3D Tactical AK-47 Rifle Mesh (Attached to weapon_r bone socket)
\tAK47Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AK47Mesh"));
\tAK47Mesh->SetupAttachment(BodyMesh, TEXT("weapon_r"));
\tAK47Mesh->SetRelativeLocation(FVector::ZeroVector);
\tAK47Mesh->SetRelativeRotation(FRotator::ZeroRotator);
\tAK47Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
\tAK47Mesh->SetCastShadow(true);
\tAK47Mesh->SetVisibility(true);

\tstatic ConstructorHelpers::FObjectFinder<UStaticMesh> AK47MeshFinder(TEXT("/Game/Characters/BaseMale/SM_AK47.SM_AK47"));
\tif (AK47MeshFinder.Succeeded())
\t{
\t\tAK47Mesh->SetStaticMesh(AK47MeshFinder.Object);
\t}
\tstatic ConstructorHelpers::FObjectFinder<UMaterialInterface> AKSteelMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Steel.M_AK47_Steel"));
\tstatic ConstructorHelpers::FObjectFinder<UMaterialInterface> AKWoodMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Wood.M_AK47_Wood"));
\tstatic ConstructorHelpers::FObjectFinder<UMaterialInterface> AKBakeMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Bakelite.M_AK47_Bakelite"));
\tif (AKSteelMatFinder.Succeeded()) AK47Mesh->SetMaterial(0, AKSteelMatFinder.Object);
\tif (AKWoodMatFinder.Succeeded()) AK47Mesh->SetMaterial(1, AKWoodMatFinder.Object);
\tif (AKBakeMatFinder.Succeeded()) AK47Mesh->SetMaterial(2, AKBakeMatFinder.Object);

\t// 6. Turntable Pedestal
\tSetupMeshComp(PedestalMesh, TEXT("PedestalMesh"), CylinderMesh, FVector(0.0f, 0.0f, 6.0f), FRotator::ZeroRotator, FVector(1.4f, 1.4f, 0.12f));"""

new_pedestal_and_body = """\tbIsRotating = false;

\t// 5. Tactical Turntable Pedestal (Radius 135cm, Height 14cm with beveled rim)
\tPedestalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PedestalMesh"));
\tPedestalMesh->SetupAttachment(TurntableRoot);
\tPedestalMesh->SetMobility(EComponentMobility::Movable);
\tPedestalMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
\tPedestalMesh->SetRelativeRotation(FRotator::ZeroRotator);
\tPedestalMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
\tPedestalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
\tPedestalMesh->SetCastShadow(true);

\tstatic ConstructorHelpers::FObjectFinder<UStaticMesh> PlinthFinder(TEXT("/Game/Environment/Hangar/SM_Hangar_Plinth.SM_Hangar_Plinth"));
\tif (PlinthFinder.Succeeded())
\t{
\t\tPedestalMesh->SetStaticMesh(PlinthFinder.Object);
\t}

\t// 5.5 Master High-Fidelity Posed Operator (Vanguard Spec-Ops Soldier with 7-Zone Tactical Armor, Visor, Boots & AK-47)
\tOperatorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OperatorMesh"));
\tOperatorMesh->SetupAttachment(TurntableRoot);
\tOperatorMesh->SetMobility(EComponentMobility::Movable);
\tOperatorMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
\tOperatorMesh->SetRelativeRotation(FRotator(0.0f, 15.0f, 0.0f)); // Heroic 15-degree angle facing camera
\tOperatorMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
\tOperatorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
\tOperatorMesh->SetCastShadow(true);
\tOperatorMesh->SetVisibility(true);

\tstatic ConstructorHelpers::FObjectFinder<UStaticMesh> PosedSoldierFinder(TEXT("/Game/Characters/BaseMale/SM_Soldier_Posed.SM_Soldier_Posed"));
\tif (PosedSoldierFinder.Succeeded())
\t{
\t\tOperatorMesh->SetStaticMesh(PosedSoldierFinder.Object);
\t}

\tstatic ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseBodyMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_Operator_BaseBody.M_Operator_BaseBody"));
\tstatic ConstructorHelpers::FObjectFinder<UMaterialInterface> VisorMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_Soldier_Visor.M_Soldier_Visor"));
\tstatic ConstructorHelpers::FObjectFinder<UMaterialInterface> AKSteelMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Steel.M_AK47_Steel"));
\tstatic ConstructorHelpers::FObjectFinder<UMaterialInterface> AKWoodMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Wood.M_AK47_Wood"));
\tstatic ConstructorHelpers::FObjectFinder<UMaterialInterface> AKBakeMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Bakelite.M_AK47_Bakelite"));

\tif (BaseBodyMatFinder.Succeeded()) OperatorMesh->SetMaterial(0, BaseBodyMatFinder.Object);
\tif (VisorMatFinder.Succeeded()) OperatorMesh->SetMaterial(1, VisorMatFinder.Object);
\tif (AKSteelMatFinder.Succeeded()) OperatorMesh->SetMaterial(2, AKSteelMatFinder.Object);
\tif (AKWoodMatFinder.Succeeded()) OperatorMesh->SetMaterial(3, AKWoodMatFinder.Object);
\tif (AKBakeMatFinder.Succeeded()) OperatorMesh->SetMaterial(4, AKBakeMatFinder.Object);

\tif (AKSteelMatFinder.Succeeded()) PedestalMesh->SetMaterial(0, AKSteelMatFinder.Object);
\tif (VisorMatFinder.Succeeded()) PedestalMesh->SetMaterial(1, VisorMatFinder.Object);

\t// Skeletal BodyMesh kept as inactive fallback
\tBodyMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BodyMesh"));
\tBodyMesh->SetupAttachment(TurntableRoot);
\tBodyMesh->SetVisibility(false);

\tOperatorGearComponent = CreateDefaultSubobject<UFPSOperatorGearComponent>(TEXT("OperatorGearComponent"));

\tAK47Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AK47Mesh"));
\tAK47Mesh->SetupAttachment(TurntableRoot);
\tAK47Mesh->SetVisibility(false);"""

def norm(s):
    return s.replace("\r\n", "\n")

if norm(old_pedestal_and_body) in norm(cpp_text):
    cpp_text = norm(cpp_text).replace(norm(old_pedestal_and_body), norm(new_pedestal_and_body), 1)
    print("Replaced Pedestal and BodyMesh in constructor")
else:
    print("WARNING: old_pedestal_and_body not found in FPSCharacterPreviewActor.cpp")

# Update camera framing
old_cam = """\tPreviewCamera->SetRelativeLocation(FVector(350.0f, 0.0f, 100.0f));
\tPreviewCamera->SetRelativeRotation(FRotator(-2.0f, 180.0f, 0.0f));
\tPreviewCamera->FieldOfView = 52.0f;"""

new_cam = """\tPreviewCamera->SetRelativeLocation(FVector(340.0f, 0.0f, 95.0f));
\tPreviewCamera->SetRelativeRotation(FRotator(-2.0f, 180.0f, 0.0f));
\tPreviewCamera->FieldOfView = 50.0f;"""

if norm(old_cam) in norm(cpp_text):
    cpp_text = norm(cpp_text).replace(norm(old_cam), norm(new_cam), 1)
    print("Updated camera framing in constructor")

# Update BeginPlay
old_beginplay = """void AFPSCharacterPreviewActor::BeginPlay()
{
\tSuper::BeginPlay();

\tUE_LOG(LogTemp, Warning, TEXT("[PREVIEW] AFPSCharacterPreviewActor::BeginPlay() running. PreviewCamera: %s at %s"),
\t\tPreviewCamera ? TEXT("VALID") : TEXT("NULL"),
\t\tPreviewCamera ? *PreviewCamera->GetRelativeLocation().ToString() : TEXT("NONE"));

\t// Ensure SK_BaseMale is loaded
\tif (BodyMesh && !BodyMesh->GetSkeletalMeshAsset())
\t{
\t\tUSkeletalMesh* LoadedMesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/BaseMale/SK_BaseMale.SK_BaseMale"));
\t\tif (!LoadedMesh)
\t\t{
\t\t\tLoadedMesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/BaseMale/SK_BaseMale"));
\t\t}
\t\tif (LoadedMesh)
\t\t{
\t\t\tBodyMesh->SetSkeletalMesh(LoadedMesh);
\t\t}
\t}

\t\tif (BodyMesh)
\t{
\t\tBodyMesh->bComponentUseFixedSkelBounds = false;
\t\tBodyMesh->BoundsScale = 10.0f;
\t\tBodyMesh->SetForcedLOD(0);

\t\t// Run C++ UFPSAnimInstance (21-bone Low-Ready Carbine + Two-Bone IK solver)
\t\tBodyMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
\t\tBodyMesh->SetAnimInstanceClass(UFPSAnimInstance::StaticClass());
\t\tBodyMesh->InitAnim(true);

\t\tif (UFPSAnimInstance* FPSAnim = Cast<UFPSAnimInstance>(BodyMesh->GetAnimInstance()))
\t\t{
\t\t\tFPSAnim->bIsMenuPreview = true;
\t\t\tFPSAnim->MenuPoseIndex = CurrentStanceIndex; // 0 = Low-Ready Carbine
\t\t}

\t\tBodyMesh->SetVisibility(true, true);
\t\tBodyMesh->SetHiddenInGame(false, true);
\t}

\tif (BodyMesh)
\t{
\t\tUMaterialInterface* CurrentMat = BodyMesh->GetMaterial(0);
\t\tif (!CurrentMat || !CurrentMat->GetName().Contains(TEXT("M_Operator_BaseBody")))
\t\t{
\t\t\tif (UMaterialInterface* LoadedMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_Operator_BaseBody.M_Operator_BaseBody")))
\t\t\t{
\t\t\t\tBodyMesh->SetMaterial(0, LoadedMat);
\t\t\t}
\t\t}
\t}

\tFBoxSphereBounds MeshBounds = BodyMesh ? BodyMesh->CalcBounds(BodyMesh->GetComponentTransform()) : FBoxSphereBounds();
\tUE_LOG(LogTemp, Warning, TEXT("[PREVIEW_DIAG] BodyMesh valid=%d, Asset=%s, Bones=%d, BoundsScale=%.1f, Radius=%.1f, Vis=%d, Hidden=%d, Loc=%s, AnimClass=%s"),
\t\tBodyMesh != nullptr,
\t\t(BodyMesh && BodyMesh->GetSkeletalMeshAsset()) ? *BodyMesh->GetSkeletalMeshAsset()->GetName() : TEXT("NULL"),
\t\t(BodyMesh && BodyMesh->GetSkeletalMeshAsset()) ? BodyMesh->GetNumBones() : 0,
\t\tBodyMesh ? BodyMesh->BoundsScale : 0.0f,
\t\tMeshBounds.SphereRadius,
\t\tBodyMesh ? BodyMesh->IsVisible() : 0,
\t\tBodyMesh ? BodyMesh->bHiddenInGame : 0,
\t\tBodyMesh ? *BodyMesh->GetComponentLocation().ToString() : TEXT("NONE"),
\t\t(BodyMesh && BodyMesh->GetAnimInstance()) ? *BodyMesh->GetAnimInstance()->GetClass()->GetName() : TEXT("NONE"));

\tif (BodyMesh && BodyMesh->GetSkeletalMeshAsset())
\t{
\t\t// Hide all legacy primitive blocks when SK_BaseMale + UFPSOperatorGearComponent are active
\t\tauto HideComp = [](UStaticMeshComponent* Comp)
\t\t{
\t\t\tif (Comp)
\t\t\t{
\t\t\t\tComp->SetVisibility(false);
\t\t\t\tComp->SetHiddenInGame(true);
\t\t\t}
\t\t};
\t\tHideComp(PelvisMesh);
\t\tHideComp(SpineLowerMesh);
\t\tHideComp(SpineUpperMesh);
\t\tHideComp(ChestMesh);
\t\tHideComp(NeckMesh);
\t\tHideComp(HeadMesh);
\t\tHideComp(RightClavicleMesh);
\t\tHideComp(RightUpperArmMesh);
\t\tHideComp(RightForearmMesh);
\t\tHideComp(RightHandMesh);
\t\tHideComp(LeftClavicleMesh);
\t\tHideComp(LeftUpperArmMesh);
\t\tHideComp(LeftForearmMesh);
\t\tHideComp(LeftHandMesh);
\t\tHideComp(LeftThighMesh);
\t\tHideComp(LeftCalfMesh);
\t\tHideComp(RightThighMesh);
\t\tHideComp(RightCalfMesh);
\t\tHideComp(LeftBootMesh);
\t\tHideComp(RightBootMesh);
\t\tHideComp(VestMesh);
\t\tHideComp(HelmetMesh);
\t\tHideComp(NVGMesh);
\t\tHideComp(BalaclavaMesh);
\t\tHideComp(CapMesh);
\t\tHideComp(WeaponReceiverMesh);
\t\tHideComp(WeaponBarrelMesh);
\t\tHideComp(WeaponOpticMesh);
\t\tHideComp(WeaponMagMesh);
\t\tHideComp(WeaponStockMesh);
\t\t// Position real 3D AK-47 in authoritative Low-Ready Carbine stance
\t\tBodyMesh->SetVisibility(true, true);
\t\tif (AK47Mesh)
\t\t{
\t\t\tif (!AK47Mesh->GetStaticMesh())
\t\t\t{
\t\t\t\tif (UStaticMesh* LoadedAK = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Characters/BaseMale/SM_AK47.SM_AK47")))
\t\t\t\t{
\t\t\t\t\tAK47Mesh->SetStaticMesh(LoadedAK);
\t\t\t\t}
\t\t\t}
\t\t\tif (BodyMesh && BodyMesh->DoesSocketExist(TEXT("weapon_r")))
\t\t\t{
\t\t\t\tAK47Mesh->AttachToComponent(BodyMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("weapon_r"));
\t\t\t\tAK47Mesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
\t\t\t\tAK47Mesh->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
\t\t\t}
\t\t\telse
\t\t\t{
\t\t\t\tAK47Mesh->AttachToComponent(TurntableRoot, FAttachmentTransformRules::KeepRelativeTransform);
\t\t\t\tAK47Mesh->SetRelativeLocation(FVector(26.0f, 8.0f, 108.0f));
\t\t\t\tAK47Mesh->SetRelativeRotation(FRotator(-20.0f, 55.0f, -8.0f));
\t\t\t}
\t\t\tAK47Mesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
\t\t\tAK47Mesh->SetVisibility(true, true);
\t\t\tAK47Mesh->SetHiddenInGame(false, true);
\t\t}
\t}

\t// Pedestal prominent cyan display plinth
\tif (PedestalMesh)
\t{
\t\tif (!PedestalMesh->GetStaticMesh())
\t\t{
\t\t\tif (UStaticMesh* Cyl = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")))
\t\t\t{
\t\t\t\tPedestalMesh->SetStaticMesh(Cyl);
\t\t\t}
\t\t}
\t\tPedestalMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 6.0f));
\t\tPedestalMesh->SetRelativeScale3D(FVector(1.4f, 1.4f, 0.12f));
\t\tPedestalMesh->SetVisibility(true, true);
\t\tPedestalMesh->SetHiddenInGame(false, true);

\t\tif (UMaterialInterface* MasterMultiMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_Operator_BaseBody.M_Operator_BaseBody")))
\t\t{
\t\t\tPedestalMesh->SetMaterial(0, MasterMultiMat);
\t\t}
\t\tif (UMaterialInstanceDynamic* PedMID = PedestalMesh->CreateDynamicMaterialInstance(0))
\t\t{
\t\t\tPedMID->SetVectorParameterValue(TEXT("CamoColorPrimary"), FLinearColor(0.0f, 0.85f, 1.0f));
\t\t\tPedMID->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.0f, 0.85f, 1.0f));
\t\t\tPedMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.0f, 0.85f, 1.0f));
\t\t}
\t}"""

new_beginplay = """void AFPSCharacterPreviewActor::BeginPlay()
{
\tSuper::BeginPlay();

\tUE_LOG(LogTemp, Warning, TEXT("[PREVIEW] AFPSCharacterPreviewActor::BeginPlay() running. PreviewCamera: %s at %s"),
\t\tPreviewCamera ? TEXT("VALID") : TEXT("NULL"),
\t\tPreviewCamera ? *PreviewCamera->GetRelativeLocation().ToString() : TEXT("NONE"));

\t// Ensure OperatorMesh is loaded and visible
\tif (OperatorMesh)
\t{
\t\tif (!OperatorMesh->GetStaticMesh())
\t\t{
\t\t\tif (UStaticMesh* LoadedPosed = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Characters/BaseMale/SM_Soldier_Posed.SM_Soldier_Posed")))
\t\t\t{
\t\t\t\tOperatorMesh->SetStaticMesh(LoadedPosed);
\t\t\t}
\t\t}

\t\tOperatorMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
\t\tOperatorMesh->SetRelativeRotation(FRotator(0.0f, 15.0f, 0.0f));
\t\tOperatorMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
\t\tOperatorMesh->SetVisibility(true, true);
\t\tOperatorMesh->SetHiddenInGame(false, true);

\t\tif (UMaterialInterface* BodyMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_Operator_BaseBody.M_Operator_BaseBody")))
\t\t\tOperatorMesh->SetMaterial(0, BodyMat);
\t\tif (UMaterialInterface* VisorMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_Soldier_Visor.M_Soldier_Visor")))
\t\t\tOperatorMesh->SetMaterial(1, VisorMat);
\t\tif (UMaterialInterface* AKSteel = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Steel.M_AK47_Steel")))
\t\t\tOperatorMesh->SetMaterial(2, AKSteel);
\t\tif (UMaterialInterface* AKWood = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Wood.M_AK47_Wood")))
\t\t\tOperatorMesh->SetMaterial(3, AKWood);
\t\tif (UMaterialInterface* AKBake = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Bakelite.M_AK47_Bakelite")))
\t\t\tOperatorMesh->SetMaterial(4, AKBake);
\t}

\t// Ensure PedestalMesh is loaded and visible
\tif (PedestalMesh)
\t{
\t\tif (!PedestalMesh->GetStaticMesh())
\t\t{
\t\t\tif (UStaticMesh* LoadedPlinth = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Environment/Hangar/SM_Hangar_Plinth.SM_Hangar_Plinth")))
\t\t\t{
\t\t\t\tPedestalMesh->SetStaticMesh(LoadedPlinth);
\t\t\t}
\t\t}

\t\tPedestalMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
\t\tPedestalMesh->SetRelativeRotation(FRotator::ZeroRotator);
\t\tPedestalMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
\t\tPedestalMesh->SetVisibility(true, true);
\t\tPedestalMesh->SetHiddenInGame(false, true);

\t\tif (UMaterialInterface* AKSteel = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Steel.M_AK47_Steel")))
\t\t\tPedestalMesh->SetMaterial(0, AKSteel);
\t\tif (UMaterialInterface* VisorMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_Soldier_Visor.M_Soldier_Visor")))
\t\t\tPedestalMesh->SetMaterial(1, VisorMat);
\t}

\t// Hide all legacy primitive boxes and inactive BodyMesh
\tauto HideComp = [](UPrimitiveComponent* Comp)
\t{
\t\tif (Comp)
\t\t{
\t\t\tComp->SetVisibility(false);
\t\t\tComp->SetHiddenInGame(true);
\t\t}
\t};
\tHideComp(BodyMesh);
\tHideComp(AK47Mesh);
\tHideComp(PelvisMesh);
\tHideComp(SpineLowerMesh);
\tHideComp(SpineUpperMesh);
\tHideComp(ChestMesh);
\tHideComp(NeckMesh);
\tHideComp(HeadMesh);
\tHideComp(RightClavicleMesh);
\tHideComp(RightUpperArmMesh);
\tHideComp(RightForearmMesh);
\tHideComp(RightHandMesh);
\tHideComp(LeftClavicleMesh);
\tHideComp(LeftUpperArmMesh);
\tHideComp(LeftForearmMesh);
\tHideComp(LeftHandMesh);
\tHideComp(LeftThighMesh);
\tHideComp(LeftCalfMesh);
\tHideComp(RightThighMesh);
\tHideComp(RightCalfMesh);
\tHideComp(LeftBootMesh);
\tHideComp(RightBootMesh);
\tHideComp(VestMesh);
\tHideComp(HelmetMesh);
\tHideComp(NVGMesh);
\tHideComp(BalaclavaMesh);
\tHideComp(CapMesh);
\tHideComp(WeaponReceiverMesh);
\tHideComp(WeaponBarrelMesh);
\tHideComp(WeaponOpticMesh);
\tHideComp(WeaponMagMesh);
\tHideComp(WeaponStockMesh);

\tUE_LOG(LogTemp, Warning, TEXT("[PREVIEW_DIAG] OperatorMesh valid=%d, Asset=%s, Vis=%d, Hidden=%d, Loc=%s, Pedestal=%s"),
\t\tOperatorMesh != nullptr,
\t\t(OperatorMesh && OperatorMesh->GetStaticMesh()) ? *OperatorMesh->GetStaticMesh()->GetName() : TEXT("NULL"),
\t\tOperatorMesh ? OperatorMesh->IsVisible() : 0,
\t\tOperatorMesh ? OperatorMesh->bHiddenInGame : 0,
\t\tOperatorMesh ? *OperatorMesh->GetComponentLocation().ToString() : TEXT("NONE"),
\t\t(PedestalMesh && PedestalMesh->GetStaticMesh()) ? *PedestalMesh->GetStaticMesh()->GetName() : TEXT("NULL"));"""

if norm(old_beginplay) in norm(cpp_text):
    cpp_text = norm(cpp_text).replace(norm(old_beginplay), norm(new_beginplay), 1)
    print("Replaced BeginPlay in FPSCharacterPreviewActor.cpp")
else:
    print("WARNING: old_beginplay not found in FPSCharacterPreviewActor.cpp")

# Update ApplyCamoColors
old_camo = """void AFPSCharacterPreviewActor::ApplyCamoColors()
{
\tif (BodyMesh)
\t{
\t\tUMaterialInstanceDynamic* BodyMID = Cast<UMaterialInstanceDynamic>(BodyMesh->GetMaterial(0));
\t\tif (!BodyMID)
\t\t{
\t\t\tBodyMID = BodyMesh->CreateDynamicMaterialInstance(0);
\t\t}
\t\tif (BodyMID)
\t\t{
\t\t\t// Bright tactical tint ensuring crisp in-engine visibility
\t\t\tFLinearColor CamoTint = FLinearColor(2.2f, 2.0f, 1.7f, 1.0f); // Desert Sand SpecOps
\t\t\tif (CurrentCamo == EOperatorCamo::WoodlandTactical)
\t\t\t{
\t\t\t\tCamoTint = FLinearColor(1.2f, 1.8f, 1.1f, 1.0f); // Tactical Olive Drab
\t\t\t}
\t\t\telse if (CurrentCamo == EOperatorCamo::StealthBlackout)
\t\t\t{
\t\t\t\tCamoTint = FLinearColor(1.2f, 1.3f, 1.5f, 1.0f); // Stealth Slate
\t\t\t}
\t\t\tBodyMID->SetVectorParameterValue(TEXT("CamoColorPrimary"), CamoTint);
\t\t\tBodyMID->SetScalarParameterValue(TEXT("RoughnessBase"), 0.45f);
\t\t}

\t\tif (UMaterialInstanceDynamic* VisorMID = BodyMesh->CreateDynamicMaterialInstance(1))
\t\t{
\t\t\tVisorMID->SetVectorParameterValue(TEXT("CamoColorPrimary"), FLinearColor(0.0f, 0.90f, 1.0f));
\t\t\tVisorMID->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.0f, 0.90f, 1.0f));
\t\t}
\t}
}"""

new_camo = """void AFPSCharacterPreviewActor::ApplyCamoColors()
{
\tif (OperatorMesh)
\t{
\t\tUMaterialInstanceDynamic* BodyMID = Cast<UMaterialInstanceDynamic>(OperatorMesh->GetMaterial(0));
\t\tif (!BodyMID)
\t\t{
\t\t\tBodyMID = OperatorMesh->CreateDynamicMaterialInstance(0);
\t\t}
\t\tif (BodyMID)
\t\t{
\t\t\t// Tactical tint ensuring crisp in-engine visibility
\t\t\tFLinearColor CamoTint = FLinearColor(1.8f, 1.7f, 1.5f, 1.0f); // Desert Sand SpecOps
\t\t\tif (CurrentCamo == EOperatorCamo::WoodlandTactical)
\t\t\t{
\t\t\t\tCamoTint = FLinearColor(1.2f, 1.6f, 1.1f, 1.0f); // Tactical Olive Drab
\t\t\t}
\t\t\telse if (CurrentCamo == EOperatorCamo::StealthBlackout)
\t\t\t{
\t\t\t\tCamoTint = FLinearColor(1.1f, 1.2f, 1.4f, 1.0f); // Stealth Slate
\t\t\t}
\t\t\tBodyMID->SetVectorParameterValue(TEXT("CamoColorPrimary"), CamoTint);
\t\t\tBodyMID->SetScalarParameterValue(TEXT("RoughnessBase"), 0.45f);
\t\t}

\t\tif (UMaterialInstanceDynamic* VisorMID = OperatorMesh->CreateDynamicMaterialInstance(1))
\t\t{
\t\t\tVisorMID->SetVectorParameterValue(TEXT("CamoColorPrimary"), FLinearColor(0.0f, 0.90f, 1.0f));
\t\t\tVisorMID->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.0f, 0.90f, 1.0f));
\t\t}
\t}
}"""

if norm(old_camo) in norm(cpp_text):
    cpp_text = norm(cpp_text).replace(norm(old_camo), norm(new_camo), 1)
    print("Replaced ApplyCamoColors in FPSCharacterPreviewActor.cpp")
else:
    print("WARNING: old_camo not found in FPSCharacterPreviewActor.cpp")

# Update ApplyWeaponFinish
old_wpn = """void AFPSCharacterPreviewActor::ApplyWeaponFinish()
{
\tif (AK47Mesh)
\t{
\t\tUMaterialInstanceDynamic* SteelMID = Cast<UMaterialInstanceDynamic>(AK47Mesh->GetMaterial(0));
\t\tif (!SteelMID)
\t\t{
\t\t\tSteelMID = AK47Mesh->CreateDynamicMaterialInstance(0);
\t\t}
\t\tif (SteelMID)
\t\t{
\t\t\tFLinearColor SteelTint(0.12f, 0.13f, 0.15f, 1.0f);
\t\t\tif (CurrentWeaponFinish == EOperatorWeaponFinish::DesertTanCerakote)
\t\t\t{
\t\t\t\tSteelTint = FLinearColor(0.65f, 0.55f, 0.40f, 1.0f);
\t\t\t}
\t\t\telse if (CurrentWeaponFinish == EOperatorWeaponFinish::CarbonFiberSpecOps)
\t\t\t{
\t\t\t\tSteelTint = FLinearColor(0.04f, 0.04f, 0.05f, 1.0f);
\t\t\t}
\t\t\telse if (CurrentWeaponFinish == EOperatorWeaponFinish::GoldOpalDamascene)
\t\t\t{
\t\t\t\tSteelTint = FLinearColor(0.08f, 0.10f, 0.12f, 1.0f);
\t\t\t}
\t\t\tSteelMID->SetVectorParameterValue(TEXT("BaseColor"), SteelTint);
\t\t}
\t}
}"""

new_wpn = """void AFPSCharacterPreviewActor::ApplyWeaponFinish()
{
\tif (OperatorMesh)
\t{
\t\tUMaterialInstanceDynamic* SteelMID = Cast<UMaterialInstanceDynamic>(OperatorMesh->GetMaterial(2));
\t\tif (!SteelMID)
\t\t{
\t\t\tSteelMID = OperatorMesh->CreateDynamicMaterialInstance(2);
\t\t}
\t\tif (SteelMID)
\t\t{
\t\t\tFLinearColor SteelTint(0.12f, 0.13f, 0.15f, 1.0f);
\t\t\tif (CurrentWeaponFinish == EOperatorWeaponFinish::DesertTanCerakote)
\t\t\t{
\t\t\t\tSteelTint = FLinearColor(0.65f, 0.55f, 0.40f, 1.0f);
\t\t\t}
\t\t\telse if (CurrentWeaponFinish == EOperatorWeaponFinish::CarbonFiberSpecOps)
\t\t\t{
\t\t\t\tSteelTint = FLinearColor(0.04f, 0.04f, 0.05f, 1.0f);
\t\t\t}
\t\t\telse if (CurrentWeaponFinish == EOperatorWeaponFinish::GoldOpalDamascene)
\t\t\t{
\t\t\t\tSteelTint = FLinearColor(0.08f, 0.10f, 0.12f, 1.0f);
\t\t\t}
\t\t\tSteelMID->SetVectorParameterValue(TEXT("BaseColor"), SteelTint);
\t\t}
\t}
}"""

if norm(old_wpn) in norm(cpp_text):
    cpp_text = norm(cpp_text).replace(norm(old_wpn), norm(new_wpn), 1)
    print("Replaced ApplyWeaponFinish in FPSCharacterPreviewActor.cpp")
else:
    print("WARNING: old_wpn not found in FPSCharacterPreviewActor.cpp")

# Ensure CRLF line endings
cpp_text = re.sub(r'(?<!\r)\n', '\r\n', cpp_text)
with open(cpp_path, "wb") as f:
    f.write(cpp_text.encode("utf-8"))

print("Saved FPSCharacterPreviewActor.cpp successfully")
