#include "Character/FPSBeltGearComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "Engine/Texture2D.h"
#include "TextureResource.h"

namespace
{
	struct FBeltMeshBuffers
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

	static void AddBox(
		FBeltMeshBuffers& Buf,
		const FVector& Center,
		const FVector& HalfExtents,
		const FRotator& Rotation = FRotator::ZeroRotator,
		const FColor& Color = FColor::White)
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
			FVector Normal;
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
			const FVector WorldNormal = RotQ.RotateVector(Faces[f].Normal);
			const FVector WorldTangent = RotQ.RotateVector(FVector::CrossProduct(Faces[f].Normal, FVector::UpVector).IsNearlyZero() ? FVector::ForwardVector : FVector::CrossProduct(Faces[f].Normal, FVector::UpVector));
			for (int32 v = 0; v < 4; ++v)
			{
				Buf.Vertices.Add(WorldCorners[Faces[f].Idx[v]]);
				Buf.Normals.Add(WorldNormal);
				Buf.Colors.Add(Color);
				Buf.Tangents.Add(FProcMeshTangent(WorldTangent, false));
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

	static void AddBeveledBox(
		FBeltMeshBuffers& Buf,
		const FVector& Center,
		const FVector& HalfExtents,
		float Bevel,
		const FRotator& Rotation = FRotator::ZeroRotator,
		const FColor& Color = FColor::White)
	{
		AddBox(Buf, Center, FVector(FMath::Max(0.1f, HalfExtents.X - Bevel), HalfExtents.Y, FMath::Max(0.1f, HalfExtents.Z - Bevel)), Rotation, Color);
		AddBox(Buf, Center, FVector(HalfExtents.X, FMath::Max(0.1f, HalfExtents.Y - Bevel), FMath::Max(0.1f, HalfExtents.Z - Bevel * 1.2f)), Rotation, Color);
		AddBox(Buf, Center, FVector(FMath::Max(0.1f, HalfExtents.X - Bevel * 1.2f), FMath::Max(0.1f, HalfExtents.Y - Bevel), HalfExtents.Z), Rotation, Color);
	}

	static void AddCylinderZ(
		FBeltMeshBuffers& Buf,
		const FVector& Center,
		float RadiusBottom,
		float RadiusTop,
		float HalfHeight,
		int32 Sides,
		const FRotator& Rotation = FRotator::ZeroRotator,
		const FColor& Color = FColor::White,
		bool bCapEnds = true)
	{
		const FQuat RotQ = Rotation.Quaternion();
		const int32 ClampedSides = FMath::Max(3, Sides);

		// Side wall quads
		for (int32 i = 0; i < ClampedSides; ++i)
		{
			const float U0 = static_cast<float>(i) / static_cast<float>(ClampedSides);
			const float U1 = static_cast<float>(i + 1) / static_cast<float>(ClampedSides);
			const float A0 = U0 * 2.0f * PI;
			const float A1 = U1 * 2.0f * PI;

			const FVector Dir0(FMath::Cos(A0), FMath::Sin(A0), 0.0f);
			const FVector Dir1(FMath::Cos(A1), FMath::Sin(A1), 0.0f);

			const FVector P00 = Center + RotQ.RotateVector(Dir0 * RadiusBottom + FVector(0.0f, 0.0f, -HalfHeight));
			const FVector P10 = Center + RotQ.RotateVector(Dir1 * RadiusBottom + FVector(0.0f, 0.0f, -HalfHeight));
			const FVector P11 = Center + RotQ.RotateVector(Dir1 * RadiusTop + FVector(0.0f, 0.0f, HalfHeight));
			const FVector P01 = Center + RotQ.RotateVector(Dir0 * RadiusTop + FVector(0.0f, 0.0f, HalfHeight));

			const FVector N0 = RotQ.RotateVector(Dir0);
			const FVector N1 = RotQ.RotateVector(Dir1);
			const FVector T0 = RotQ.RotateVector(FVector(-Dir0.Y, Dir0.X, 0.0f));
			const FVector T1 = RotQ.RotateVector(FVector(-Dir1.Y, Dir1.X, 0.0f));

			const int32 BaseIdx = Buf.Vertices.Num();
			Buf.Vertices.Add(P00); Buf.Normals.Add(N0); Buf.UVs.Add(FVector2D(U0, 1.0f)); Buf.Colors.Add(Color); Buf.Tangents.Add(FProcMeshTangent(T0, false));
			Buf.Vertices.Add(P10); Buf.Normals.Add(N1); Buf.UVs.Add(FVector2D(U1, 1.0f)); Buf.Colors.Add(Color); Buf.Tangents.Add(FProcMeshTangent(T1, false));
			Buf.Vertices.Add(P11); Buf.Normals.Add(N1); Buf.UVs.Add(FVector2D(U1, 0.0f)); Buf.Colors.Add(Color); Buf.Tangents.Add(FProcMeshTangent(T1, false));
			Buf.Vertices.Add(P01); Buf.Normals.Add(N0); Buf.UVs.Add(FVector2D(U0, 0.0f)); Buf.Colors.Add(Color); Buf.Tangents.Add(FProcMeshTangent(T0, false));

			Buf.Triangles.Add(BaseIdx + 0);
			Buf.Triangles.Add(BaseIdx + 2);
			Buf.Triangles.Add(BaseIdx + 1);
			Buf.Triangles.Add(BaseIdx + 0);
			Buf.Triangles.Add(BaseIdx + 3);
			Buf.Triangles.Add(BaseIdx + 2);
		}

		if (bCapEnds)
		{
			// Top Cap (+Z)
			if (RadiusTop > 0.01f)
			{
				const int32 TopCenterIdx = Buf.Vertices.Num();
				const FVector TopNormal = RotQ.RotateVector(FVector::UpVector);
				const FVector TopTangent = RotQ.RotateVector(FVector::ForwardVector);
				Buf.Vertices.Add(Center + RotQ.RotateVector(FVector(0.0f, 0.0f, HalfHeight)));
				Buf.Normals.Add(TopNormal);
				Buf.UVs.Add(FVector2D(0.5f, 0.5f));
				Buf.Colors.Add(Color);
				Buf.Tangents.Add(FProcMeshTangent(TopTangent, false));

				for (int32 i = 0; i < ClampedSides; ++i)
				{
					const float A0 = (static_cast<float>(i) / ClampedSides) * 2.0f * PI;
					const float A1 = (static_cast<float>(i + 1) / ClampedSides) * 2.0f * PI;
					const FVector P0 = Center + RotQ.RotateVector(FVector(FMath::Cos(A0) * RadiusTop, FMath::Sin(A0) * RadiusTop, HalfHeight));
					const FVector P1 = Center + RotQ.RotateVector(FVector(FMath::Cos(A1) * RadiusTop, FMath::Sin(A1) * RadiusTop, HalfHeight));

					const int32 VIdx = Buf.Vertices.Num();
					Buf.Vertices.Add(P0); Buf.Normals.Add(TopNormal); Buf.UVs.Add(FVector2D(0.5f, 0.5f)); Buf.Colors.Add(Color); Buf.Tangents.Add(FProcMeshTangent(TopTangent, false));
					Buf.Vertices.Add(P1); Buf.Normals.Add(TopNormal); Buf.UVs.Add(FVector2D(0.5f, 0.5f)); Buf.Colors.Add(Color); Buf.Tangents.Add(FProcMeshTangent(TopTangent, false));

					Buf.Triangles.Add(TopCenterIdx);
					Buf.Triangles.Add(VIdx + 1);
					Buf.Triangles.Add(VIdx + 0);
				}
			}

			// Bottom Cap (-Z)
			if (RadiusBottom > 0.01f)
			{
				const int32 BotCenterIdx = Buf.Vertices.Num();
				const FVector BotNormal = RotQ.RotateVector(FVector::DownVector);
				const FVector BotTangent = RotQ.RotateVector(FVector::ForwardVector);
				Buf.Vertices.Add(Center + RotQ.RotateVector(FVector(0.0f, 0.0f, -HalfHeight)));
				Buf.Normals.Add(BotNormal);
				Buf.UVs.Add(FVector2D(0.5f, 0.5f));
				Buf.Colors.Add(Color);
				Buf.Tangents.Add(FProcMeshTangent(BotTangent, false));

				for (int32 i = 0; i < ClampedSides; ++i)
				{
					const float A0 = (static_cast<float>(i) / ClampedSides) * 2.0f * PI;
					const float A1 = (static_cast<float>(i + 1) / ClampedSides) * 2.0f * PI;
					const FVector P0 = Center + RotQ.RotateVector(FVector(FMath::Cos(A0) * RadiusBottom, FMath::Sin(A0) * RadiusBottom, -HalfHeight));
					const FVector P1 = Center + RotQ.RotateVector(FVector(FMath::Cos(A1) * RadiusBottom, FMath::Sin(A1) * RadiusBottom, -HalfHeight));

					const int32 VIdx = Buf.Vertices.Num();
					Buf.Vertices.Add(P0); Buf.Normals.Add(BotNormal); Buf.UVs.Add(FVector2D(0.5f, 0.5f)); Buf.Colors.Add(Color); Buf.Tangents.Add(FProcMeshTangent(BotTangent, false));
					Buf.Vertices.Add(P1); Buf.Normals.Add(BotNormal); Buf.UVs.Add(FVector2D(0.5f, 0.5f)); Buf.Colors.Add(Color); Buf.Tangents.Add(FProcMeshTangent(BotTangent, false));

					Buf.Triangles.Add(BotCenterIdx);
					Buf.Triangles.Add(VIdx + 0);
					Buf.Triangles.Add(VIdx + 1);
				}
			}
		}
	}

	static void AddUVSphere(
		FBeltMeshBuffers& Buf,
		const FVector& Center,
		float RadiusX,
		float RadiusY,
		float RadiusZ,
		int32 Slices,
		int32 Stacks,
		const FColor& Color = FColor::White)
	{
		for (int32 st = 0; st < Stacks; ++st)
		{
			const float V0 = static_cast<float>(st) / Stacks;
			const float V1 = static_cast<float>(st + 1) / Stacks;
			const float Phi0 = (V0 - 0.5f) * PI;
			const float Phi1 = (V1 - 0.5f) * PI;

			for (int32 sl = 0; sl < Slices; ++sl)
			{
				const float U0 = static_cast<float>(sl) / Slices;
				const float U1 = static_cast<float>(sl + 1) / Slices;
				const float Theta0 = U0 * 2.0f * PI;
				const float Theta1 = U1 * 2.0f * PI;

				auto EvalSphere = [&](float Phi, float Theta, FVector& OutPos, FVector& OutNorm)
				{
					const float CP = FMath::Cos(Phi);
					const float SP = FMath::Sin(Phi);
					const float CT = FMath::Cos(Theta);
					const float ST = FMath::Sin(Theta);
					const FVector Unit(CP * CT, CP * ST, SP);
					OutPos = Center + FVector(Unit.X * RadiusX, Unit.Y * RadiusY, Unit.Z * RadiusZ);
					OutNorm = Unit.GetSafeNormal();
				};

				FVector P00, N00, P10, N10, P11, N11, P01, N01;
				EvalSphere(Phi0, Theta0, P00, N00);
				EvalSphere(Phi0, Theta1, P10, N10);
				EvalSphere(Phi1, Theta1, P11, N11);
				EvalSphere(Phi1, Theta0, P01, N01);

				const int32 BaseIdx = Buf.Vertices.Num();
				const FProcMeshTangent Tang(FVector(-FMath::Sin(Theta0), FMath::Cos(Theta0), 0.0f), false);
				Buf.Vertices.Add(P00); Buf.Normals.Add(N00); Buf.UVs.Add(FVector2D(U0, 1.0f - V0)); Buf.Colors.Add(Color); Buf.Tangents.Add(Tang);
				Buf.Vertices.Add(P10); Buf.Normals.Add(N10); Buf.UVs.Add(FVector2D(U1, 1.0f - V0)); Buf.Colors.Add(Color); Buf.Tangents.Add(Tang);
				Buf.Vertices.Add(P11); Buf.Normals.Add(N11); Buf.UVs.Add(FVector2D(U1, 1.0f - V1)); Buf.Colors.Add(Color); Buf.Tangents.Add(Tang);
				Buf.Vertices.Add(P01); Buf.Normals.Add(N01); Buf.UVs.Add(FVector2D(U0, 1.0f - V1)); Buf.Colors.Add(Color); Buf.Tangents.Add(Tang);

				Buf.Triangles.Add(BaseIdx + 0);
				Buf.Triangles.Add(BaseIdx + 1);
				Buf.Triangles.Add(BaseIdx + 2);
				Buf.Triangles.Add(BaseIdx + 0);
				Buf.Triangles.Add(BaseIdx + 2);
				Buf.Triangles.Add(BaseIdx + 3);
			}
		}
	}

	static void AddTorusRing(
		FBeltMeshBuffers& Buf,
		const FVector& Center,
		float MajorRadius,
		float MinorRadius,
		int32 MajorSegs,
		int32 MinorSegs,
		const FRotator& Rotation = FRotator::ZeroRotator,
		const FColor& Color = FColor::White)
	{
		const FQuat RotQ = Rotation.Quaternion();
		for (int32 i = 0; i < MajorSegs; ++i)
		{
			const float A0 = (static_cast<float>(i) / MajorSegs) * 2.0f * PI;
			const float A1 = (static_cast<float>(i + 1) / MajorSegs) * 2.0f * PI;

			for (int32 j = 0; j < MinorSegs; ++j)
			{
				const float B0 = (static_cast<float>(j) / MinorSegs) * 2.0f * PI;
				const float B1 = (static_cast<float>(j + 1) / MinorSegs) * 2.0f * PI;

				auto EvalTorus = [&](float A, float B, FVector& OutP, FVector& OutN)
				{
					const float R = MajorRadius + MinorRadius * FMath::Cos(B);
					const FVector LocalP(R * FMath::Cos(A), R * FMath::Sin(A), MinorRadius * FMath::Sin(B));
					const FVector LocalN(FMath::Cos(B) * FMath::Cos(A), FMath::Cos(B) * FMath::Sin(A), FMath::Sin(B));
					OutP = Center + RotQ.RotateVector(LocalP);
					OutN = RotQ.RotateVector(LocalN);
				};

				FVector P00, N00, P10, N10, P11, N11, P01, N01;
				EvalTorus(A0, B0, P00, N00);
				EvalTorus(A1, B0, P10, N10);
				EvalTorus(A1, B1, P11, N11);
				EvalTorus(A0, B1, P01, N01);

				const int32 BaseIdx = Buf.Vertices.Num();
				const FProcMeshTangent Tang(RotQ.RotateVector(FVector(-FMath::Sin(A0), FMath::Cos(A0), 0.0f)), false);
				Buf.Vertices.Add(P00); Buf.Normals.Add(N00); Buf.UVs.Add(FVector2D(0.0f, 0.0f)); Buf.Colors.Add(Color); Buf.Tangents.Add(Tang);
				Buf.Vertices.Add(P10); Buf.Normals.Add(N10); Buf.UVs.Add(FVector2D(1.0f, 0.0f)); Buf.Colors.Add(Color); Buf.Tangents.Add(Tang);
				Buf.Vertices.Add(P11); Buf.Normals.Add(N11); Buf.UVs.Add(FVector2D(1.0f, 1.0f)); Buf.Colors.Add(Color); Buf.Tangents.Add(Tang);
				Buf.Vertices.Add(P01); Buf.Normals.Add(N01); Buf.UVs.Add(FVector2D(0.0f, 1.0f)); Buf.Colors.Add(Color); Buf.Tangents.Add(Tang);

				Buf.Triangles.Add(BaseIdx + 0);
				Buf.Triangles.Add(BaseIdx + 2);
				Buf.Triangles.Add(BaseIdx + 1);
				Buf.Triangles.Add(BaseIdx + 0);
				Buf.Triangles.Add(BaseIdx + 3);
				Buf.Triangles.Add(BaseIdx + 2);
			}
		}
	}

	static void CommitSection(UProceduralMeshComponent* Comp, int32 SectionIdx, FBeltMeshBuffers& Buf)
	{
		if (Comp && Buf.Vertices.Num() > 0)
		{
			Comp->CreateMeshSection(SectionIdx, Buf.Vertices, Buf.Triangles, Buf.Normals, Buf.UVs, Buf.Colors, Buf.Tangents, false);
		}
	}
}

UFPSBeltGearComponent::UFPSBeltGearComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bTickEvenWhenPaused = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;

	BeltMagComp_1 = nullptr;
	BeltMagComp_2 = nullptr;
	BeltFragComp  = nullptr;
	BeltFlashComp = nullptr;

	HeldMagComp   = nullptr;
	HeldFragComp  = nullptr;
	HeldFlashComp = nullptr;
}

void UFPSBeltGearComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UFPSBeltGearComponent::InitializeBeltKit(USkeletalMeshComponent* InBodyMesh)
{
	TargetBodyMesh = InBodyMesh;
	GenerateRuntimeBeltTextures();

	AActor* OwnerActor = GetOwner();
	USceneComponent* AttachTarget = InBodyMesh ? static_cast<USceneComponent*>(InBodyMesh) : (OwnerActor ? OwnerActor->GetRootComponent() : nullptr);

	auto EnsureBeltSubMesh = [&](TObjectPtr<UProceduralMeshComponent>& CompPtr, const TCHAR* Name, bool bDefaultVis)
	{
		if (!CompPtr && OwnerActor)
		{
			CompPtr = NewObject<UProceduralMeshComponent>(OwnerActor, Name);
		}
		if (CompPtr)
		{
			CompPtr->SetMobility(EComponentMobility::Movable);
			CompPtr->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			CompPtr->SetCastShadow(true);
			CompPtr->bCastDynamicShadow = true;
			if (AttachTarget && CompPtr->GetAttachParent() != AttachTarget)
			{
				CompPtr->AttachToComponent(AttachTarget, FAttachmentTransformRules::KeepWorldTransform);
			}
			if (!CompPtr->IsRegistered() && OwnerActor)
			{
				CompPtr->RegisterComponent();
			}
			CompPtr->SetVisibility(bDefaultVis, false);
			CompPtr->SetHiddenInGame(!bDefaultVis);
		}
	};

	EnsureBeltSubMesh(BeltMagComp_1, TEXT("BeltMagComp_1"), true);
	EnsureBeltSubMesh(BeltMagComp_2, TEXT("BeltMagComp_2"), true);
	EnsureBeltSubMesh(BeltFragComp,  TEXT("BeltFragComp"),  true);
	EnsureBeltSubMesh(BeltFlashComp, TEXT("BeltFlashComp"), true);
	EnsureBeltSubMesh(HeldMagComp,   TEXT("HeldMagComp"),   false);
	EnsureBeltSubMesh(HeldFragComp,  TEXT("HeldFragComp"),  false);
	EnsureBeltSubMesh(HeldFlashComp, TEXT("HeldFlashComp"), false);

	// Build 3D Geometry & PBR Materials for each addressable component
	BuildStanagFastMagMesh(BeltMagComp_1, true, true);
	BuildStanagFastMagMesh(BeltMagComp_2, true, true);
	BuildM67FragGrenadeMesh(BeltFragComp);
	BuildM84FlashbangMesh(BeltFlashComp);

	// Build Hand-Held Proxy Meshes (for live reload & grenade throw belt-grabs)
	BuildStanagFastMagMesh(HeldMagComp, false, true);
	BuildM67FragGrenadeMesh(HeldFragComp);
	BuildM84FlashbangMesh(HeldFlashComp);

	if (HeldMagComp)   { HeldMagComp->SetVisibility(false);   HeldMagComp->SetHiddenInGame(true); }
	if (HeldFragComp)  { HeldFragComp->SetVisibility(false);  HeldFragComp->SetHiddenInGame(true); }
	if (HeldFlashComp) { HeldFlashComp->SetVisibility(false); HeldFlashComp->SetHiddenInGame(true); }

	UpdateBeltTransforms();
}

void UFPSBeltGearComponent::GenerateRuntimeBeltTextures()
{
	// 1. Pure White 16x16 Diffuse Texture so M_Operator_Armor multiplies 1.0 * CamoColorPrimary
	if (!WhiteDetailTex)
	{
		WhiteDetailTex = UTexture2D::CreateTransient(16, 16, PF_B8G8R8A8);
		if (WhiteDetailTex)
		{
			WhiteDetailTex->SRGB = true;
			WhiteDetailTex->Filter = TF_Bilinear;
			FTexture2DMipMap& Mip = WhiteDetailTex->GetPlatformData()->Mips[0];
			uint8* Data = static_cast<uint8*>(Mip.BulkData.Lock(LOCK_READ_WRITE));
			if (Data)
			{
				for (int32 i = 0; i < 16 * 16 * 4; i += 4)
				{
					Data[i + 0] = 245; // B
					Data[i + 1] = 245; // G
					Data[i + 2] = 245; // R
					Data[i + 3] = 255; // A
				}
				Mip.BulkData.Unlock();
				WhiteDetailTex->UpdateResource();
			}
		}
	}

	// 2. Perforated Steel Texture (128x128) for M84 Stun Grenade Blast Ports
	if (!PerforatedSteelTex)
	{
		PerforatedSteelTex = UTexture2D::CreateTransient(128, 128, PF_B8G8R8A8);
		if (PerforatedSteelTex)
		{
			PerforatedSteelTex->SRGB = true;
			PerforatedSteelTex->Filter = TF_Bilinear;
			FTexture2DMipMap& Mip = PerforatedSteelTex->GetPlatformData()->Mips[0];
			uint8* Data = static_cast<uint8*>(Mip.BulkData.Lock(LOCK_READ_WRITE));
			if (Data)
			{
				for (int32 Y = 0; Y < 128; ++Y)
				{
					for (int32 X = 0; X < 128; ++X)
					{
						uint8 Val = 235;
						// 8 columns x 4 rows of circular blast ports in the middle band (Y in 28..100)
						if (Y >= 24 && Y <= 104)
						{
							const float CellX = FMath::Fmod(static_cast<float>(X), 16.0f) - 8.0f;
							const float CellY = FMath::Fmod(static_cast<float>(Y - 24), 20.0f) - 10.0f;
							const float DistSq = CellX * CellX + CellY * CellY;
							if (DistSq < 18.0f)
							{
								Val = 24; // Dark recessed blast port hole
							}
							else if (DistSq < 25.0f)
							{
								Val = 150; // Machined chamfer rim around port
							}
						}
						const int32 Idx = (Y * 128 + X) * 4;
						Data[Idx + 0] = Val;
						Data[Idx + 1] = Val;
						Data[Idx + 2] = Val;
						Data[Idx + 3] = 255;
					}
				}
				Mip.BulkData.Unlock();
				PerforatedSteelTex->UpdateResource();
			}
		}
	}

	// 3. Flat Tangent-Space Normal Map (16x16) so UV-mapped procedural geometry never gets distorted normals
	if (!FlatNormalTex)
	{
		FlatNormalTex = UTexture2D::CreateTransient(16, 16, PF_B8G8R8A8);
		if (FlatNormalTex)
		{
			FlatNormalTex->SRGB = false;
			FlatNormalTex->CompressionSettings = TC_Normalmap;
			FlatNormalTex->Filter = TF_Bilinear;
			FTexture2DMipMap& Mip = FlatNormalTex->GetPlatformData()->Mips[0];
			uint8* Data = static_cast<uint8*>(Mip.BulkData.Lock(LOCK_READ_WRITE));
			if (Data)
			{
				for (int32 i = 0; i < 16 * 16 * 4; i += 4)
				{
					Data[i + 0] = 255; // B (+Z)
					Data[i + 1] = 128; // G (0)
					Data[i + 2] = 128; // R (0)
					Data[i + 3] = 255; // A
				}
				Mip.BulkData.Unlock();
				FlatNormalTex->UpdateResource();
			}
		}
	}
}

void UFPSBeltGearComponent::ApplyPBRSectionMaterial(
	UProceduralMeshComponent* Comp,
	int32 SectionIdx,
	const FLinearColor& BaseColor,
	float Roughness,
	float Metallic,
	UTexture2D* CustomDiffuse)
{
	if (!Comp || Comp->GetProcMeshSection(SectionIdx) == nullptr)
	{
		return;
	}

	UMaterialInterface* ParentMat = nullptr;
	if (!CustomDiffuse)
	{
		ParentMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_Belt_Gear_PBR.M_Belt_Gear_PBR"));
	}
	if (!ParentMat)
	{
		ParentMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_Operator_Armor.M_Operator_Armor"));
	}
	if (!ParentMat)
	{
		ParentMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	}
	if (!ParentMat)
	{
		ParentMat = UMaterial::GetDefaultMaterial(MD_Surface);
	}
	if (!ParentMat)
	{
		return;
	}

	UMaterialInstanceDynamic* MID = Comp->CreateDynamicMaterialInstance(SectionIdx, ParentMat);
	if (MID)
	{
		UTexture2D* DiffTex = CustomDiffuse ? CustomDiffuse : WhiteDetailTex.Get();
		if (DiffTex)
		{
			MID->SetTextureParameterValue(TEXT("Texture_Diffuse"), DiffTex);
		}
		if (FlatNormalTex)
		{
			MID->SetTextureParameterValue(TEXT("Texture_Normal"), FlatNormalTex);
		}
		MID->SetVectorParameterValue(TEXT("CamoColorPrimary"), BaseColor);
		MID->SetVectorParameterValue(TEXT("BaseColor"), BaseColor);
		MID->SetVectorParameterValue(TEXT("Color"), BaseColor);
		MID->SetScalarParameterValue(TEXT("RoughnessBase"), Roughness);
		MID->SetScalarParameterValue(TEXT("Roughness"), Roughness);
		MID->SetScalarParameterValue(TEXT("Metallic"), Metallic);
	}
}

void UFPSBeltGearComponent::BuildStanagFastMagMesh(UProceduralMeshComponent* Comp, bool bIncludeKydexPouch, bool bIncludeMagazine)
{
	if (!Comp) return;
	Comp->ClearAllMeshSections();

	FBeltMeshBuffers Sec0_Kydex;
	FBeltMeshBuffers Sec1_Polymer;
	FBeltMeshBuffers Sec2_GunmetalMag;
	FBeltMeshBuffers Sec3_BrassAmmo;

	// Local Frame of Magazine Pouch:
	// +Z = Up (Feed lips at top, Ranger floorplate at bottom -Z)
	// +X = Width across magazine spine-to-curve (approx 6.4 cm total width -> HalfX = 3.2 cm)
	// +Y = Thickness outward from body (approx 2.4 cm total thickness -> HalfY = 1.2 cm)

	if (bIncludeKydexPouch)
	{
		// 1. Open-Top Kydex Fast-Mag Shell wrapping lower/mid magazine (Z = -4.2 cm to +2.8 cm)
		AddBeveledBox(Sec0_Kydex, FVector(0.0f, 0.0f, -0.8f), FVector(3.55f, 1.50f, 3.6f), 0.35f);
		// Flared Kydex extraction funnel lips at top of pouch (Z = +2.6 cm)
		AddBox(Sec0_Kydex, FVector(0.0f,  1.45f, 2.5f), FVector(3.4f, 0.22f, 0.65f), FRotator(12.0f, 0.0f, 0.0f));
		AddBox(Sec0_Kydex, FVector(0.0f, -1.45f, 2.5f), FVector(3.4f, 0.22f, 0.65f), FRotator(-12.0f, 0.0f, 0.0f));
		// Rear Tek-Lok / MOLLE Belt Mounting Clip (Y = -1.65 cm)
		AddBeveledBox(Sec0_Kydex, FVector(0.0f, -1.65f, 0.0f), FVector(2.4f, 0.35f, 3.2f), 0.2f);

		// 2. Matte Polymer Tension Retention Bands wrapping the Kydex shell
		AddBox(Sec1_Polymer, FVector(0.0f, 0.0f,  0.8f), FVector(3.70f, 1.62f, 0.55f));
		AddBox(Sec1_Polymer, FVector(0.0f, 0.0f, -2.2f), FVector(3.70f, 1.62f, 0.55f));
		// Side Bungee Retention Cord Blocks
		AddBox(Sec1_Polymer, FVector( 3.65f, 0.0f, -0.7f), FVector(0.25f, 0.9f, 2.6f));
		AddBox(Sec1_Polymer, FVector(-3.65f, 0.0f, -0.7f), FVector(0.25f, 0.9f, 2.6f));
	}

	if (bIncludeMagazine)
	{
		// 3. Curved 30-Round STANAG Magazine Body (6 swept segments from Z = -6.2 cm up to Z = +6.8 cm)
		const int32 NumSegs = 6;
		const float ZMin = -6.0f;
		const float ZMax =  6.6f;
		const float SegHalfH = ((ZMax - ZMin) / NumSegs) * 0.54f;

		for (int32 s = 0; s < NumSegs; ++s)
		{
			const float Alpha = (static_cast<float>(s) + 0.5f) / NumSegs;
			const float Z = FMath::Lerp(ZMin, ZMax, Alpha);
			// Authentic STANAG curve along +X as Z goes down toward floorplate
			const float CurveInv = 1.0f - Alpha;
			const float CurveX = CurveInv * CurveInv * 1.35f;
			const float PitchDeg = CurveInv * 11.0f;

			AddBeveledBox(
				Sec2_GunmetalMag,
				FVector(CurveX, 0.0f, Z),
				FVector(3.10f, 1.12f, SegHalfH),
				0.22f,
				FRotator(PitchDeg, 0.0f, 0.0f));

			// Stamped Lateral Reinforcement Ribs on front & back faces of magazine
			if (s >= 1 && s <= 4)
			{
				AddBox(
					Sec2_GunmetalMag,
					FVector(CurveX, 0.0f, Z),
					FVector(2.45f, 1.22f, 0.22f),
					FRotator(PitchDeg, 0.0f, 0.0f));
			}
		}

		// Top Feed Lips & Rear Locking Notch (Z = +6.9 cm)
		AddBox(Sec2_GunmetalMag, FVector(-0.4f,  0.72f, 6.95f), FVector(2.2f, 0.22f, 0.45f));
		AddBox(Sec2_GunmetalMag, FVector(-0.4f, -0.72f, 6.95f), FVector(2.2f, 0.22f, 0.45f));

		// Bottom Magpul-Style Ranger Floorplate & Finger Pull-Loop (Z = -6.8 cm)
		AddBeveledBox(Sec1_Polymer, FVector(1.45f, 0.0f, -6.5f), FVector(3.35f, 1.28f, 0.65f), 0.2f, FRotator(11.0f, 0.0f, 0.0f));
		AddTorusRing(Sec1_Polymer, FVector(1.65f, 0.0f, -7.8f), 1.35f, 0.28f, 10, 6, FRotator(0.0f, 90.0f, 90.0f));

		// 4. Visible 5.56x45mm NATO Brass Cartridges & Copper Bullet Tips at Top of Magazine (Z = +7.05 cm)
		// Cartridge 1 (Right Feed Lip)
		AddCylinderZ(Sec3_BrassAmmo, FVector(-0.8f, 0.35f, 7.05f), 0.46f, 0.44f, 1.65f, 10, FRotator(90.0f, 0.0f, 0.0f));
		AddCylinderZ(Sec3_BrassAmmo, FVector( 1.4f, 0.35f, 7.05f), 0.42f, 0.08f, 0.75f, 10, FRotator(90.0f, 0.0f, 0.0f));
		// Cartridge 2 (Left Feed Lip, slightly recessed)
		AddCylinderZ(Sec3_BrassAmmo, FVector(-0.8f, -0.35f, 6.85f), 0.46f, 0.44f, 1.65f, 10, FRotator(90.0f, 0.0f, 0.0f));
		AddCylinderZ(Sec3_BrassAmmo, FVector( 1.4f, -0.35f, 6.85f), 0.42f, 0.08f, 0.75f, 10, FRotator(90.0f, 0.0f, 0.0f));
	}

	CommitSection(Comp, 0, Sec0_Kydex);
	CommitSection(Comp, 1, Sec1_Polymer);
	CommitSection(Comp, 2, Sec2_GunmetalMag);
	CommitSection(Comp, 3, Sec3_BrassAmmo);

	// Apply Calibrated PBR Materials:
	// Sec 0: Matte Black/Slate Kydex Pouch
	ApplyPBRSectionMaterial(Comp, 0, FLinearColor(0.055f, 0.062f, 0.072f, 1.0f), 0.52f, 0.15f);
	// Sec 1: Coyote/FDE Polymer Retention Band & Ranger Pull Floorplate
	ApplyPBRSectionMaterial(Comp, 1, FLinearColor(0.28f, 0.23f, 0.16f, 1.0f), 0.65f, 0.08f);
	// Sec 2: Dry-Film Lubricated Gunmetal Steel STANAG Magazine
	ApplyPBRSectionMaterial(Comp, 2, FLinearColor(0.14f, 0.155f, 0.175f, 1.0f), 0.36f, 0.85f);
	// Sec 3: Polished 5.56mm Brass Cases & Copper Tips
	ApplyPBRSectionMaterial(Comp, 3, FLinearColor(0.82f, 0.58f, 0.18f, 1.0f), 0.22f, 0.95f);
}

void UFPSBeltGearComponent::BuildM67FragGrenadeMesh(UProceduralMeshComponent* Comp)
{
	if (!Comp) return;
	Comp->ClearAllMeshSections();

	FBeltMeshBuffers Sec0_OliveBody;
	FBeltMeshBuffers Sec1_YellowBand;
	FBeltMeshBuffers Sec2_FuseAndSpoon;

	// 1. M67 Spherical Cast-Steel Body (Radius = 3.25 cm, slightly oblate with top neck collar)
	AddUVSphere(Sec0_OliveBody, FVector(0.0f, 0.0f, 0.0f), 3.25f, 3.25f, 3.35f, 18, 12);
	// Crimped Steel Neck Collar at top of sphere (Z = +3.15 cm)
	AddCylinderZ(Sec0_OliveBody, FVector(0.0f, 0.0f, 3.25f), 1.35f, 1.25f, 0.45f, 12);

	// 2. High-Explosive (Comp B) Yellow Identification Band around upper hemisphere (Z = +1.35 cm)
	AddCylinderZ(Sec1_YellowBand, FVector(0.0f, 0.0f, 1.35f), 3.12f, 2.86f, 0.38f, 18, FRotator::ZeroRotator, FColor::White, false);

	// 3. M213 Delay Fuse Assembly, Curved Safety Spoon Lever & Pull-Ring Pin
	// Threaded Zinc-Steel Fuse Head (Z = +4.25 cm)
	AddCylinderZ(Sec2_FuseAndSpoon, FVector(0.0f, 0.0f, 4.20f), 1.05f, 1.12f, 0.75f, 12);
	// Top Fuse Striker Hinge Hood (Z = +5.05 cm)
	AddBeveledBox(Sec2_FuseAndSpoon, FVector(0.15f, 0.0f, 5.05f), FVector(1.35f, 0.85f, 0.42f), 0.12f);

	// Curved Safety Spoon Lever wrapping down the +X profile of the M67 sphere
	AddBox(Sec2_FuseAndSpoon, FVector(2.15f, 0.0f, 4.55f), FVector(1.25f, 0.68f, 0.18f), FRotator(-18.0f, 0.0f, 0.0f));
	AddBox(Sec2_FuseAndSpoon, FVector(3.35f, 0.0f, 3.05f), FVector(0.22f, 0.65f, 1.45f), FRotator(-16.0f, 0.0f, 0.0f));
	AddBox(Sec2_FuseAndSpoon, FVector(3.60f, 0.0f, 0.75f), FVector(0.20f, 0.58f, 1.35f), FRotator(4.0f, 0.0f, 0.0f));
	AddBox(Sec2_FuseAndSpoon, FVector(3.25f, 0.0f, -1.25f), FVector(0.18f, 0.52f, 0.95f), FRotator(22.0f, 0.0f, 0.0f));

	// Safety Cotter Pin & Circular Pull-Ring on front face (+Y) of Fuse Head
	AddCylinderZ(Sec2_FuseAndSpoon, FVector(0.0f, 0.0f, 4.55f), 0.14f, 0.14f, 1.25f, 8, FRotator(90.0f, 90.0f, 0.0f));
	AddTorusRing(Sec2_FuseAndSpoon, FVector(-0.35f, 1.35f, 4.15f), 1.15f, 0.18f, 14, 6, FRotator(15.0f, 0.0f, 0.0f));

	CommitSection(Comp, 0, Sec0_OliveBody);
	CommitSection(Comp, 1, Sec1_YellowBand);
	CommitSection(Comp, 2, Sec2_FuseAndSpoon);

	// Apply Calibrated PBR Materials:
	// Sec 0: Olive-Drab Cast Steel (#3B4628, Roughness = 0.65)
	ApplyPBRSectionMaterial(Comp, 0, FLinearColor(0.14f, 0.18f, 0.085f, 1.0f), 0.65f, 0.18f);
	// Sec 1: High-Explosive Amber-Yellow Identification Band
	ApplyPBRSectionMaterial(Comp, 1, FLinearColor(0.88f, 0.64f, 0.08f, 1.0f), 0.48f, 0.10f);
	// Sec 2: Machined Zinc / Steel Fuse, Spoon & Pull-Ring
	ApplyPBRSectionMaterial(Comp, 2, FLinearColor(0.42f, 0.44f, 0.46f, 1.0f), 0.30f, 0.88f);
}

void UFPSBeltGearComponent::BuildM84FlashbangMesh(UProceduralMeshComponent* Comp)
{
	if (!Comp) return;
	Comp->ClearAllMeshSections();

	FBeltMeshBuffers Sec0_PerforatedSteel;
	FBeltMeshBuffers Sec1_BlueFlashBand;
	FBeltMeshBuffers Sec2_FuseAndClip;

	// 1. Central Perforated Steel Cylinder (Radius = 1.95 cm, HalfHeight = 4.2 cm)
	AddCylinderZ(Sec0_PerforatedSteel, FVector(0.0f, 0.0f, 0.0f), 1.95f, 1.95f, 4.20f, 16);

	// Top & Bottom Heavy Hexagonal Machined Steel Blast Caps (6 sides, Radius = 2.45 cm)
	AddCylinderZ(Sec0_PerforatedSteel, FVector(0.0f, 0.0f,  4.45f), 2.45f, 2.45f, 0.65f, 6);
	AddCylinderZ(Sec0_PerforatedSteel, FVector(0.0f, 0.0f, -4.45f), 2.45f, 2.45f, 0.65f, 6);

	// 2. Tactical Blue/White Stun Identification Bands around Upper & Lower Body
	AddCylinderZ(Sec1_BlueFlashBand, FVector(0.0f, 0.0f,  3.15f), 2.02f, 2.02f, 0.38f, 16, FRotator::ZeroRotator, FColor::White, false);
	AddCylinderZ(Sec1_BlueFlashBand, FVector(0.0f, 0.0f, -3.15f), 2.02f, 2.02f, 0.38f, 16, FRotator::ZeroRotator, FColor::White, false);

	// 3. M201A1 Fuse Assembly, Straight Safety Spoon Lever & Dual Pull-Rings
	AddCylinderZ(Sec2_FuseAndClip, FVector(0.0f, 0.0f, 5.75f), 1.05f, 1.05f, 0.75f, 12);
	AddBeveledBox(Sec2_FuseAndClip, FVector(0.20f, 0.0f, 6.55f), FVector(1.35f, 0.82f, 0.38f), 0.10f);

	// Straight Stamped Safety Spoon Lever running down +X side of Flashbang
	AddBox(Sec2_FuseAndClip, FVector(1.85f, 0.0f, 6.20f), FVector(1.10f, 0.62f, 0.18f), FRotator(-12.0f, 0.0f, 0.0f));
	AddBox(Sec2_FuseAndClip, FVector(2.68f, 0.0f, 1.20f), FVector(0.18f, 0.58f, 4.65f));

	// Pull-Ring Pin on front (+Y) of Fuse
	AddTorusRing(Sec2_FuseAndClip, FVector(-0.25f, 1.30f, 5.85f), 1.10f, 0.17f, 14, 6, FRotator(15.0f, 0.0f, 0.0f));

	CommitSection(Comp, 0, Sec0_PerforatedSteel);
	CommitSection(Comp, 1, Sec1_BlueFlashBand);
	CommitSection(Comp, 2, Sec2_FuseAndClip);

	// Apply Calibrated PBR Materials:
	// Sec 0: Perforated Zinc/Steel Cylinder & Hex Caps (uses PerforatedSteelTex)
	ApplyPBRSectionMaterial(Comp, 0, FLinearColor(0.52f, 0.55f, 0.58f, 1.0f), 0.32f, 0.88f, PerforatedSteelTex.Get());
	// Sec 1: Tactical Stun Blue/Cyan Identification Band
	ApplyPBRSectionMaterial(Comp, 1, FLinearColor(0.10f, 0.48f, 0.86f, 1.0f), 0.45f, 0.15f);
	// Sec 2: Dark Gunmetal Fuse & Safety Spoon
	ApplyPBRSectionMaterial(Comp, 2, FLinearColor(0.18f, 0.20f, 0.23f, 1.0f), 0.34f, 0.85f);
}

bool UFPSBeltGearComponent::ComputePelvisOrthonormalFrame(
	FVector& OutPelvisOrigin,
	FQuat& OutPelvisQuat,
	FVector& OutRight,
	FVector& OutFwd,
	FVector& OutUp) const
{
	if (!TargetBodyMesh || !TargetBodyMesh->GetSkeletalMeshAsset())
	{
		return false;
	}

	const FTransform MeshTransform = TargetBodyMesh->GetComponentTransform();
	OutFwd   = MeshTransform.GetUnitAxis(EAxis::X);
	OutRight = MeshTransform.GetUnitAxis(EAxis::Y);
	OutUp    = MeshTransform.GetUnitAxis(EAxis::Z);
	OutPelvisQuat = MeshTransform.GetRotation();

	const FVector PelvisPos = TargetBodyMesh->GetSocketLocation(FName(TEXT("pelvis")));
	OutPelvisOrigin = PelvisPos.IsNearlyZero()
		? (MeshTransform.GetLocation() + OutUp * 105.0f)
		: PelvisPos;

	return true;
}

FVector UFPSBeltGearComponent::GetItemLocalOffset(EFPSBeltItemType Item) const
{
	// Local coordinates in Pelvis Orthonormal Frame:
	// +X = Operator's Right Hip, -X = Operator's Left Hip
	// +Y = Operator's Front (Belt Buckle), -Y = Operator's Back
	// +Z = Up along Spine (Waist belt sits at Z = +6.2 cm above Pelvis joint)
	switch (Item)
	{
	case EFPSBeltItemType::Mag1:
		// Front-Left Primary Fast-Mag (ergonomically positioned for left-hand speed reload)
		return FVector(-11.8f, 13.4f, 6.2f);
	case EFPSBeltItemType::Mag2:
		// Front-Left Secondary Fast-Mag (adjacent on left hip arc)
		return FVector(-17.4f, 8.8f, 6.2f);
	case EFPSBeltItemType::FragGrenade:
		// Front-Right M67 Fragmentation Grenade
		return FVector(12.2f, 13.8f, 6.4f);
	case EFPSBeltItemType::Flashbang:
	default:
		// Right-Hip M84 Stun / Flashbang Grenade
		return FVector(18.0f, 7.6f, 6.0f);
	}
}

FRotator UFPSBeltGearComponent::GetItemLocalRotation(EFPSBeltItemType Item) const
{
	switch (Item)
	{
	case EFPSBeltItemType::Mag1:
		// Tangent to front-left waist curve, angled slightly forward for ergonomic draw
		return FRotator(8.0f, -36.0f, -6.0f);
	case EFPSBeltItemType::Mag2:
		return FRotator(8.0f, -62.0f, -6.0f);
	case EFPSBeltItemType::FragGrenade:
		// Spoon facing outward/right so pull-ring faces forward
		return FRotator(0.0f, 25.0f, 8.0f);
	case EFPSBeltItemType::Flashbang:
	default:
		return FRotator(0.0f, 55.0f, 6.0f);
	}
}

FVector UFPSBeltGearComponent::GetBeltItemWorldLocation(EFPSBeltItemType Item) const
{
	FVector PelvisOrigin, Right, Fwd, Up;
	FQuat PelvisQuat;
	if (!ComputePelvisOrthonormalFrame(PelvisOrigin, PelvisQuat, Right, Fwd, Up))
	{
		return GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
	}

	const FVector LocalOff = GetItemLocalOffset(Item);
	return PelvisOrigin + Right * LocalOff.X + Fwd * LocalOff.Y + Up * LocalOff.Z;
}

FVector UFPSBeltGearComponent::GetBeltItemComponentSpaceLocation(EFPSBeltItemType Item) const
{
	if (!TargetBodyMesh)
	{
		return FVector::ZeroVector;
	}
	const FVector WorldLoc = GetBeltItemWorldLocation(Item);
	return TargetBodyMesh->GetComponentTransform().InverseTransformPosition(WorldLoc);
}

void UFPSBeltGearComponent::UpdateReloadBeltGrabState(bool bIsReloading, float ReloadAlpha, bool bHasReserveAmmo)
{
	// During reload [0.30 .. 0.75]:
	// hand_l grabs the magazine out of BeltMagComp_1 (leaving the empty Kydex pouch on the belt!)
	// and carries HeldMagComp into the rifle's magwell.
	const bool bNewMag1Grabbed = bIsReloading && (ReloadAlpha >= 0.30f && ReloadAlpha <= 0.85f);
	const bool bNewHeldMagVis  = bIsReloading && (ReloadAlpha >= 0.30f && ReloadAlpha <= 0.73f);

	if (bNewMag1Grabbed != bMag1Grabbed)
	{
		bMag1Grabbed = bNewMag1Grabbed;
		if (BeltMagComp_1)
		{
			// Keep Section 0 & 1 (Kydex pouch & bands) visible on the belt; hide Section 2 & 3 (magazine & rounds) while grabbed!
			BeltMagComp_1->SetMeshSectionVisible(2, !bMag1Grabbed && bHasReserveAmmo);
			BeltMagComp_1->SetMeshSectionVisible(3, !bMag1Grabbed && bHasReserveAmmo);
		}
	}

	if (bNewHeldMagVis != bHeldMagVisible)
	{
		bHeldMagVisible = bNewHeldMagVis;
		if (HeldMagComp)
		{
			HeldMagComp->SetVisibility(bHeldMagVisible && bMasterVisible);
			HeldMagComp->SetHiddenInGame(!(bHeldMagVisible && bMasterVisible));
		}
	}
}

void UFPSBeltGearComponent::UpdateThrowBeltGrabState(
	bool bIsThrowing,
	float ThrowAlpha,
	EFPSBeltItemType ThrowItem,
	bool bHasFragRemaining,
	bool bHasFlashRemaining)
{
	const bool bUnclippedFromBelt = bIsThrowing && (ThrowAlpha >= 0.18f);
	const bool bInHandBeforeRelease = bIsThrowing && (ThrowAlpha >= 0.18f && ThrowAlpha <= 0.56f);

	const bool bNewFragGrabbed  = (bUnclippedFromBelt && ThrowItem == EFPSBeltItemType::FragGrenade) || !bHasFragRemaining;
	const bool bNewFlashGrabbed = (bUnclippedFromBelt && ThrowItem == EFPSBeltItemType::Flashbang)   || !bHasFlashRemaining;

	if (bNewFragGrabbed != bFragGrabbed)
	{
		bFragGrabbed = bNewFragGrabbed;
		if (BeltFragComp)
		{
			BeltFragComp->SetVisibility(!bFragGrabbed && bMasterVisible);
			BeltFragComp->SetHiddenInGame(bFragGrabbed || !bMasterVisible);
		}
	}

	if (bNewFlashGrabbed != bFlashGrabbed)
	{
		bFlashGrabbed = bNewFlashGrabbed;
		if (BeltFlashComp)
		{
			BeltFlashComp->SetVisibility(!bFlashGrabbed && bMasterVisible);
			BeltFlashComp->SetHiddenInGame(bFlashGrabbed || !bMasterVisible);
		}
	}

	const bool bNewHeldFrag  = bInHandBeforeRelease && (ThrowItem == EFPSBeltItemType::FragGrenade);
	const bool bNewHeldFlash = bInHandBeforeRelease && (ThrowItem == EFPSBeltItemType::Flashbang);

	if (bNewHeldFrag != bHeldFragVisible)
	{
		bHeldFragVisible = bNewHeldFrag;
		if (HeldFragComp)
		{
			HeldFragComp->SetVisibility(bHeldFragVisible && bMasterVisible);
			HeldFragComp->SetHiddenInGame(!(bHeldFragVisible && bMasterVisible));
		}
	}

	if (bNewHeldFlash != bHeldFlashVisible)
	{
		bHeldFlashVisible = bNewHeldFlash;
		if (HeldFlashComp)
		{
			HeldFlashComp->SetVisibility(bHeldFlashVisible && bMasterVisible);
			HeldFlashComp->SetHiddenInGame(!(bHeldFlashVisible && bMasterVisible));
		}
	}
}

void UFPSBeltGearComponent::SetBeltVisibility(bool bVisible)
{
	bMasterVisible = bVisible;
	if (BeltMagComp_1) { BeltMagComp_1->SetVisibility(bVisible); BeltMagComp_1->SetHiddenInGame(!bVisible); }
	if (BeltMagComp_2) { BeltMagComp_2->SetVisibility(bVisible); BeltMagComp_2->SetHiddenInGame(!bVisible); }
	if (BeltFragComp)  { BeltFragComp->SetVisibility(bVisible && !bFragGrabbed);  BeltFragComp->SetHiddenInGame(!bVisible || bFragGrabbed); }
	if (BeltFlashComp) { BeltFlashComp->SetVisibility(bVisible && !bFlashGrabbed); BeltFlashComp->SetHiddenInGame(!bVisible || bFlashGrabbed); }
	if (HeldMagComp)   { HeldMagComp->SetVisibility(bVisible && bHeldMagVisible);   HeldMagComp->SetHiddenInGame(!bVisible || !bHeldMagVisible); }
	if (HeldFragComp)  { HeldFragComp->SetVisibility(bVisible && bHeldFragVisible);  HeldFragComp->SetHiddenInGame(!bVisible || !bHeldFragVisible); }
	if (HeldFlashComp) { HeldFlashComp->SetVisibility(bVisible && bHeldFlashVisible); HeldFlashComp->SetHiddenInGame(!bVisible || !bHeldFlashVisible); }
}

void UFPSBeltGearComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateBeltTransforms();
}

void UFPSBeltGearComponent::UpdateBeltTransforms()
{
	FVector PelvisOrigin, Right, Fwd, Up;
	FQuat PelvisQuat;
	if (!ComputePelvisOrthonormalFrame(PelvisOrigin, PelvisQuat, Right, Fwd, Up))
	{
		return;
	}

	auto PinBeltItem = [&](UProceduralMeshComponent* Comp, EFPSBeltItemType Item)
	{
		if (!Comp) return;
		const FVector LocalOff = GetItemLocalOffset(Item);
		const FVector WorldPos = PelvisOrigin + Right * LocalOff.X + Fwd * LocalOff.Y + Up * LocalOff.Z;
		const FQuat WorldRot = PelvisQuat * GetItemLocalRotation(Item).Quaternion();
		Comp->SetWorldLocationAndRotation(WorldPos, WorldRot);
		Comp->SetWorldScale3D(FVector(1.0f, 1.0f, 1.0f));
	};

	PinBeltItem(BeltMagComp_1, EFPSBeltItemType::Mag1);
	PinBeltItem(BeltMagComp_2, EFPSBeltItemType::Mag2);
	PinBeltItem(BeltFragComp,  EFPSBeltItemType::FragGrenade);
	PinBeltItem(BeltFlashComp, EFPSBeltItemType::Flashbang);

	// Pin active hand-held proxy meshes to the operator's hands
	if (bHeldMagVisible && HeldMagComp && TargetBodyMesh)
	{
		const FVector HandLPos = TargetBodyMesh->GetBoneLocation(FName(TEXT("hand_l")), EBoneSpaces::WorldSpace);
		const FVector LowerArmLPos = TargetBodyMesh->GetBoneLocation(FName(TEXT("lowerarm_l")), EBoneSpaces::WorldSpace);
		const FVector ForearmDir = (HandLPos - LowerArmLPos).GetSafeNormal();
		const FQuat HandQuat = FRotationMatrix::MakeFromXZ(ForearmDir.IsNearlyZero() ? Fwd : ForearmDir, Up).ToQuat();
		HeldMagComp->SetWorldLocationAndRotation(HandLPos + Up * 2.5f + Fwd * 2.0f, HandQuat);
		HeldMagComp->SetWorldScale3D(FVector(1.0f, 1.0f, 1.0f));
	}

	if ((bHeldFragVisible || bHeldFlashVisible) && TargetBodyMesh)
	{
		const FVector HandRPos = TargetBodyMesh->GetBoneLocation(FName(TEXT("hand_r")), EBoneSpaces::WorldSpace);
		const FQuat GrenadeQuat = PelvisQuat;
		if (bHeldFragVisible && HeldFragComp)
		{
			HeldFragComp->SetWorldLocationAndRotation(HandRPos + Fwd * 3.0f + Up * 1.5f, GrenadeQuat);
			HeldFragComp->SetWorldScale3D(FVector(1.0f, 1.0f, 1.0f));
		}
		if (bHeldFlashVisible && HeldFlashComp)
		{
			HeldFlashComp->SetWorldLocationAndRotation(HandRPos + Fwd * 3.0f + Up * 1.5f, GrenadeQuat);
			HeldFlashComp->SetWorldScale3D(FVector(1.0f, 1.0f, 1.0f));
		}
	}
}
