#include "Character/FPSOperatorGearComponent.h"
#include "Character/FPSAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"
#include "Engine/Texture2D.h"
#include "TextureResource.h"

namespace
{
	struct FProcSectionBuffers
	{
		TArray<FVector> Vertices;
		TArray<int32> Triangles;
		TArray<FVector> Normals;
		TArray<FVector2D> UVs;
		TArray<FColor> Colors;
		TArray<FProcMeshTangent> Tangents;

		void Reset()
		{
			Vertices.Reset();
			Triangles.Reset();
			Normals.Reset();
			UVs.Reset();
			Colors.Reset();
			Tangents.Reset();
		}
	};

	static void AddBoxToSection(
		FProcSectionBuffers& Buf,
		const FVector& Center,
		const FVector& HalfExtents,
		const FRotator& Rotation = FRotator::ZeroRotator,
		const FColor& VertexColor = FColor(30, 34, 40, 255))
	{
		const FQuat RotQ = Rotation.Quaternion();
		const FVector LocalCorners[8] = {
			FVector(-HalfExtents.X, -HalfExtents.Y, -HalfExtents.Z),
			FVector( HalfExtents.X, -HalfExtents.Y, -HalfExtents.Z),
			FVector( HalfExtents.X,  HalfExtents.Y, -HalfExtents.Z),
			FVector(-HalfExtents.X,  HalfExtents.Y, -HalfExtents.Z),
			FVector(-HalfExtents.X, -HalfExtents.Y,  HalfExtents.Z),
			FVector( HalfExtents.X, -HalfExtents.Y,  HalfExtents.Z),
			FVector( HalfExtents.X,  HalfExtents.Y,  HalfExtents.Z),
			FVector(-HalfExtents.X,  HalfExtents.Y,  HalfExtents.Z)
		};

		FVector WorldCorners[8];
		for (int32 i = 0; i < 8; ++i)
		{
			WorldCorners[i] = Center + RotQ.RotateVector(LocalCorners[i]);
		}

		struct FFaceDef
		{
			int32 Idx[4];
			FVector LocalNormal;
		};

		const FFaceDef Faces[6] = {
			{ { 4, 5, 6, 7 }, FVector( 0,  0,  1) },
			{ { 3, 2, 1, 0 }, FVector( 0,  0, -1) },
			{ { 1, 2, 6, 5 }, FVector( 1,  0,  0) },
			{ { 3, 0, 4, 7 }, FVector(-1,  0,  0) },
			{ { 2, 3, 7, 6 }, FVector( 0,  1,  0) },
			{ { 0, 1, 5, 4 }, FVector( 0, -1,  0) }
		};

		for (int32 f = 0; f < 6; ++f)
		{
			const int32 BaseIdx = Buf.Vertices.Num();
			const FVector FaceNormal = RotQ.RotateVector(Faces[f].LocalNormal);
			for (int32 v = 0; v < 4; ++v)
			{
				Buf.Vertices.Add(WorldCorners[Faces[f].Idx[v]]);
				Buf.Normals.Add(FaceNormal);
				Buf.Colors.Add(VertexColor);
			}
			Buf.UVs.Add(FVector2D(0.0f, 0.0f));
			Buf.UVs.Add(FVector2D(1.0f, 0.0f));
			Buf.UVs.Add(FVector2D(1.0f, 1.0f));
			Buf.UVs.Add(FVector2D(0.0f, 1.0f));

			Buf.Triangles.Add(BaseIdx + 0);
			Buf.Triangles.Add(BaseIdx + 2);
			Buf.Triangles.Add(BaseIdx + 1);
			Buf.Triangles.Add(BaseIdx + 0);
			Buf.Triangles.Add(BaseIdx + 3);
			Buf.Triangles.Add(BaseIdx + 2);
		}
	}

	/** Adds a beveled octagonal armor plate / rounded hard-surface shell using 3 overlapping chamfered boxes */
	static void AddBeveledPlateToSection(
		FProcSectionBuffers& Buf,
		const FVector& Center,
		const FVector& HalfExtents,
		float BevelInset,
		const FRotator& Rotation = FRotator::ZeroRotator,
		const FColor& VertexColor = FColor(30, 34, 40, 255))
	{
		// Core full-depth center slab
		AddBoxToSection(Buf, Center, FVector(
			FMath::Max(0.5f, HalfExtents.X - BevelInset),
			HalfExtents.Y,
			FMath::Max(0.5f, HalfExtents.Z - BevelInset)), Rotation, VertexColor);

		// Wide mid-height slab
		AddBoxToSection(Buf, Center, FVector(
			HalfExtents.X,
			FMath::Max(0.3f, HalfExtents.Y * 0.86f),
			FMath::Max(0.5f, HalfExtents.Z - BevelInset * 1.4f)), Rotation, VertexColor);

		// Tall mid-width slab
		AddBoxToSection(Buf, Center, FVector(
			FMath::Max(0.5f, HalfExtents.X - BevelInset * 1.4f),
			FMath::Max(0.3f, HalfExtents.Y * 0.86f),
			HalfExtents.Z), Rotation, VertexColor);
	}

	/** Renders 3D raised micro-stencil text ("NOBLE FERMI") onto a placard along the X (Left->Right) and Z (Up) axes */
	static void Add3DStencilTextToSection(
		FProcSectionBuffers& Buf,
		const FString& Text,
		const FVector& CenterPos,
		float CharWidth,
		float CharHeight,
		float ReliefDepth,
		const FColor& Color)
	{
		// Note: In our bone-local frame, -X is Character Right and +X is Character Left.
		// Looking at the character's chest from the front camera (+Y looking toward -Y):
		// Screen-Left is +X (Character's Right side = -X? Wait: when facing camera (+X world),
		// Character's Right is +Y world (-X local), which is Screen-Left!).
		// Let's verify: Camera is at X=+335 looking toward X=0 (-X world direction).
		// Looking along -X world:
		//   Screen-Right is +Y world = Character's Right = -X local!
		//   Screen-Left  is -Y world = Character's Left  = +X local!
		// Wait: let's verify! Camera is at (335, 0, 126) with Yaw = 180 deg.
		// For Yaw = 180 deg: Camera Forward = (-1, 0, 0), Camera Right = (0, -1, 0), Camera Up = (0, 0, 1)!
		// And in our MeshQuat / MakeFromYZ frame:
		//   Local +Y = World (+1, 0, 0) (pointing toward camera)
		//   Local +X = World (0, -1, 0) (which is Camera Right = Screen-Right!)
		//   Local -X = World (0, +1, 0) (which is Camera Left  = Screen-Left!)
		// Therefore, reading Left-to-Right across the screen goes from Local -X to Local +X!

		auto GetGlyphRows = [](TCHAR Ch, uint8 OutRows[5])
		{
			switch (FChar::ToUpper(Ch))
			{
			case 'N': OutRows[0]=0x11; OutRows[1]=0x19; OutRows[2]=0x15; OutRows[3]=0x13; OutRows[4]=0x11; break;
			case 'O': OutRows[0]=0x0E; OutRows[1]=0x11; OutRows[2]=0x11; OutRows[3]=0x11; OutRows[4]=0x0E; break;
			case 'B': OutRows[0]=0x1E; OutRows[1]=0x11; OutRows[2]=0x1E; OutRows[3]=0x11; OutRows[4]=0x1E; break;
			case 'L': OutRows[0]=0x10; OutRows[1]=0x10; OutRows[2]=0x10; OutRows[3]=0x10; OutRows[4]=0x1F; break;
			case 'E': OutRows[0]=0x1F; OutRows[1]=0x10; OutRows[2]=0x1E; OutRows[3]=0x10; OutRows[4]=0x1F; break;
			case 'F': OutRows[0]=0x1F; OutRows[1]=0x10; OutRows[2]=0x1E; OutRows[3]=0x10; OutRows[4]=0x10; break;
			case 'R': OutRows[0]=0x1E; OutRows[1]=0x11; OutRows[2]=0x1E; OutRows[3]=0x14; OutRows[4]=0x13; break;
			case 'M': OutRows[0]=0x11; OutRows[1]=0x1B; OutRows[2]=0x15; OutRows[3]=0x11; OutRows[4]=0x11; break;
			case 'I': OutRows[0]=0x0E; OutRows[1]=0x04; OutRows[2]=0x04; OutRows[3]=0x04; OutRows[4]=0x0E; break;
			case '-': OutRows[0]=0x00; OutRows[1]=0x00; OutRows[2]=0x0E; OutRows[3]=0x00; OutRows[4]=0x00; break;
			default:  OutRows[0]=0x00; OutRows[1]=0x00; OutRows[2]=0x00; OutRows[3]=0x00; OutRows[4]=0x00; break;
			}
		};

		const int32 NumChars = Text.Len();
		if (NumChars <= 0) return;

		const float Spacing = CharWidth * 0.32f;
		const float TotalWidth = NumChars * CharWidth + (NumChars - 1) * Spacing;
		const float StartX = CenterPos.X - TotalWidth * 0.5f;
		const float PixelW = CharWidth / 5.0f;
		const float PixelH = CharHeight / 5.0f;

		for (int32 Idx = 0; Idx < NumChars; ++Idx)
		{
			uint8 Rows[5];
			GetGlyphRows(Text[Idx], Rows);
			const float CharBaseX = StartX + Idx * (CharWidth + Spacing);

			for (int32 Row = 0; Row < 5; ++Row)
			{
				const float PZ = CenterPos.Z + (2 - Row) * PixelH;
				for (int32 Col = 0; Col < 5; ++Col)
				{
					if ((Rows[Row] & (1 << (4 - Col))) != 0)
					{
						const float PX = CharBaseX + (Col + 0.5f) * PixelW;
						AddBoxToSection(
							Buf,
							FVector(PX, CenterPos.Y, PZ),
							FVector(PixelW * 0.52f, ReliefDepth, PixelH * 0.52f),
							FRotator::ZeroRotator,
							Color);
					}
				}
			}
		}
	}

	static void CommitSection(UProceduralMeshComponent* Mesh, int32 SectionIndex, FProcSectionBuffers& Buf)
	{
		if (Mesh && Buf.Vertices.Num() > 0)
		{
			Mesh->CreateMeshSection(SectionIndex, Buf.Vertices, Buf.Triangles, Buf.Normals, Buf.UVs, Buf.Colors, Buf.Tangents, false);
		}
	}
}

UFPSOperatorGearComponent::UFPSOperatorGearComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bTickEvenWhenPaused = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;

	auto CreateGearMesh = [this](const TCHAR* Name) -> TObjectPtr<UProceduralMeshComponent>
	{
		UProceduralMeshComponent* Comp = CreateDefaultSubobject<UProceduralMeshComponent>(Name);
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetCastShadow(true);
		Comp->bCastDynamicShadow = true;
		Comp->SetOwnerNoSee(false);
		Comp->SetOnlyOwnerSee(false);
		Comp->SetHiddenInGame(false);
		Comp->SetVisibility(true, true);
		AllGearMeshes.Add(Comp);
		return Comp;
	};

	HeadGearProcMesh = CreateGearMesh(TEXT("Gear_Zone1_Head"));
	TorsoGearProcMesh = CreateGearMesh(TEXT("Gear_Zone2_Torso"));
	LeftPauldronProcMesh = CreateGearMesh(TEXT("Gear_Zone3_LeftPauldron"));
	RightPauldronProcMesh = CreateGearMesh(TEXT("Gear_Zone3_RightPauldron"));
	LeftForearmProcMesh = CreateGearMesh(TEXT("Gear_Zone4_LeftForearm"));
	RightForearmProcMesh = CreateGearMesh(TEXT("Gear_Zone4_RightForearm"));
	LeftGloveProcMesh = CreateGearMesh(TEXT("Gear_Zone4_LeftGlove"));
	RightGloveProcMesh = CreateGearMesh(TEXT("Gear_Zone4_RightGlove"));
	BattleBeltProcMesh = CreateGearMesh(TEXT("Gear_Zone5_BattleBelt"));
	RightThighProcMesh = CreateGearMesh(TEXT("Gear_Zone6_RightThighHolster"));
	LeftThighProcMesh = CreateGearMesh(TEXT("Gear_Zone6_LeftThighRig"));
	LeftCalfProcMesh = CreateGearMesh(TEXT("Gear_Zone7_LeftKneeBoot"));
	RightCalfProcMesh = CreateGearMesh(TEXT("Gear_Zone7_RightKneeBoot"));
	CarbineProcMesh = CreateGearMesh(TEXT("Gear_SpecOpsCarbine"));
}

void UFPSOperatorGearComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UFPSOperatorGearComponent::InitializeGearRig(USkeletalMeshComponent* InBodyMesh, USceneComponent* InUnscaledAttachParent, bool bInIncludeCarbine)
{
	TargetBodyMesh = InBodyMesh;
	UnscaledAttachParent = InUnscaledAttachParent;
	bIncludeCarbine = bInIncludeCarbine;

	if (!UnscaledAttachParent && GetOwner())
	{
		UnscaledAttachParent = GetOwner()->GetRootComponent();
	}

	for (UProceduralMeshComponent* Comp : AllGearMeshes)
	{
		if (Comp)
		{
			Comp->SetVisibility(false, true);
			Comp->SetHiddenInGame(true);
		}
	}

	if (CarbineProcMesh)
	{
		CarbineProcMesh->SetVisibility(false, true);
		CarbineProcMesh->SetHiddenInGame(true);
	}

	// High-fidelity SK_BaseMale model contains full integrated 3D tactical gear.
	// Procedural boxes are disabled to prevent floating disjointed parts.
	CalibrateOperatorBaseBodyMaterial();

	// 3. Apply calibrated dark spec-ops PBR materials across all sections
	for (UProceduralMeshComponent* Comp : AllGearMeshes)
	{
		ApplyGearMaterialsToMesh(Comp);
	}

	// 4. Pin immediately to initial bone locations
	UpdateGearTransforms();
}

void UFPSOperatorGearComponent::GenerateRuntimeOperatorTextures()
{
	// 1. RuntimeCamoTexture: 256x256 low-contrast (75% damped) tactical ripstop micro-pattern
	// Eliminates the bright grey/white TV static noise of the old T_Camo_Digital texture
	if (!RuntimeCamoTexture)
	{
		RuntimeCamoTexture = UTexture2D::CreateTransient(256, 256, PF_B8G8R8A8);
		if (RuntimeCamoTexture)
		{
			RuntimeCamoTexture->SRGB = true;
			RuntimeCamoTexture->Filter = TF_Bilinear;
			RuntimeCamoTexture->AddressX = TA_Wrap;
			RuntimeCamoTexture->AddressY = TA_Wrap;

			FTexture2DMipMap& Mip = RuntimeCamoTexture->GetPlatformData()->Mips[0];
			uint8* Data = static_cast<uint8*>(Mip.BulkData.Lock(LOCK_READ_WRITE));
			if (Data)
			{
				for (int32 Y = 0; Y < 256; ++Y)
				{
					for (int32 X = 0; X < 256; ++X)
					{
						// Digital micro-cell hash + ripstop crosshatch weave
						const int32 CellX = X / 8;
						const int32 CellY = Y / 8;
						const uint32 Hash = ((CellX * 73856093u) ^ (CellY * 19349663u)) & 0xFFu;

						// Keep values compressed in a tight [28..64] range (75% lower contrast than 0..255!)
						uint8 Val = static_cast<uint8>(32 + (Hash % 28));

						// Subtle 4px ripstop grid thread modulation
						if ((X % 4 == 0) || (Y % 4 == 0))
						{
							Val = static_cast<uint8>(FMath::Clamp(static_cast<int32>(Val) - 5, 18, 72));
						}

						const int32 Idx = (Y * 256 + X) * 4;
						Data[Idx + 0] = Val; // B
						Data[Idx + 1] = Val; // G
						Data[Idx + 2] = Val; // R
						Data[Idx + 3] = 255; // A
					}
				}
				Mip.BulkData.Unlock();
				RuntimeCamoTexture->UpdateResource();
			}
		}
	}

	// 2. RuntimeBodyMaskTexture: 256x256 seamless zone mask eliminating the harsh neck/chest UV seam
	if (!RuntimeBodyMaskTexture)
	{
		RuntimeBodyMaskTexture = UTexture2D::CreateTransient(256, 256, PF_B8G8R8A8);
		if (RuntimeBodyMaskTexture)
		{
			RuntimeBodyMaskTexture->SRGB = false;
			RuntimeBodyMaskTexture->Filter = TF_Bilinear;
			RuntimeBodyMaskTexture->AddressX = TA_Clamp;
			RuntimeBodyMaskTexture->AddressY = TA_Clamp;

			FTexture2DMipMap& Mip = RuntimeBodyMaskTexture->GetPlatformData()->Mips[0];
			uint8* Data = static_cast<uint8*>(Mip.BulkData.Lock(LOCK_READ_WRITE));
			if (Data)
			{
				for (int32 Y = 0; Y < 256; ++Y)
				{
					for (int32 X = 0; X < 256; ++X)
					{
						// Smoothly blend R (Torso combat shirt) and G (BDU ripstop limbs) with zero hard seams
						// B (Skin) is kept 0 on the base mesh so the cranium/neck renders as a seamless matte-black balaclava
						// while Zone 1 HeadGearProcMesh renders the exposed realistic Eye/Brow facial strip!
						const int32 Idx = (Y * 256 + X) * 4;
						Data[Idx + 0] = 0;   // B (Skin = 0 -> no flesh seam on neck/chest)
						Data[Idx + 1] = 200; // G (BDU Ripstop Weave)
						Data[Idx + 2] = 55;  // R (Combat Shirt Blend)
						Data[Idx + 3] = 0;   // A
					}
				}
				Mip.BulkData.Unlock();
				RuntimeBodyMaskTexture->UpdateResource();
			}
		}
	}
}

void UFPSOperatorGearComponent::CalibrateOperatorBaseBodyMaterial()
{
	if (!TargetBodyMesh)
	{
		return;
	}

	UMaterialInstanceDynamic* BodyMID = Cast<UMaterialInstanceDynamic>(TargetBodyMesh->GetMaterial(0));
	if (!BodyMID)
	{
		BodyMID = TargetBodyMesh->CreateDynamicMaterialInstance(0);
	}
	if (!BodyMID)
	{
		return;
	}

	if (RuntimeCamoTexture)
	{
		BodyMID->SetTextureParameterValue(TEXT("Texture_Camo"), RuntimeCamoTexture);
	}
	if (RuntimeBodyMaskTexture)
	{
		BodyMID->SetTextureParameterValue(TEXT("Texture_BodyMask"), RuntimeBodyMaskTexture);
	}

	// Default Task Force Echo Full-Fidelity Textured Soldier
	FLinearColor CamoPrimary(1.0f, 1.0f, 1.0f, 1.0f);
	FLinearColor CamoSecondary(0.85f, 0.85f, 0.85f, 1.0f);
	FLinearColor CamoTertiary(0.65f, 0.65f, 0.65f, 1.0f);
	FLinearColor TorsoColor(1.0f, 1.0f, 1.0f, 1.0f);
	FLinearColor BalaclavaBase(0.12f, 0.14f, 0.18f, 1.0f);
	FLinearColor GloveBootBase(0.15f, 0.18f, 0.22f, 1.0f);
	float RoughnessBase = 0.55f;

	if (CurrentCamoIndex == 1) // Woodland Tactical
	{
		CamoPrimary = FLinearColor(0.024f, 0.036f, 0.020f, 1.0f);
		CamoSecondary = FLinearColor(0.014f, 0.022f, 0.012f, 1.0f);
		CamoTertiary = FLinearColor(0.042f, 0.052f, 0.030f, 1.0f);
		TorsoColor = FLinearColor(0.020f, 0.030f, 0.018f, 1.0f);
		RoughnessBase = 0.85f;
	}
	else if (CurrentCamoIndex == 2) // Arid Desert Spec-Ops
	{
		CamoPrimary = FLinearColor(0.110f, 0.092f, 0.068f, 1.0f);
		CamoSecondary = FLinearColor(0.072f, 0.060f, 0.044f, 1.0f);
		CamoTertiary = FLinearColor(0.145f, 0.122f, 0.090f, 1.0f);
		TorsoColor = FLinearColor(0.088f, 0.074f, 0.054f, 1.0f);
		RoughnessBase = 0.86f;
	}
	else if (CurrentCamoIndex == 3) // Stealth Blackout
	{
		CamoPrimary = FLinearColor(0.008f, 0.010f, 0.014f, 1.0f);
		CamoSecondary = FLinearColor(0.005f, 0.007f, 0.010f, 1.0f);
		CamoTertiary = FLinearColor(0.014f, 0.018f, 0.024f, 1.0f);
		TorsoColor = FLinearColor(0.007f, 0.009f, 0.012f, 1.0f);
		RoughnessBase = 0.82f;
	}

	BodyMID->SetVectorParameterValue(TEXT("Color"), CamoPrimary);
	BodyMID->SetVectorParameterValue(TEXT("BaseColor"), CamoPrimary);
	BodyMID->SetVectorParameterValue(TEXT("CamoColorPrimary"), CamoPrimary);
	BodyMID->SetVectorParameterValue(TEXT("CamoColorSecondary"), CamoSecondary);
	BodyMID->SetVectorParameterValue(TEXT("CamoColorTertiary"), CamoTertiary);
	BodyMID->SetVectorParameterValue(TEXT("TorsoColor"), TorsoColor);
	BodyMID->SetVectorParameterValue(TEXT("SkinTone"), BalaclavaBase);
	BodyMID->SetVectorParameterValue(TEXT("GearColor"), GloveBootBase);
	BodyMID->SetScalarParameterValue(TEXT("RoughnessBase"), RoughnessBase);
	BodyMID->SetScalarParameterValue(TEXT("SubsurfaceIntensity"), 0.08f);
	BodyMID->SetScalarParameterValue(TEXT("CamoTiling"), 10.0f);
	BodyMID->SetScalarParameterValue(TEXT("RipstopTiling"), 42.0f);
}

void UFPSOperatorGearComponent::BuildZone1_HeadAndComms()
{
	if (!HeadGearProcMesh) return;
	HeadGearProcMesh->ClearAllMeshSections();

	// Coordinate convention around 'head' bone (Z = 164 cm in world):
	// +Y = Forward (Nose / Visor), -Y = Back of Head, -X = Right Ear, +X = Left Ear, +Z = Up (Crown)
	FProcSectionBuffers Sec0_Cordura;  // Balaclava fabric & Velcro
	FProcSectionBuffers Sec1_Helmet;   // FAST Ballistic Helmet shell & Peltor ear cups
	FProcSectionBuffers Sec2_Gunmetal; // NVG Shroud, ARC rails, boom mic, goggle frame
	FProcSectionBuffers Sec3_Skin;     // Exposed Eye/Brow facial skin strip (#B88B72)
	FProcSectionBuffers Sec4_Optics;   // Ballistic Goggle lens / NVG phosphor optics
	FProcSectionBuffers Sec5_Stencil;  // Helmet callsign patch accent

	const bool bHasHelmet = (CurrentHeadgearIndex == 0 || CurrentHeadgearIndex == 1);
	const bool bHasNVGs = (CurrentHeadgearIndex == 1);
	const bool bIsCapMode = (CurrentHeadgearIndex == 3);

	// 1. Matte Black Tactical Balaclava Wrap & Lower-Face Mandible Guard (Hides base head/neck seam)
	AddBeveledPlateToSection(Sec0_Cordura, FVector(0.0f, 0.2f, 1.5f), FVector(9.6f, 10.6f, 9.2f), 2.2f);
	// Lower Face / Nose Bridge & Jaw Mandible Wrap
	AddBeveledPlateToSection(Sec0_Cordura, FVector(0.0f, 5.2f, -2.6f), FVector(7.8f, 6.6f, 4.8f), 1.6f);
	// Breathing Vent Ribbing over Mouth/Nose
	AddBoxToSection(Sec2_Gunmetal, FVector(0.0f, 11.4f, -2.4f), FVector(3.4f, 0.7f, 2.2f));
	// Neck Gaiter / Balaclava Skirt extending down into the Ballistic Collar (-9 cm to -2 cm)
	AddBeveledPlateToSection(Sec0_Cordura, FVector(0.0f, -0.4f, -6.8f), FVector(7.6f, 8.2f, 4.5f), 1.8f);

	// 2. Exposed Eye / Brow Facial Skin Strip (#B88B72) with Recessed Dark Tactical Eye-Orbit Shadowing
	// Main warm skin band across brow & upper cheekbones (Z = +2.4 cm to +5.4 cm)
	AddBeveledPlateToSection(Sec3_Skin, FVector(0.0f, 8.8f, 3.8f), FVector(7.2f, 2.6f, 1.85f), 0.5f, FRotator::ZeroRotator, FColor(184, 139, 114, 255));
	// Brow Ridge & Glabella
	AddBoxToSection(Sec3_Skin, FVector(0.0f, 10.9f, 4.8f), FVector(6.2f, 0.9f, 0.65f), FRotator::ZeroRotator, FColor(175, 130, 105, 255));
	// Nose Bridge Upper Crest
	AddBoxToSection(Sec3_Skin, FVector(0.0f, 11.4f, 3.2f), FVector(1.1f, 0.8f, 1.2f), FRotator(-12.0f, 0.0f, 0.0f), FColor(184, 139, 114, 255));
	// Recessed Dark Eye-Orbit Shadows & Focused Dark Irises/Pupils (Left & Right Eyes at X = +-3.1 cm)
	AddBoxToSection(Sec2_Gunmetal, FVector(-3.1f, 11.1f, 3.6f), FVector(1.45f, 0.45f, 0.55f));
	AddBoxToSection(Sec2_Gunmetal, FVector( 3.1f, 11.1f, 3.6f), FVector(1.45f, 0.45f, 0.55f));

	// 3. Tactical Ballistic Goggles / Eye Shield Frame & Refractive Dark Visor
	// When wearing FAST Helmet (Index 0), place sleek ballistic eye-shield frame at brow/eye line so the skin strip + eyes read clearly underneath
	const float GoggleZ = (CurrentHeadgearIndex == 0) ? 6.8f : 3.8f;
	const float GoggleY = (CurrentHeadgearIndex == 0) ? 12.2f : 11.3f;
	AddBeveledPlateToSection(Sec2_Gunmetal, FVector(0.0f, GoggleY, GoggleZ), FVector(8.6f, 1.8f, 1.9f), 0.5f);
	AddBoxToSection(Sec4_Optics, FVector(-3.6f, GoggleY + 1.4f, GoggleZ), FVector(3.2f, 0.5f, 1.35f));
	AddBoxToSection(Sec4_Optics, FVector( 3.6f, GoggleY + 1.4f, GoggleZ), FVector(3.2f, 0.5f, 1.35f));
	// Goggle Retention Strap wrapping around helmet/head
	AddBoxToSection(Sec0_Cordura, FVector(-9.8f, 2.0f, GoggleZ), FVector(0.8f, 8.5f, 1.1f));
	AddBoxToSection(Sec0_Cordura, FVector( 9.8f, 2.0f, GoggleZ), FVector(0.8f, 8.5f, 1.1f));

	// 4. Peltor ComTac Tactical Headset (Over-Ear Acoustic Cups + Wire Suspension + Boom Mic)
	// Right Ear Cup (-X) & Left Ear Cup (+X)
	AddBeveledPlateToSection(Sec1_Helmet, FVector(-10.8f, -0.2f, 2.0f), FVector(1.8f, 4.2f, 4.8f), 0.9f);
	AddBeveledPlateToSection(Sec1_Helmet, FVector( 10.8f, -0.2f, 2.0f), FVector(1.8f, 4.2f, 4.8f), 0.9f);
	// Outer Anodized Comms Bezel & Wire Brackets
	AddBoxToSection(Sec2_Gunmetal, FVector(-12.4f, -0.2f, 2.0f), FVector(0.5f, 2.6f, 3.2f));
	AddBoxToSection(Sec2_Gunmetal, FVector( 12.4f, -0.2f, 2.0f), FVector(0.5f, 2.6f, 3.2f));
	AddBoxToSection(Sec2_Gunmetal, FVector(-11.4f, -0.2f, 6.2f), FVector(0.5f, 0.6f, 2.8f));
	AddBoxToSection(Sec2_Gunmetal, FVector( 11.4f, -0.2f, 6.2f), FVector(0.5f, 0.6f, 2.8f));
	// Flexible Boom Microphone extending from Left Ear Cup (+X) to Mouth
	AddBoxToSection(Sec2_Gunmetal, FVector(8.2f, 6.5f, -1.6f), FVector(3.2f, 0.45f, 0.45f), FRotator(12.0f, -52.0f, 0.0f));
	AddBoxToSection(Sec2_Gunmetal, FVector(2.2f, 11.6f, -2.5f), FVector(1.1f, 0.65f, 0.65f));

	// 5. FAST Ops-Core High-Cut Ballistic Helmet Shell (or Tactical Cap)
	if (bHasHelmet)
	{
		// Main Rounded High-Cut Ballistic Dome (covers forehead Z=+5.5 cm up to Crown Z=+19.2 cm)
		AddBeveledPlateToSection(Sec1_Helmet, FVector(0.0f, 0.0f, 11.8f), FVector(11.6f, 12.8f, 6.8f), 2.6f);
		// Upper Crown Shell & Ventilation Channels
		AddBeveledPlateToSection(Sec1_Helmet, FVector(0.0f, -0.4f, 16.8f), FVector(9.2f, 10.6f, 3.2f), 1.8f);
		// Rear Occipital Nape Shield
		AddBeveledPlateToSection(Sec1_Helmet, FVector(0.0f, -9.2f, 6.2f), FVector(9.8f, 4.2f, 4.6f), 1.4f);

		// Forehead Wilcox NVG Shroud & Aluminum Mount Bracket
		AddBoxToSection(Sec2_Gunmetal, FVector(0.0f, 12.8f, 10.2f), FVector(2.8f, 1.1f, 2.8f));
		AddBoxToSection(Sec2_Gunmetal, FVector(0.0f, 13.8f, 10.4f), FVector(1.6f, 1.2f, 1.8f));

		// Left & Right Ops-Core ARC Accessory Rails
		AddBoxToSection(Sec2_Gunmetal, FVector(-11.8f, 1.2f, 8.6f), FVector(0.9f, 6.8f, 1.6f));
		AddBoxToSection(Sec2_Gunmetal, FVector( 11.8f, 1.2f, 8.6f), FVector(0.9f, 6.8f, 1.6f));
		// Side Tactical Flashlight / IR Strobe on Right Rail (-X)
		AddBoxToSection(Sec2_Gunmetal, FVector(-12.6f, 4.2f, 9.2f), FVector(0.9f, 3.2f, 0.9f));

		// Side & Crown Loop Velcro Patch Panels + Callsign Stencil Accent
		AddBoxToSection(Sec0_Cordura, FVector(-11.6f, -1.5f, 12.4f), FVector(0.4f, 5.2f, 2.2f));
		AddBoxToSection(Sec0_Cordura, FVector( 11.6f, -1.5f, 12.4f), FVector(0.4f, 5.2f, 2.2f));
		AddBoxToSection(Sec5_Stencil, FVector(-11.9f, -1.5f, 12.4f), FVector(0.2f, 2.8f, 0.8f));

		// Rear Counterweight / NVG Battery Box & Bungee Retention Cord
		AddBeveledPlateToSection(Sec0_Cordura, FVector(0.0f, -13.2f, 8.2f), FVector(4.6f, 1.8f, 3.2f), 0.6f);
		AddBoxToSection(Sec2_Gunmetal, FVector(0.0f, -14.6f, 8.2f), FVector(3.2f, 0.6f, 2.0f));

		// Chinstrap Webbing V-Harnesses down to Jaw
		AddBoxToSection(Sec0_Cordura, FVector(-8.8f, 3.8f, 0.5f), FVector(0.35f, 0.8f, 4.8f), FRotator(18.0f, 0.0f, 12.0f));
		AddBoxToSection(Sec0_Cordura, FVector( 8.8f, 3.8f, 0.5f), FVector(0.35f, 0.8f, 4.8f), FRotator(18.0f, 0.0f, -12.0f));

		// Optional GPNVG-18 Quad-Nod Panoramic Night Vision Goggles
		if (bHasNVGs)
		{
			// Bridge Arm
			AddBoxToSection(Sec2_Gunmetal, FVector(0.0f, 15.5f, 6.2f), FVector(7.8f, 1.6f, 1.2f));
			// 4 Quad-Nod Objective Tubes + Phosphor Optics
			const float TubeX[4] = { -6.2f, -2.4f, 2.4f, 6.2f };
			for (int32 t = 0; t < 4; ++t)
			{
				AddBoxToSection(Sec2_Gunmetal, FVector(TubeX[t], 17.5f, 4.4f), FVector(1.45f, 3.6f, 1.45f));
				AddBoxToSection(Sec4_Optics, FVector(TubeX[t], 21.2f, 4.4f), FVector(1.25f, 0.35f, 1.25f));
			}
		}
	}
	else if (bIsCapMode)
	{
		// Tactical Operator Baseball Cap + Curved Brim
		AddBeveledPlateToSection(Sec0_Cordura, FVector(0.0f, 0.0f, 11.2f), FVector(10.4f, 11.2f, 5.4f), 2.0f);
		AddBeveledPlateToSection(Sec0_Cordura, FVector(0.0f, 13.5f, 8.2f), FVector(7.6f, 4.8f, 0.65f), 1.2f, FRotator(-10.0f, 0.0f, 0.0f));
		AddBoxToSection(Sec5_Stencil, FVector(0.0f, 11.2f, 11.0f), FVector(3.4f, 0.3f, 1.6f));
	}

	CommitSection(HeadGearProcMesh, 0, Sec0_Cordura);
	CommitSection(HeadGearProcMesh, 1, Sec1_Helmet);
	CommitSection(HeadGearProcMesh, 2, Sec2_Gunmetal);
	CommitSection(HeadGearProcMesh, 3, Sec3_Skin);
	CommitSection(HeadGearProcMesh, 4, Sec4_Optics);
	CommitSection(HeadGearProcMesh, 5, Sec5_Stencil);
}

void UFPSOperatorGearComponent::BuildZone2_PlateCarrierAndCollar()
{
	if (!TorsoGearProcMesh) return;
	TorsoGearProcMesh->ClearAllMeshSections();

	// Coordinate convention around 'spine_03' bone (Z = 138 cm in world):
	// +Y = Chest Forward, -Y = Back, -X = Right Side, +X = Left Side, +Z = Up toward Neck
	FProcSectionBuffers Sec0_Cordura;
	FProcSectionBuffers Sec1_Plate;
	FProcSectionBuffers Sec2_Gunmetal;
	FProcSectionBuffers Sec3_Unused;
	FProcSectionBuffers Sec4_Optics;
	FProcSectionBuffers Sec5_Stencil;

	const float BulkScale = (CurrentVestIndex == 0) ? 1.0f : ((CurrentVestIndex == 1) ? 0.88f : 0.78f);

	// 1. Padded 360-Degree Ballistic Collar / Neck Yoke (Z = +12.5 cm to +19.5 cm above spine_03)
	// Completely conceals the neck-to-torso mesh seam and gives heavy armored spec-ops trap volume!
	AddBeveledPlateToSection(Sec0_Cordura, FVector(0.0f, -0.5f, 15.8f), FVector(11.8f, 10.8f, 3.6f), 2.2f);
	// Left & Right Raised Collar Flanks & Throat Protector
	AddBeveledPlateToSection(Sec1_Plate, FVector(-9.5f, 0.5f, 17.2f), FVector(2.6f, 8.2f, 3.2f), 0.8f);
	AddBeveledPlateToSection(Sec1_Plate, FVector( 9.5f, 0.5f, 17.2f), FVector(2.6f, 8.2f, 3.2f), 0.8f);
	AddBeveledPlateToSection(Sec0_Cordura, FVector(0.0f, 9.2f, 14.2f), FVector(7.8f, 2.4f, 2.6f), 0.8f);

	// 2. Front & Rear Level IV Ceramic Plate Bags (+5.2 cm chest protrusion, +4.6 cm back protrusion)
	// Inner Padded BDU / Trauma Liner (fills chest-to-plate gap completely)
	AddBeveledPlateToSection(Sec0_Cordura, FVector(0.0f, 4.5f, -1.5f), FVector(16.8f * BulkScale, 9.5f, 17.5f), 3.2f);
	// Front Octagonal Level IV Plate Bag
	AddBeveledPlateToSection(Sec0_Cordura, FVector(0.0f, 11.2f * BulkScale, -1.0f), FVector(15.6f * BulkScale, 4.8f * BulkScale, 16.8f), 3.4f);
	// Hard SAPI Ceramic Strike Face Bezel
	AddBeveledPlateToSection(Sec1_Plate, FVector(0.0f, 14.2f * BulkScale, -0.5f), FVector(13.8f * BulkScale, 2.4f * BulkScale, 14.8f), 2.8f);

	// Rear Octagonal Level IV Plate Bag & Spine Trauma Pad
	AddBeveledPlateToSection(Sec0_Cordura, FVector(0.0f, -10.2f * BulkScale, -0.5f), FVector(15.0f * BulkScale, 4.4f * BulkScale, 16.2f), 3.0f);
	AddBeveledPlateToSection(Sec1_Plate, FVector(0.0f, -12.8f * BulkScale, 0.0f), FVector(13.0f * BulkScale, 2.2f * BulkScale, 14.0f), 2.5f);
	// Rear Hydration / Breaching Pack & Drag Handle
	AddBeveledPlateToSection(Sec0_Cordura, FVector(0.0f, -15.2f * BulkScale, 1.0f), FVector(9.5f, 2.8f, 11.5f), 1.8f);

	// 3. Heavy Padded Load-Bearing Shoulder Straps over Clavicles
	AddBeveledPlateToSection(Sec0_Cordura, FVector(-10.5f, 1.0f, 15.8f), FVector(4.4f, 10.5f, 2.4f), 0.9f);
	AddBeveledPlateToSection(Sec0_Cordura, FVector( 10.5f, 1.0f, 15.8f), FVector(4.4f, 10.5f, 2.4f), 0.9f);
	AddBoxToSection(Sec2_Gunmetal, FVector(-10.5f, 7.5f, 13.2f), FVector(2.8f, 1.4f, 1.6f));
	AddBoxToSection(Sec2_Gunmetal, FVector( 10.5f, 7.5f, 13.2f), FVector(2.8f, 1.4f, 1.6f));

	// 4. Left & Right Cummerbund Side Plate Flanks & Radio/Side-SAPI Pouches
	AddBeveledPlateToSection(Sec0_Cordura, FVector(-16.2f * BulkScale, 0.5f, -5.5f), FVector(3.2f, 10.8f, 8.2f), 1.4f);
	AddBeveledPlateToSection(Sec0_Cordura, FVector( 16.2f * BulkScale, 0.5f, -5.5f), FVector(3.2f, 10.8f, 8.2f), 1.4f);
	AddBeveledPlateToSection(Sec1_Plate, FVector(-18.2f * BulkScale, 1.5f, -5.5f), FVector(1.6f, 6.8f, 6.2f), 0.8f);
	AddBeveledPlateToSection(Sec1_Plate, FVector( 18.2f * BulkScale, 1.5f, -5.5f), FVector(1.6f, 6.8f, 6.2f), 0.8f);

	// 5. Upper-Chest Velcro ID Placard with Crisp 3D "NOBLE FERMI" Tactical Stencil Lettering (matches menu-prop.jpeg!)
	const float PlacardY = 16.4f * BulkScale;
	AddBeveledPlateToSection(Sec2_Gunmetal, FVector(0.0f, PlacardY, 6.2f), FVector(10.8f, 0.8f, 3.2f), 0.5f);
	// Top & Bottom Stencil Border Bars
	AddBoxToSection(Sec5_Stencil, FVector(0.0f, PlacardY + 0.75f, 8.6f), FVector(9.6f, 0.2f, 0.18f));
	AddBoxToSection(Sec5_Stencil, FVector(0.0f, PlacardY + 0.75f, 3.8f), FVector(9.6f, 0.2f, 0.18f));
	// Crisp 3D Extruded "NOBLE FERMI" Stencil Text across the Upper Chest Placard
	Add3DStencilTextToSection(
		Sec5_Stencil,
		TEXT("NOBLE FERMI"),
		FVector(0.0f, PlacardY + 0.82f, 6.2f),
		1.32f,
		2.65f,
		0.24f,
		FColor(245, 205, 75, 255));

	// 6. Triple 5.56/7.62 Rifle Magazine Pouches across Lower Chest (spine_02 level: Z = -6.5 cm)
	const float MagX[3] = { -8.4f, 0.0f, 8.4f };
	const float PouchY = 16.6f * BulkScale;
	for (int32 m = 0; m < 3; ++m)
	{
		// Cordura/Kydex Open-Top Mag Pouch Body
		AddBeveledPlateToSection(Sec0_Cordura, FVector(MagX[m], PouchY, -7.2f), FVector(3.6f, 2.6f, 6.4f), 0.7f);
		// MOLLE Retention Band
		AddBoxToSection(Sec1_Plate, FVector(MagX[m], PouchY + 2.2f, -6.5f), FVector(3.75f, 0.6f, 2.2f));
		// Protruding Polymer Rifle Magazine + Ranger Pull Tab
		AddBeveledPlateToSection(Sec2_Gunmetal, FVector(MagX[m], PouchY + 0.4f, -1.2f), FVector(2.8f, 1.6f, 2.8f), 0.4f, FRotator(0.0f, 0.0f, -4.0f));
		AddBoxToSection(Sec1_Plate, FVector(MagX[m], PouchY + 0.6f, 1.6f), FVector(1.8f, 1.2f, 0.6f));
	}

	// 7. Tactical Comms PTT Box, Radio & Whip Antenna on Left Upper Chest (+X) + Tourniquet on Right (-X)
	AddBeveledPlateToSection(Sec2_Gunmetal, FVector(9.2f, PlacardY + 0.4f, 11.2f), FVector(2.2f, 1.4f, 2.6f), 0.4f);
	// AN/PRC-152 Tactical Radio in Left Cummerbund Pouch + Whip Antenna rising behind left shoulder
	AddBoxToSection(Sec2_Gunmetal, FVector(13.8f * BulkScale, 9.5f, -2.0f), FVector(2.2f, 2.0f, 5.5f));
	AddBoxToSection(Sec2_Gunmetal, FVector(13.5f * BulkScale, 7.5f, 12.5f), FVector(0.35f, 0.35f, 11.0f), FRotator(-8.0f, 0.0f, 4.0f));
	// CAT Tourniquet & Chemlights on Right Shoulder/Chest (-X)
	AddBeveledPlateToSection(Sec2_Gunmetal, FVector(-9.5f, PlacardY + 0.3f, 11.0f), FVector(1.8f, 1.2f, 2.8f), 0.4f);

	CommitSection(TorsoGearProcMesh, 0, Sec0_Cordura);
	CommitSection(TorsoGearProcMesh, 1, Sec1_Plate);
	CommitSection(TorsoGearProcMesh, 2, Sec2_Gunmetal);
	CommitSection(TorsoGearProcMesh, 3, Sec3_Unused);
	CommitSection(TorsoGearProcMesh, 4, Sec4_Optics);
	CommitSection(TorsoGearProcMesh, 5, Sec5_Stencil);
}

void UFPSOperatorGearComponent::BuildZone3_ShoulderPauldrons()
{
	// Coordinate convention along 'upperarm_l' and 'upperarm_r':
	// Origin (0,0,0) is at the Shoulder joint; -Z goes down the humerus toward the Elbow (~28 cm).
	// For Left Arm (LeftPauldronProcMesh): +X is Outward (Lateral Deltoid).
	// For Right Arm (RightPauldronProcMesh): -X is Outward (Lateral Deltoid).
	auto BuildPauldron = [](UProceduralMeshComponent* Mesh, float OutwardSign)
	{
		if (!Mesh) return;
		Mesh->ClearAllMeshSections();

		FProcSectionBuffers Sec0_Cordura;
		FProcSectionBuffers Sec1_Plate;
		FProcSectionBuffers Sec2_Gunmetal;
		FProcSectionBuffers Sec3_Unused;
		FProcSectionBuffers Sec4_Unused;
		FProcSectionBuffers Sec5_Stencil;

		// 1. BDU Combat Shirt Upper-Sleeve Fabric Bulk & Zippered Shoulder Pocket (eliminates thin skin-tight arm)
		AddBeveledPlateToSection(Sec0_Cordura, FVector(OutwardSign * 0.6f, 0.0f, -12.5f), FVector(6.8f, 6.8f, 11.5f), 2.0f);

		// 2. Articulated Deltoid Ballistic Pauldron Shell (+3.8 cm lateral shoulder widening)
		AddBeveledPlateToSection(Sec0_Cordura, FVector(OutwardSign * 4.2f, 0.0f, -8.5f), FVector(3.4f, 7.6f, 8.8f), 1.6f,
			FRotator(0.0f, 0.0f, OutwardSign * -10.0f));
		// Hard Polymer Outer Deltoid Strike Plate
		AddBeveledPlateToSection(Sec1_Plate, FVector(OutwardSign * 6.2f, 0.0f, -8.2f), FVector(1.8f, 6.4f, 7.2f), 1.2f,
			FRotator(0.0f, 0.0f, OutwardSign * -10.0f));

		// 3. Velcro Unit Patch Bezel & Tactical Callsign Accent Bar
		AddBoxToSection(Sec2_Gunmetal, FVector(OutwardSign * 7.6f, 0.0f, -7.8f), FVector(0.5f, 4.4f, 3.2f),
			FRotator(0.0f, 0.0f, OutwardSign * -10.0f));
		AddBoxToSection(Sec5_Stencil, FVector(OutwardSign * 8.0f, 0.0f, -7.8f), FVector(0.25f, 3.2f, 0.55f),
			FRotator(0.0f, 0.0f, OutwardSign * -10.0f));

		// 4. Horizontal Bicep Retention Strap & Buckle
		AddBeveledPlateToSection(Sec2_Gunmetal, FVector(OutwardSign * 0.8f, 0.0f, -15.2f), FVector(7.2f, 7.2f, 1.4f), 1.6f);

		CommitSection(Mesh, 0, Sec0_Cordura);
		CommitSection(Mesh, 1, Sec1_Plate);
		CommitSection(Mesh, 2, Sec2_Gunmetal);
		CommitSection(Mesh, 3, Sec3_Unused);
		CommitSection(Mesh, 4, Sec4_Unused);
		CommitSection(Mesh, 5, Sec5_Stencil);
	};

	BuildPauldron(LeftPauldronProcMesh, +1.0f);
	BuildPauldron(RightPauldronProcMesh, -1.0f);
}

void UFPSOperatorGearComponent::BuildZone4_ForearmsAndGloves()
{
	// Forearms: Origin at Elbow ('lowerarm_l/r'), -Z goes down toward Wrist ('hand_l/r', ~26 cm)
	auto BuildForearm = [](UProceduralMeshComponent* Mesh, float OutwardSign, bool bHasWristGPS)
	{
		if (!Mesh) return;
		Mesh->ClearAllMeshSections();

		FProcSectionBuffers Sec0_Cordura;
		FProcSectionBuffers Sec1_Plate;
		FProcSectionBuffers Sec2_Gunmetal;
		FProcSectionBuffers Sec3_Unused;
		FProcSectionBuffers Sec4_Optics;

		// 1. Rolled/Bloused BDU Combat Sleeve Cuff & Elbow Pad Bulk
		AddBeveledPlateToSection(Sec0_Cordura, FVector(0.0f, 0.0f, -9.5f), FVector(5.6f, 5.6f, 9.2f), 1.8f);
		// Articulated Neoprene/Polymer Elbow Guard Cap (-Y posterior elbow at Z = -2.5 cm)
		AddBeveledPlateToSection(Sec1_Plate, FVector(0.0f, -3.8f, -2.8f), FVector(4.4f, 2.8f, 4.6f), 1.0f);

		// 2. Forearm Vambrace / Gauntlet Cuff (Z = -14 cm to -22 cm)
		AddBeveledPlateToSection(Sec1_Plate, FVector(OutwardSign * 0.8f, 0.0f, -17.5f), FVector(4.8f, 4.8f, 4.8f), 1.4f);

		// 3. Garmin Foretrex / Tactical Wrist GPS Display on Left Wrist
		if (bHasWristGPS)
		{
			AddBeveledPlateToSection(Sec2_Gunmetal, FVector(OutwardSign * 4.6f, 1.2f, -19.2f), FVector(1.4f, 3.2f, 2.4f), 0.4f);
			AddBoxToSection(Sec4_Optics, FVector(OutwardSign * 5.8f, 1.2f, -19.2f), FVector(0.35f, 2.4f, 1.7f));
		}

		CommitSection(Mesh, 0, Sec0_Cordura);
		CommitSection(Mesh, 1, Sec1_Plate);
		CommitSection(Mesh, 2, Sec2_Gunmetal);
		CommitSection(Mesh, 3, Sec3_Unused);
		CommitSection(Mesh, 4, Sec4_Optics);
	};

	BuildForearm(LeftForearmProcMesh, +1.0f, true);
	BuildForearm(RightForearmProcMesh, -1.0f, false);

	// Gloves: Origin at Wrist ('hand_l/r'), -Z goes along palm/fingers (~10 cm)
	auto BuildGlove = [](UProceduralMeshComponent* Mesh, float OutwardSign)
	{
		if (!Mesh) return;
		Mesh->ClearAllMeshSections();

		FProcSectionBuffers Sec0_Cordura;
		FProcSectionBuffers Sec1_Plate;
		FProcSectionBuffers Sec2_Gunmetal;

		// 1. Tactical Glove Gauntlet Wrist Cuff
		AddBeveledPlateToSection(Sec0_Cordura, FVector(0.0f, 0.0f, -1.8f), FVector(4.2f, 3.6f, 2.8f), 0.9f);
		// 2. Reinforced Padded Palm & Back-of-Hand Shell
		AddBeveledPlateToSection(Sec1_Plate, FVector(0.0f, 0.0f, -5.8f), FVector(4.4f, 3.2f, 3.8f), 0.9f);
		// 3. Molded Carbon-Fiber Knuckle Guard Bridge
		AddBeveledPlateToSection(Sec2_Gunmetal, FVector(OutwardSign * 2.2f, 0.4f, -7.8f), FVector(2.2f, 3.4f, 1.4f), 0.4f);

		CommitSection(Mesh, 0, Sec0_Cordura);
		CommitSection(Mesh, 1, Sec1_Plate);
		CommitSection(Mesh, 2, Sec2_Gunmetal);
	};

	BuildGlove(LeftGloveProcMesh, +1.0f);
	BuildGlove(RightGloveProcMesh, -1.0f);
}

void UFPSOperatorGearComponent::BuildZone5_BattleBeltAndGroin()
{
	if (!BattleBeltProcMesh) return;
	BattleBeltProcMesh->ClearAllMeshSections();

	// Coordinate convention around 'pelvis' bone (Z = 96 cm in world):
	// +Y = Front (Groin / Belt Buckle), -Y = Rear (Dump Pouch / IFAK), -X = Right Hip, +X = Left Hip, +Z = Up
	FProcSectionBuffers Sec0_Cordura;
	FProcSectionBuffers Sec1_Plate;
	FProcSectionBuffers Sec2_Gunmetal;

	// 1. BDU Hip / Upper Pelvis Fabric Bulk (eliminates vacuum-sealed waist/hip transition)
	AddBeveledPlateToSection(Sec0_Cordura, FVector(0.0f, 0.0f, -2.5f), FVector(16.2f, 11.8f, 7.2f), 2.8f);

	// 2. Heavy 2-Piece MOLLE Tactical War-Belt Ring wrapping the Waist (Z = +1.0 cm to +7.5 cm)
	AddBeveledPlateToSection(Sec0_Cordura, FVector(0.0f, 0.2f, 3.8f), FVector(17.4f, 12.6f, 3.8f), 3.2f);
	// Outer Laser-Cut MOLLE Webbing Belt Band
	AddBeveledPlateToSection(Sec1_Plate, FVector(0.0f, 0.2f, 3.8f), FVector(18.0f, 13.0f, 2.4f), 3.4f);
	// Front Cobra Quick-Release Metal Buckle
	AddBoxToSection(Sec2_Gunmetal, FVector(0.0f, 13.2f, 3.8f), FVector(3.4f, 1.2f, 2.8f));

	// 3. Trapezoidal Ballistic Groin / Lap Protector Pad (Hangs at front pelvis seam: Z = -7.5 cm, Y = +12.2 cm)
	AddBeveledPlateToSection(Sec0_Cordura, FVector(0.0f, 11.8f, -6.8f), FVector(9.6f, 2.2f, 7.6f), 1.8f, FRotator(6.0f, 0.0f, 0.0f));
	AddBeveledPlateToSection(Sec1_Plate, FVector(0.0f, 13.0f, -6.8f), FVector(7.8f, 1.2f, 5.8f), 1.4f, FRotator(6.0f, 0.0f, 0.0f));

	// 4. Left Hip (+X): Double Kydex Pistol Mag Pouches & Dump Pouch
	AddBeveledPlateToSection(Sec2_Gunmetal, FVector(16.5f, 6.2f, 4.2f), FVector(2.2f, 2.2f, 4.4f), 0.5f);
	AddBeveledPlateToSection(Sec2_Gunmetal, FVector(17.8f, 1.5f, 4.2f), FVector(2.2f, 2.2f, 4.4f), 0.5f);
	// Left Rear Rolled Nylon Dump Pouch
	AddBeveledPlateToSection(Sec0_Cordura, FVector(14.2f, -9.8f, 1.5f), FVector(4.2f, 3.6f, 6.2f), 1.2f);

	// 5. Right Hip (-X): Flashbang / Frag Grenade Pouches & Drop-Holster Belt Hanger Strap
	AddBeveledPlateToSection(Sec0_Cordura, FVector(-16.5f, 5.8f, 4.0f), FVector(2.8f, 2.8f, 4.6f), 0.8f);
	AddBoxToSection(Sec2_Gunmetal, FVector(-17.6f, 0.0f, -2.5f), FVector(0.8f, 2.8f, 5.5f));

	// 6. Center Rear (-Y): Blowout Trauma IFAK Med-Pouch
	AddBeveledPlateToSection(Sec0_Cordura, FVector(0.0f, -13.6f, 3.8f), FVector(6.8f, 3.4f, 4.4f), 1.2f);
	AddBoxToSection(Sec2_Gunmetal, FVector(0.0f, -16.8f, 3.8f), FVector(4.5f, 0.6f, 2.2f));

	CommitSection(BattleBeltProcMesh, 0, Sec0_Cordura);
	CommitSection(BattleBeltProcMesh, 1, Sec1_Plate);
	CommitSection(BattleBeltProcMesh, 2, Sec2_Gunmetal);
}

void UFPSOperatorGearComponent::BuildZone6_ThighHolsterAndRig()
{
	// Coordinate convention along 'thigh_r' and 'thigh_l':
	// Origin (0,0,0) is at Hip joint; -Z goes down femur toward Knee (~42 cm).
	// For Right Thigh ('thigh_r'): -X is Outer Right Thigh.
	// For Left Thigh ('thigh_l'): +X is Outer Left Thigh.

	// 1. Right Thigh: BDU Trouser Cargo Bulk + Drop-Leg Kydex Sidearm Holster & Tactical 9mm Pistol
	if (RightThighProcMesh)
	{
		RightThighProcMesh->ClearAllMeshSections();
		FProcSectionBuffers Sec0_Cordura;
		FProcSectionBuffers Sec1_Plate;
		FProcSectionBuffers Sec2_Gunmetal;

		// BDU Trouser Thigh Fabric Bulk (adds realistic combat pant fullness)
		AddBeveledPlateToSection(Sec0_Cordura, FVector(0.0f, 0.0f, -18.5f), FVector(8.6f, 8.8f, 16.5f), 2.4f);

		// Vertical Drop-Leg Hanger Webbing from Battle Belt
		AddBoxToSection(Sec0_Cordura, FVector(-8.6f, 0.0f, -7.5f), FVector(0.6f, 2.8f, 7.5f));

		// Dual Horizontal Elastic Thigh Retention Straps (wrapping 360 around thigh at Z = -13 cm and Z = -22 cm)
		AddBeveledPlateToSection(Sec2_Gunmetal, FVector(0.0f, 0.0f, -13.2f), FVector(9.2f, 9.2f, 1.6f), 2.2f);
		AddBeveledPlateToSection(Sec2_Gunmetal, FVector(0.0f, 0.0f, -21.8f), FVector(8.8f, 8.8f, 1.6f), 2.2f);

		// Molded Kydex Drop-Leg Shroud & Sidearm Holster Shell on Outer Right Thigh (-X)
		AddBeveledPlateToSection(Sec1_Plate, FVector(-9.4f, 0.5f, -17.5f), FVector(1.6f, 6.2f, 7.8f), 1.0f);
		AddBeveledPlateToSection(Sec1_Plate, FVector(-11.2f, 0.8f, -18.2f), FVector(2.2f, 3.8f, 6.8f), 0.8f);

		// Holstered Tactical 9mm Sidearm (Slide, Frame, Pistol Grip & Extended Mag Baseplate protruding up)
		AddBeveledPlateToSection(Sec2_Gunmetal, FVector(-11.4f, 1.2f, -12.5f), FVector(1.4f, 2.6f, 3.2f), 0.4f);
		// Angled Pistol Grip & Mag Baseplate
		AddBoxToSection(Sec2_Gunmetal, FVector(-11.4f, -1.8f, -9.8f), FVector(1.3f, 3.4f, 2.0f), FRotator(-18.0f, 0.0f, 0.0f));
		AddBoxToSection(Sec1_Plate, FVector(-11.4f, -4.8f, -8.8f), FVector(1.4f, 0.8f, 1.8f), FRotator(-18.0f, 0.0f, 0.0f));

		CommitSection(RightThighProcMesh, 0, Sec0_Cordura);
		CommitSection(RightThighProcMesh, 1, Sec1_Plate);
		CommitSection(RightThighProcMesh, 2, Sec2_Gunmetal);
	}

	// 2. Left Thigh: BDU Trouser Cargo Bulk + Drop-Leg Tactical Sub-Load / Breaching Pouch
	if (LeftThighProcMesh)
	{
		LeftThighProcMesh->ClearAllMeshSections();
		FProcSectionBuffers Sec0_Cordura;
		FProcSectionBuffers Sec1_Plate;
		FProcSectionBuffers Sec2_Gunmetal;

		// BDU Trouser Thigh Fabric Bulk
		AddBeveledPlateToSection(Sec0_Cordura, FVector(0.0f, 0.0f, -18.5f), FVector(8.6f, 8.8f, 16.5f), 2.4f);

		// Dual Horizontal Elastic Thigh Retention Straps
		AddBeveledPlateToSection(Sec2_Gunmetal, FVector(0.0f, 0.0f, -14.0f), FVector(9.2f, 9.2f, 1.5f), 2.2f);
		AddBeveledPlateToSection(Sec2_Gunmetal, FVector(0.0f, 0.0f, -22.5f), FVector(8.8f, 8.8f, 1.5f), 2.2f);

		// Outer Left Thigh (+X) Tactical Cargo / Sub-Load Utility Rig + Quick-Release Buckles
		AddBeveledPlateToSection(Sec0_Cordura, FVector(9.8f, 0.5f, -18.2f), FVector(2.8f, 6.4f, 7.2f), 1.2f);
		AddBoxToSection(Sec1_Plate, FVector(12.2f, 0.5f, -16.5f), FVector(0.8f, 5.2f, 2.8f));
		AddBoxToSection(Sec2_Gunmetal, FVector(12.6f, -1.8f, -19.5f), FVector(0.6f, 1.2f, 2.2f));
		AddBoxToSection(Sec2_Gunmetal, FVector(12.6f,  2.8f, -19.5f), FVector(0.6f, 1.2f, 2.2f));

		CommitSection(LeftThighProcMesh, 0, Sec0_Cordura);
		CommitSection(LeftThighProcMesh, 1, Sec1_Plate);
		CommitSection(LeftThighProcMesh, 2, Sec2_Gunmetal);
	}
}

void UFPSOperatorGearComponent::BuildZone7_KneePadsAndBoots()
{
	// Coordinate convention along 'calf_l' and 'calf_r':
	// Origin (0,0,0) is at the Knee joint (Z = 52 cm in world); -Z goes down the tibia to the Ankle ('foot_l/r', ~42 cm).
	// +Y is Anterior (Front of Knee / Shin / Boot Toe).
	auto BuildCalfAndBoot = [](UProceduralMeshComponent* Mesh)
	{
		if (!Mesh) return;
		Mesh->ClearAllMeshSections();

		FProcSectionBuffers Sec0_Cordura;
		FProcSectionBuffers Sec1_Plate;
		FProcSectionBuffers Sec2_Gunmetal;

		// 1. Articulated Hard-Shell Tactical Knee Pad at Upper Calf / Knee Joint (Z = +1.5 cm to -8.5 cm, Front +Y)
		// Neoprene Under-Pad Surround
		AddBeveledPlateToSection(Sec0_Cordura, FVector(0.0f, 4.8f, -3.2f), FVector(6.6f, 4.2f, 6.2f), 1.6f);
		// Molded Arc'teryx / Crye Hard Polymer Knee Cap Strike Shell
		AddBeveledPlateToSection(Sec1_Plate, FVector(0.0f, 7.6f, -3.2f), FVector(5.4f, 2.4f, 4.8f), 1.2f);
		AddBoxToSection(Sec2_Gunmetal, FVector(0.0f, 9.6f, -3.2f), FVector(3.6f, 0.6f, 2.6f));
		// Upper & Lower Elastic Knee Pad Straps wrapping behind the knee fossa (-Y)
		AddBeveledPlateToSection(Sec2_Gunmetal, FVector(0.0f, 0.0f, -0.5f), FVector(6.8f, 6.8f, 1.2f), 1.6f);
		AddBeveledPlateToSection(Sec2_Gunmetal, FVector(0.0f, 0.0f, -6.2f), FVector(6.4f, 6.4f, 1.2f), 1.6f);

		// 2. Bloused BDU Combat Trouser Lower-Leg Cuff (Z = -8 cm to -28 cm)
		AddBeveledPlateToSection(Sec0_Cordura, FVector(0.0f, -0.4f, -18.0f), FVector(6.6f, 6.8f, 11.2f), 2.0f);

		// 3. Heavy Military Combat Boot Cuff, Ankle Support & Reinforced Toe/Heel (Z = -27 cm to -44 cm)
		// Boot Upper Collar & Tongue
		AddBeveledPlateToSection(Sec1_Plate, FVector(0.0f, 0.2f, -33.5f), FVector(5.8f, 6.2f, 6.8f), 1.6f);
		// Speed-Lace Eyelets & Shin Guard Strip
		AddBoxToSection(Sec2_Gunmetal, FVector(0.0f, 6.2f, -32.5f), FVector(2.4f, 0.8f, 5.2f));
		// Heavy Lugged Sole, Heel Counter & Armored Toe Cap extending forward (+Y) at Foot level (Z = -39.5 cm)
		AddBeveledPlateToSection(Sec2_Gunmetal, FVector(0.0f, 4.5f, -40.2f), FVector(5.8f, 11.2f, 3.6f), 1.4f);
		AddBeveledPlateToSection(Sec1_Plate, FVector(0.0f, 11.5f, -39.5f), FVector(5.2f, 4.8f, 3.2f), 1.2f);

		CommitSection(Mesh, 0, Sec0_Cordura);
		CommitSection(Mesh, 1, Sec1_Plate);
		CommitSection(Mesh, 2, Sec2_Gunmetal);
	};

	BuildCalfAndBoot(LeftCalfProcMesh);
	BuildCalfAndBoot(RightCalfProcMesh);
}

void UFPSOperatorGearComponent::BuildSpecOpsCarbine()
{
	if (!CarbineProcMesh) return;
	CarbineProcMesh->ClearAllMeshSections();

	// Local Coordinate Convention of CarbineProcMesh:
	// Origin (0,0,0) = Right-Hand Pistol Grip Contact Point ('hand_r')!
	// +X = Forward along the Barrel toward Muzzle
	// +Y = Right side of Receiver (Ejection Port)
	// +Z = Up toward Optic Rail
	FProcSectionBuffers Sec0_Furniture; // Matte Polymer Stock, Grip, Handguard Covers, PMAG
	FProcSectionBuffers Sec1_Receiver;  // Upper/Lower Anodized Receiver & M-LOK Handguard
	FProcSectionBuffers Sec2_Barrel;    // Steel Barrel, Suppressor, Picatinny Rail, PEQ-15
	FProcSectionBuffers Sec3_Unused;
	FProcSectionBuffers Sec4_OpticLens; // Glowing Holographic Reticle Lens
	FProcSectionBuffers Sec5_Stencil;   // Weapon Serial / Accent Marking

	// 1. Ergonomic Textured Pistol Grip at Origin (0, 0, -2.5 cm) -> Gripped by Right Hand ('hand_r')
	AddBeveledPlateToSection(Sec0_Furniture, FVector(-1.2f, 0.0f, -2.8f), FVector(2.4f, 1.55f, 5.6f), 0.5f, FRotator(-16.0f, 0.0f, 0.0f));
	// Trigger Guard & Curved Match Trigger
	AddBoxToSection(Sec2_Barrel, FVector(3.2f, 0.0f, -0.8f), FVector(2.8f, 0.8f, 1.4f));

	// 2. Billet Upper & Lower Tactical Receiver (X = -4 cm to +18 cm, Z = +1.5 cm to +7.2 cm)
	AddBeveledPlateToSection(Sec1_Receiver, FVector(6.5f, 0.0f, 4.2f), FVector(11.5f, 2.2f, 3.6f), 0.6f);
	// Flared Magazine Well & Brass Deflector
	AddBeveledPlateToSection(Sec1_Receiver, FVector(12.2f, 0.0f, 1.2f), FVector(4.2f, 2.35f, 2.8f), 0.5f);
	// Full-Length Top Picatinny Rail (X = -3 cm to +42 cm, Z = +8.0 cm)
	AddBoxToSection(Sec2_Barrel, FVector(19.5f, 0.0f, 8.0f), FVector(22.5f, 1.35f, 0.65f));

	// 3. Collapsible Spec-Ops Crane Buttstock & Buffer Tube (X = -4 cm to -23 cm)
	AddBoxToSection(Sec2_Barrel, FVector(-12.5f, 0.0f, 5.2f), FVector(9.5f, 1.45f, 1.45f));
	AddBeveledPlateToSection(Sec0_Furniture, FVector(-17.5f, 0.0f, 3.6f), FVector(6.8f, 2.4f, 5.4f), 0.9f);
	// Rubberized Non-Slip Buttpad
	AddBoxToSection(Sec2_Barrel, FVector(-24.2f, 0.0f, 3.4f), FVector(0.8f, 2.45f, 5.8f));

	// 4. Curved 30-Round Polymer PMAG Magazine (X = +12.5 cm, Z = -2 cm down to -19 cm)
	AddBeveledPlateToSection(Sec0_Furniture, FVector(12.8f, 0.0f, -6.2f), FVector(3.4f, 1.5f, 5.2f), 0.4f, FRotator(8.0f, 0.0f, 0.0f));
	AddBeveledPlateToSection(Sec0_Furniture, FVector(14.6f, 0.0f, -14.2f), FVector(3.4f, 1.5f, 5.2f), 0.4f, FRotator(18.0f, 0.0f, 0.0f));
	AddBoxToSection(Sec2_Barrel, FVector(16.2f, 0.0f, -19.2f), FVector(3.6f, 1.65f, 0.8f), FRotator(18.0f, 0.0f, 0.0f));

	// 5. Free-Float Octagonal M-LOK Tactical Handguard (X = +18 cm to +43 cm) -> Gripped by Left Hand ('hand_l') at X = +26 cm!
	AddBeveledPlateToSection(Sec1_Receiver, FVector(30.5f, 0.0f, 5.0f), FVector(12.8f, 2.25f, 2.65f), 0.7f);
	// Angled Foregrip (AFG) / Hand-Stop at X = +26.5 cm, Z = +1.8 cm (exact Left-Hand IK target!)
	AddBeveledPlateToSection(Sec0_Furniture, FVector(26.5f, 0.0f, 1.6f), FVector(4.6f, 1.7f, 1.4f), 0.4f, FRotator(12.0f, 0.0f, 0.0f));
	// Rail Panels on Left/Right Sides of Handguard
	AddBoxToSection(Sec0_Furniture, FVector(28.0f, -2.35f, 5.0f), FVector(7.5f, 0.35f, 1.1f));
	AddBoxToSection(Sec0_Furniture, FVector(28.0f,  2.35f, 5.0f), FVector(7.5f, 0.35f, 1.1f));

	// 6. Heavy Match Barrel & SOCOM Quick-Detach Tactical Suppressor (X = +43 cm to +62 cm)
	AddBoxToSection(Sec2_Barrel, FVector(46.0f, 0.0f, 5.2f), FVector(4.5f, 0.95f, 0.95f));
	// QD Suppressor Canister
	AddBeveledPlateToSection(Sec2_Barrel, FVector(55.5f, 0.0f, 5.2f), FVector(7.2f, 1.85f, 1.85f), 0.5f);

	// 7. EOTech EXPS3 Holographic Weapon Sight (X = +7.5 cm, Z = +10.6 cm)
	AddBeveledPlateToSection(Sec2_Barrel, FVector(7.5f, 0.0f, 10.6f), FVector(4.2f, 2.45f, 2.4f), 0.5f);
	AddBoxToSection(Sec4_OpticLens, FVector(7.5f, 0.0f, 10.8f), FVector(0.4f, 1.65f, 1.45f));

	// 8. AN/PEQ-15 Tactical Laser Aiming Module on Forward Top Rail (X = +36.5 cm, Z = +9.6 cm)
	AddBeveledPlateToSection(Sec0_Furniture, FVector(36.5f, 0.0f, 9.6f), FVector(4.4f, 2.6f, 1.35f), 0.4f);
	// Weapon-Mounted Tactical Flashlight on Right M-LOK Rail (+Y)
	AddBeveledPlateToSection(Sec2_Barrel, FVector(37.5f, 2.8f, 5.6f), FVector(4.8f, 1.25f, 1.25f), 0.35f);
	// Backup Folding Iron Sights (Front & Rear)
	AddBoxToSection(Sec2_Barrel, FVector(-1.5f, 0.0f, 9.2f), FVector(1.2f, 1.6f, 0.8f));
	AddBoxToSection(Sec2_Barrel, FVector(41.2f, 0.0f, 9.2f), FVector(1.2f, 1.6f, 0.8f));

	CommitSection(CarbineProcMesh, 0, Sec0_Furniture);
	CommitSection(CarbineProcMesh, 1, Sec1_Receiver);
	CommitSection(CarbineProcMesh, 2, Sec2_Barrel);
	CommitSection(CarbineProcMesh, 3, Sec3_Unused);
	CommitSection(CarbineProcMesh, 4, Sec4_OpticLens);
	CommitSection(CarbineProcMesh, 5, Sec5_Stencil);
}

void UFPSOperatorGearComponent::ApplyGearMaterialsToMesh(UProceduralMeshComponent* ProcMesh)
{
	if (!ProcMesh) return;

	UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (!BaseMat)
	{
		BaseMat = UMaterial::GetDefaultMaterial(MD_Surface);
	}
	if (!BaseMat) return;

	// Calibrated Dark Military Spec-Ops Palette (matching menu-prop.jpeg)
	FLinearColor CorduraColor(0.015f, 0.020f, 0.026f, 1.0f);  // #131820 Matte Cordura / BDU Bulk
	FLinearColor PlateColor(0.022f, 0.028f, 0.036f, 1.0f);    // #1A2028 Level IV Ceramic / Polymer Armor
	FLinearColor GunmetalColor(0.009f, 0.012f, 0.016f, 1.0f); // #0C0F14 Anodized Gunmetal / Buckles / Rails
	FLinearColor SkinColor(0.46f, 0.28f, 0.19f, 1.0f);        // #B88B72 Exposed Eye/Brow Facial Skin Strip
	FLinearColor OpticsColor(0.006f, 0.010f, 0.015f, 1.0f);   // Glossy Dark Visor / Goggles
	FLinearColor StencilColor(0.88f, 0.68f, 0.16f, 1.0f);     // #F2C84B Crisp Yellow-Amber Tactical Stencil

	if (CurrentCamoIndex == 1) // Woodland Tactical
	{
		CorduraColor = FLinearColor(0.020f, 0.028f, 0.016f, 1.0f);
		PlateColor = FLinearColor(0.028f, 0.036f, 0.022f, 1.0f);
	}
	else if (CurrentCamoIndex == 2) // Arid Desert Spec-Ops
	{
		CorduraColor = FLinearColor(0.095f, 0.078f, 0.056f, 1.0f);
		PlateColor = FLinearColor(0.125f, 0.105f, 0.076f, 1.0f);
	}
	else if (CurrentCamoIndex == 3) // Stealth Blackout
	{
		CorduraColor = FLinearColor(0.008f, 0.010f, 0.014f, 1.0f);
		PlateColor = FLinearColor(0.012f, 0.015f, 0.020f, 1.0f);
	}

	if (ProcMesh == HeadGearProcMesh && CurrentHeadgearIndex == 1)
	{
		// Phosphor-green NVG objective glow when Quad-Nods are active
		OpticsColor = FLinearColor(0.08f, 0.85f, 0.28f, 1.0f);
	}

	if (ProcMesh == CarbineProcMesh)
	{
		// Apply Weapon Finish to CarbineProcMesh
		if (CurrentWeaponFinishIndex == 0) // Factory Anodized Gunmetal Steel
		{
			CorduraColor = FLinearColor(0.014f, 0.018f, 0.022f, 1.0f);
			PlateColor = FLinearColor(0.020f, 0.025f, 0.032f, 1.0f);
			GunmetalColor = FLinearColor(0.010f, 0.013f, 0.018f, 1.0f);
		}
		else if (CurrentWeaponFinishIndex == 1) // Desert Tan Cerakote
		{
			CorduraColor = FLinearColor(0.14f, 0.11f, 0.075f, 1.0f);
			PlateColor = FLinearColor(0.22f, 0.18f, 0.12f, 1.0f);
			GunmetalColor = FLinearColor(0.04f, 0.035f, 0.03f, 1.0f);
		}
		else if (CurrentWeaponFinishIndex == 2) // Carbon Fiber Spec-Ops
		{
			CorduraColor = FLinearColor(0.008f, 0.012f, 0.016f, 1.0f);
			PlateColor = FLinearColor(0.012f, 0.018f, 0.024f, 1.0f);
			GunmetalColor = FLinearColor(0.0f, 0.38f, 0.48f, 1.0f);
		}
		else if (CurrentWeaponFinishIndex == 3) // 24K Gold & Opal Damascene
		{
			CorduraColor = FLinearColor(0.02f, 0.06f, 0.09f, 1.0f);
			PlateColor = FLinearColor(0.85f, 0.58f, 0.10f, 1.0f);
			GunmetalColor = FLinearColor(0.0f, 0.72f, 0.88f, 1.0f);
		}
		OpticsColor = FLinearColor(0.95f, 0.22f, 0.05f, 1.0f); // Glowing EOTech Holographic Reticle
	}

	const FLinearColor SectionColors[6] = {
		CorduraColor,
		PlateColor,
		GunmetalColor,
		SkinColor,
		OpticsColor,
		StencilColor
	};

	const float SectionRoughness[6] = {
		0.84f, // Sec 0: Matte Cordura / Ripstop
		0.52f, // Sec 1: Hard Ceramic Plate / Polymer / Kydex
		0.34f, // Sec 2: Anodized Gunmetal / Weapon Steel
		0.46f, // Sec 3: Exposed Eye/Brow Skin Strip
		0.12f, // Sec 4: Refractive Visor / Goggle Lens
		0.65f  // Sec 5: Tactical Yellow/White Stencil Ink
	};

	const float SectionMetallic[6] = {
		0.05f,
		0.24f,
		0.85f,
		0.00f,
		0.85f,
		0.10f
	};

	for (int32 SecIdx = 0; SecIdx < 6; ++SecIdx)
	{
		if (ProcMesh->GetProcMeshSection(SecIdx) != nullptr)
		{
			UMaterialInstanceDynamic* MID = ProcMesh->CreateDynamicMaterialInstance(SecIdx, BaseMat);
			if (MID)
			{
				MID->SetVectorParameterValue(TEXT("Color"), SectionColors[SecIdx]);
				MID->SetVectorParameterValue(TEXT("BaseColor"), SectionColors[SecIdx]);
				MID->SetVectorParameterValue(TEXT("Tint"), SectionColors[SecIdx]);
				MID->SetScalarParameterValue(TEXT("Roughness"), SectionRoughness[SecIdx]);
				MID->SetScalarParameterValue(TEXT("Metallic"), SectionMetallic[SecIdx]);
			}
		}
	}
}

void UFPSOperatorGearComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateGearTransforms();
}

void UFPSOperatorGearComponent::UpdateGearTransforms()
{
	if (!TargetBodyMesh || !TargetBodyMesh->GetSkeletalMeshAsset())
	{
		return;
	}

	// Base Character Orientation in World Space:
	// TargetBodyMesh has RelativeRotation (0, -90, 0), so:
	//   MeshQuat.RotateVector(0, 1, 0) = Character Forward (+X in Actor/Turntable space)
	//   MeshQuat.RotateVector(1, 0, 0) = Character Left    (-Y in Actor/Turntable space)
	//   MeshQuat.RotateVector(0, 0, 1) = Character Up      (+Z in Actor/Turntable space)
	const FQuat MeshQuat = TargetBodyMesh->GetComponentQuat();
	const FVector CharFwd = MeshQuat.RotateVector(FVector(0.0f, 1.0f, 0.0f)).GetSafeNormal();
	const FVector CharLeft = MeshQuat.RotateVector(FVector(1.0f, 0.0f, 0.0f)).GetSafeNormal();
	const FVector CharUp = MeshQuat.RotateVector(FVector(0.0f, 0.0f, 1.0f)).GetSafeNormal();

	// Helper: Compute a 1:1 scale-free World Quaternion for a limb bone going from ParentPos down (-Z) to ChildPos,
	// with +Y aligned with HintFwd!
	auto MakeLimbWorldQuat = [](const FVector& ParentPos, const FVector& ChildPos, const FVector& HintFwd, const FQuat& FallbackQuat) -> FQuat
	{
		const FVector BoneUp = (ParentPos - ChildPos).GetSafeNormal();
		if (BoneUp.IsNearlyZero())
		{
			return FallbackQuat;
		}
		FVector LeftAxis = FVector::CrossProduct(BoneUp, HintFwd).GetSafeNormal();
		if (LeftAxis.IsNearlyZero())
		{
			LeftAxis = FVector::CrossProduct(BoneUp, FVector::UpVector).GetSafeNormal();
		}
		const FVector OrthoFwd = FVector::CrossProduct(LeftAxis, BoneUp).GetSafeNormal();
		return FRotationMatrix::MakeFromYZ(OrthoFwd, BoneUp).ToQuat();
	};

	auto PinMesh = [](UProceduralMeshComponent* Comp, const FVector& WorldLoc, const FQuat& WorldQuat, const FVector& Scale = FVector(1.0f, 1.0f, 1.0f))
	{
		if (Comp)
		{
			Comp->SetWorldLocationAndRotation(WorldLoc, WorldQuat);
			Comp->SetWorldScale3D(Scale);
		}
	};

	// Query authoritative world-space bone locations (100% immune to 100x FBX bone scale!)
	const FVector PelvisPos = TargetBodyMesh->GetBoneLocation(FName(TEXT("pelvis")), EBoneSpaces::WorldSpace);
	const FVector Spine01Pos = TargetBodyMesh->GetBoneLocation(FName(TEXT("spine_01")), EBoneSpaces::WorldSpace);
	const FVector Spine02Pos = TargetBodyMesh->GetBoneLocation(FName(TEXT("spine_02")), EBoneSpaces::WorldSpace);
	const FVector Spine03Pos = TargetBodyMesh->GetBoneLocation(FName(TEXT("spine_03")), EBoneSpaces::WorldSpace);
	const FVector NeckPos = TargetBodyMesh->GetBoneLocation(FName(TEXT("neck_01")), EBoneSpaces::WorldSpace);
	const FVector HeadPos = TargetBodyMesh->GetBoneLocation(FName(TEXT("head")), EBoneSpaces::WorldSpace);

	const FVector UpperArmL_Pos = TargetBodyMesh->GetBoneLocation(FName(TEXT("upperarm_l")), EBoneSpaces::WorldSpace);
	const FVector LowerArmL_Pos = TargetBodyMesh->GetBoneLocation(FName(TEXT("lowerarm_l")), EBoneSpaces::WorldSpace);
	const FVector HandL_Pos = TargetBodyMesh->GetBoneLocation(FName(TEXT("hand_l")), EBoneSpaces::WorldSpace);

	const FVector UpperArmR_Pos = TargetBodyMesh->GetBoneLocation(FName(TEXT("upperarm_r")), EBoneSpaces::WorldSpace);
	const FVector LowerArmR_Pos = TargetBodyMesh->GetBoneLocation(FName(TEXT("lowerarm_r")), EBoneSpaces::WorldSpace);
	const FVector HandR_Pos = TargetBodyMesh->GetBoneLocation(FName(TEXT("hand_r")), EBoneSpaces::WorldSpace);

	const FVector ThighL_Pos = TargetBodyMesh->GetBoneLocation(FName(TEXT("thigh_l")), EBoneSpaces::WorldSpace);
	const FVector CalfL_Pos = TargetBodyMesh->GetBoneLocation(FName(TEXT("calf_l")), EBoneSpaces::WorldSpace);
	const FVector FootL_Pos = TargetBodyMesh->GetBoneLocation(FName(TEXT("foot_l")), EBoneSpaces::WorldSpace);

	const FVector ThighR_Pos = TargetBodyMesh->GetBoneLocation(FName(TEXT("thigh_r")), EBoneSpaces::WorldSpace);
	const FVector CalfR_Pos = TargetBodyMesh->GetBoneLocation(FName(TEXT("calf_r")), EBoneSpaces::WorldSpace);
	const FVector FootR_Pos = TargetBodyMesh->GetBoneLocation(FName(TEXT("foot_r")), EBoneSpaces::WorldSpace);

	// Derive live shoulder-to-shoulder axis for upper torso twist tracking
	FVector TorsoLeft = (UpperArmL_Pos - UpperArmR_Pos).GetSafeNormal();
	if (TorsoLeft.IsNearlyZero())
	{
		TorsoLeft = CharLeft;
	}
	const FVector TorsoUp = (NeckPos - Spine01Pos).GetSafeNormal().IsNearlyZero() ? CharUp : (NeckPos - Spine01Pos).GetSafeNormal();
	const FVector TorsoFwd = FVector::CrossProduct(TorsoLeft, TorsoUp).GetSafeNormal();
	const FQuat TorsoQuat = FRotationMatrix::MakeFromYZ(TorsoFwd, TorsoUp).ToQuat();

	// 1. Zone 1: Head, Helmet, Visor, Eye-Strip & Comms
	if (HeadGearProcMesh && !bIsHelmetPopped)
	{
		const FVector HeadUp = (HeadPos - NeckPos).GetSafeNormal().IsNearlyZero() ? CharUp : (HeadPos - NeckPos).GetSafeNormal();
		// Subtle authoritative -3 deg chin tuck in menu Low-Ready stance
		const FQuat HeadQuat = FRotationMatrix::MakeFromYZ(CharFwd, HeadUp).ToQuat();
		PinMesh(HeadGearProcMesh, HeadPos, HeadQuat);
	}

	// 2. Zone 2: Level IV Plate Carrier, Ballistic Collar & Chest Rig
	if (TorsoGearProcMesh)
	{
		const FVector CarrierScale = bIsPlateShattered ? FVector(0.92f, 0.80f, 0.92f) : FVector(1.0f, 1.0f, 1.0f);
		PinMesh(TorsoGearProcMesh, Spine03Pos, TorsoQuat, CarrierScale);
	}

	// 3. Zone 3: Articulated Shoulder Pauldrons (upperarm_l -> lowerarm_l, upperarm_r -> lowerarm_r)
	const FQuat UpperArmL_Quat = MakeLimbWorldQuat(UpperArmL_Pos, LowerArmL_Pos, CharFwd, MeshQuat);
	const FQuat UpperArmR_Quat = MakeLimbWorldQuat(UpperArmR_Pos, LowerArmR_Pos, CharFwd, MeshQuat);
	PinMesh(LeftPauldronProcMesh, UpperArmL_Pos, UpperArmL_Quat);
	PinMesh(RightPauldronProcMesh, UpperArmR_Pos, UpperArmR_Quat);

	// 4. Zone 4: Forearm Vambraces & Reinforced Carbon-Knuckle Gloves
	const FQuat LowerArmL_Quat = MakeLimbWorldQuat(LowerArmL_Pos, HandL_Pos, CharFwd, UpperArmL_Quat);
	const FQuat LowerArmR_Quat = MakeLimbWorldQuat(LowerArmR_Pos, HandR_Pos, CharFwd, UpperArmR_Quat);
	PinMesh(LeftForearmProcMesh, LowerArmL_Pos, LowerArmL_Quat);
	PinMesh(RightForearmProcMesh, LowerArmR_Pos, LowerArmR_Quat);
	PinMesh(LeftGloveProcMesh, HandL_Pos, LowerArmL_Quat);
	PinMesh(RightGloveProcMesh, HandR_Pos, LowerArmR_Quat);

	// 5. Zone 5: MOLLE Battle Belt, Pouches & Groin Protector
	const FVector PelvisUp = (Spine02Pos - PelvisPos).GetSafeNormal().IsNearlyZero() ? CharUp : (Spine02Pos - PelvisPos).GetSafeNormal();
	const FQuat PelvisQuat = FRotationMatrix::MakeFromYZ(CharFwd, PelvisUp).ToQuat();
	PinMesh(BattleBeltProcMesh, PelvisPos, PelvisQuat);

	// 6. Zone 6: Thigh Rig & Drop-Leg Kydex Sidearm Holster (thigh_r -> calf_r, thigh_l -> calf_l)
	const FQuat ThighR_Quat = MakeLimbWorldQuat(ThighR_Pos, CalfR_Pos, CharFwd, MeshQuat);
	const FQuat ThighL_Quat = MakeLimbWorldQuat(ThighL_Pos, CalfL_Pos, CharFwd, MeshQuat);
	PinMesh(RightThighProcMesh, ThighR_Pos, ThighR_Quat);
	PinMesh(LeftThighProcMesh, ThighL_Pos, ThighL_Quat);

	// 7. Zone 7: Hard-Shell Tactical Knee Pads & Combat Boot Cuffs (calf_r -> foot_r, calf_l -> foot_l)
	const FQuat CalfR_Quat = MakeLimbWorldQuat(CalfR_Pos, FootR_Pos, CharFwd, ThighR_Quat);
	const FQuat CalfL_Quat = MakeLimbWorldQuat(CalfL_Pos, FootL_Pos, CharFwd, ThighL_Quat);
	PinMesh(RightCalfProcMesh, CalfR_Pos, CalfR_Quat);
	PinMesh(LeftCalfProcMesh, CalfL_Pos, CalfL_Quat);

	// 8. Spec-Ops Tactical Carbine: Locked between Right Hand (Pistol Grip at origin) and Left Hand (Foregrip at X=+26.5 cm)!
	if (CarbineProcMesh && bIncludeCarbine)
	{
		FVector BarrelDir = (HandL_Pos - HandR_Pos).GetSafeNormal();
		if (BarrelDir.IsNearlyZero())
		{
			BarrelDir = (CharFwd * 0.72f + CharLeft * 0.38f - CharUp * 0.58f).GetSafeNormal();
		}

		// Keep the rifle's top rail (+Z) oriented upright/slightly canted outward in a natural tactical grip
		FVector RifleRight = FVector::CrossProduct(CharUp, BarrelDir).GetSafeNormal();
		if (RifleRight.IsNearlyZero())
		{
			RifleRight = -CharLeft;
		}
		const FVector RifleUp = FVector::CrossProduct(BarrelDir, RifleRight).GetSafeNormal();
		const FQuat RifleWorldQuat = FRotationMatrix::MakeFromXZ(BarrelDir, RifleUp).ToQuat();

		PinMesh(CarbineProcMesh, HandR_Pos, RifleWorldQuat);
	}
}

void UFPSOperatorGearComponent::ApplyCamoPreset(int32 CamoIndex)
{
	CurrentCamoIndex = FMath::Clamp(CamoIndex, 0, 3);
	CalibrateOperatorBaseBodyMaterial();
	for (UProceduralMeshComponent* Comp : AllGearMeshes)
	{
		ApplyGearMaterialsToMesh(Comp);
	}
}

void UFPSOperatorGearComponent::ApplyHeadgearVariant(int32 HeadgearIndex)
{
	CurrentHeadgearIndex = FMath::Clamp(HeadgearIndex, 0, 3);
	BuildZone1_HeadAndComms();
	ApplyGearMaterialsToMesh(HeadGearProcMesh);
	UpdateGearTransforms();
}

void UFPSOperatorGearComponent::ApplyVestVariant(int32 VestIndex)
{
	CurrentVestIndex = FMath::Clamp(VestIndex, 0, 2);
	BuildZone2_PlateCarrierAndCollar();
	ApplyGearMaterialsToMesh(TorsoGearProcMesh);

	const bool bShowHeavyPauldrons = (CurrentVestIndex != 2);
	if (LeftPauldronProcMesh) LeftPauldronProcMesh->SetVisibility(bShowHeavyPauldrons, true);
	if (RightPauldronProcMesh) RightPauldronProcMesh->SetVisibility(bShowHeavyPauldrons, true);
	UpdateGearTransforms();
}

void UFPSOperatorGearComponent::ApplyWeaponFinishVariant(int32 FinishIndex)
{
	CurrentWeaponFinishIndex = FMath::Clamp(FinishIndex, 0, 3);
	ApplyGearMaterialsToMesh(CarbineProcMesh);
}

void UFPSOperatorGearComponent::SetHelmetVisible(bool bVisible)
{
	bIsHelmetPopped = !bVisible;
	if (HeadGearProcMesh)
	{
		HeadGearProcMesh->SetVisibility(bVisible, true);
	}
}

void UFPSOperatorGearComponent::SetPlateCarrierShattered(bool bShattered)
{
	bIsPlateShattered = bShattered;
	UpdateGearTransforms();
}
