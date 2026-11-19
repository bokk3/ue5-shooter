#include "Character/FPSCharacterPreviewActor.h"
#include "Character/FPSOperatorGearComponent.h"
#include "Character/FPSBeltGearComponent.h"
#include "Character/FPSBreathVaporComponent.h"
#include "Character/FPSAnimInstance.h"
#include "Framework/Application/SlateApplication.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Camera/CameraComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Engine/GameInstance.h"
#include "Economy/FPSEconomySubsystem.h"
#include "ProceduralMeshComponent.h"
#include "Engine/Texture2D.h"
#include "TextureResource.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	struct FDuvelMeshSection
	{
		TArray<FVector> Vertices;
		TArray<int32> Triangles;
		TArray<FVector> Normals;
		TArray<FVector2D> UVs;
		TArray<FColor> Colors;
		TArray<FProcMeshTangent> Tangents;
	};

	void AddOrientedQuad(
		FDuvelMeshSection& Sec,
		const FVector& P0, const FVector& P1, const FVector& P2, const FVector& P3,
		const FVector& ExpectedNormal)
	{
		const int32 Base = Sec.Vertices.Num();
		FVector CalcNorm = FVector::CrossProduct(P2 - P0, P1 - P0).GetSafeNormal();
		if (CalcNorm.IsNearlyZero())
		{
			CalcNorm = ExpectedNormal.GetSafeNormal();
		}
		const bool bFlip = (FVector::DotProduct(CalcNorm, ExpectedNormal) < 0.0f);
		const FVector FinalNorm = bFlip ? -CalcNorm : CalcNorm;
		const FVector TangentDir = (P1 - P0).GetSafeNormal();
		const FProcMeshTangent Tang(TangentDir.IsNearlyZero() ? FVector::ForwardVector : TangentDir, false);

		Sec.Vertices.Add(P0);
		Sec.Vertices.Add(P1);
		Sec.Vertices.Add(P2);
		Sec.Vertices.Add(P3);

		for (int32 i = 0; i < 4; ++i)
		{
			Sec.Normals.Add(FinalNorm);
			Sec.Colors.Add(FColor::White);
			Sec.Tangents.Add(Tang);
		}
		Sec.UVs.Add(FVector2D(0.0f, 1.0f));
		Sec.UVs.Add(FVector2D(1.0f, 1.0f));
		Sec.UVs.Add(FVector2D(1.0f, 0.0f));
		Sec.UVs.Add(FVector2D(0.0f, 0.0f));

		if (!bFlip)
		{
			Sec.Triangles.Add(Base + 0); Sec.Triangles.Add(Base + 1); Sec.Triangles.Add(Base + 2);
			Sec.Triangles.Add(Base + 0); Sec.Triangles.Add(Base + 2); Sec.Triangles.Add(Base + 3);
		}
		else
		{
			Sec.Triangles.Add(Base + 0); Sec.Triangles.Add(Base + 2); Sec.Triangles.Add(Base + 1);
			Sec.Triangles.Add(Base + 0); Sec.Triangles.Add(Base + 3); Sec.Triangles.Add(Base + 2);
		}
	}

	// Adds an oriented 3D box centered at Center with local axes AxisX, AxisY, AxisZ and half-extents Ext
	void AddOrientedBox(
		FDuvelMeshSection& Sec,
		const FVector& Center,
		const FVector& AxisX,
		const FVector& AxisY,
		const FVector& AxisZ,
		const FVector& HalfExt)
	{
		const FVector X = AxisX * HalfExt.X;
		const FVector Y = AxisY * HalfExt.Y;
		const FVector Z = AxisZ * HalfExt.Z;

		// +X face
		AddOrientedQuad(Sec, Center + X - Y - Z, Center + X + Y - Z, Center + X + Y + Z, Center + X - Y + Z, AxisX);
		// -X face
		AddOrientedQuad(Sec, Center - X + Y - Z, Center - X - Y - Z, Center - X - Y + Z, Center - X + Y + Z, -AxisX);
		// +Y face
		AddOrientedQuad(Sec, Center + X + Y - Z, Center - X + Y - Z, Center - X + Y + Z, Center + X + Y + Z, AxisY);
		// -Y face
		AddOrientedQuad(Sec, Center - X - Y - Z, Center + X - Y - Z, Center + X - Y + Z, Center - X - Y + Z, -AxisY);
		// +Z face
		AddOrientedQuad(Sec, Center - X - Y + Z, Center + X - Y + Z, Center + X + Y + Z, Center - X + Y + Z, AxisZ);
		// -Z face
		AddOrientedQuad(Sec, Center - X + Y - Z, Center + X + Y - Z, Center + X - Y - Z, Center - X - Y - Z, -AxisZ);
	}

	// Adds a vertical cylinder frustum segment between Z0 (Radius R0) and Z1 (Radius R1)
	void AddVerticalLatheSegment(
		FDuvelMeshSection& Sec,
		const FVector& BaseCenter,
		float Z0, float R0,
		float Z1, float R1,
		int32 Sides = 12,
		bool bCapTop = false)
	{
		for (int32 i = 0; i < Sides; ++i)
		{
			const float A0 = (2.0f * PI * i) / Sides;
			const float A1 = (2.0f * PI * (i + 1)) / Sides;
			const FVector Dir0(FMath::Cos(A0), FMath::Sin(A0), 0.0f);
			const FVector Dir1(FMath::Cos(A1), FMath::Sin(A1), 0.0f);

			const FVector P0 = BaseCenter + Dir0 * R0 + FVector(0.0f, 0.0f, Z0);
			const FVector P1 = BaseCenter + Dir1 * R0 + FVector(0.0f, 0.0f, Z0);
			const FVector P2 = BaseCenter + Dir1 * R1 + FVector(0.0f, 0.0f, Z1);
			const FVector P3 = BaseCenter + Dir0 * R1 + FVector(0.0f, 0.0f, Z1);

			const FVector OutNorm = ((Dir0 + Dir1) * 0.5f + FVector(0.0f, 0.0f, (R0 - R1) / FMath::Max(0.1f, Z1 - Z0))).GetSafeNormal();
			AddOrientedQuad(Sec, P0, P1, P2, P3, OutNorm);

			if (bCapTop)
			{
				const FVector TopC = BaseCenter + FVector(0.0f, 0.0f, Z1);
				AddOrientedQuad(Sec, TopC, P3, P2, TopC, FVector::UpVector);
			}
		}
	}

	// Adds a stubby Belgian 33cl Duvel bottle ("steini" bulbous profile) with dark brown glass, white body label, and crimped silver-gold crown cap
	void AddDuvelBottle(
		FDuvelMeshSection& GlassSec,
		FDuvelMeshSection& WhiteLabelSec,
		FDuvelMeshSection& RedAccentSec,
		FDuvelMeshSection& CrownCapSec,
		const FVector& BottleBottomCenter,
		float Scale = 1.0f,
		bool bIncludeBodyLabel = true)
	{
		// Classic stubby Duvel 33cl profile:
		// 0..8.2 cm: wide cylindrical body (R = 3.15 cm)
		// 8.2..12.2 cm: bulbous rounded shoulder tapering from R=3.15 to R=1.35
		// 12.2..15.8 cm: dark brown glass neck tapering from R=1.35 to R=1.18
		// 15.8..16.9 cm: crimped silver-gold/white metallic crown cap (R=1.34) with red Duvel dot on top
		AddVerticalLatheSegment(GlassSec, BottleBottomCenter, 0.0f * Scale, 3.05f * Scale, 1.2f * Scale, 3.16f * Scale, 12, false);
		AddVerticalLatheSegment(GlassSec, BottleBottomCenter, 1.2f * Scale, 3.16f * Scale, 8.2f * Scale, 3.16f * Scale, 12, false);
		AddVerticalLatheSegment(GlassSec, BottleBottomCenter, 8.2f * Scale, 3.16f * Scale, 10.6f * Scale, 2.52f * Scale, 12, false);
		AddVerticalLatheSegment(GlassSec, BottleBottomCenter, 10.6f * Scale, 2.52f * Scale, 12.6f * Scale, 1.36f * Scale, 12, false);
		AddVerticalLatheSegment(GlassSec, BottleBottomCenter, 12.6f * Scale, 1.36f * Scale, 15.8f * Scale, 1.18f * Scale, 12, false);

		if (bIncludeBodyLabel)
		{
			AddVerticalLatheSegment(WhiteLabelSec, BottleBottomCenter, 2.6f * Scale, 3.21f * Scale, 6.8f * Scale, 3.21f * Scale, 12, false);
			AddVerticalLatheSegment(RedAccentSec, BottleBottomCenter, 4.1f * Scale, 3.25f * Scale, 5.5f * Scale, 3.25f * Scale, 12, false);
		}

		// Crimped metallic silver-gold/white crown cap at the top of the neck (Z = 15.7..16.9 cm)
		AddVerticalLatheSegment(CrownCapSec, BottleBottomCenter, 15.7f * Scale, 1.38f * Scale, 16.4f * Scale, 1.34f * Scale, 12, false);
		AddVerticalLatheSegment(CrownCapSec, BottleBottomCenter, 16.4f * Scale, 1.34f * Scale, 16.9f * Scale, 1.26f * Scale, 12, true);
		// Tiny red Duvel emblem dot in the center of the crown cap
		AddVerticalLatheSegment(RedAccentSec, BottleBottomCenter, 16.9f * Scale, 0.55f * Scale, 16.98f * Scale, 0.52f * Scale, 8, true);
	}

	// Adds the authentic Belgian "Duvel - Anno 1871" heraldic coat-of-arms & gothic calligraphy plaque onto a crate wall
	void AddDuvelLabelPlaqueOnWall(
		FDuvelMeshSection& WhitePlaqueSec,
		FDuvelMeshSection& RedDuvelSec,
		FDuvelMeshSection& GoldCrestSec,
		FDuvelMeshSection& BlackShadowSec,
		const FVector& PlaqueCenter,
		const FVector& WallOutNormal,
		float PlaqueWidth,
		float PlaqueHeight)
	{
		const FVector LabelRight = FVector::CrossProduct(WallOutNormal, FVector::UpVector).GetSafeNormal();
		const FVector LabelUp = FVector::UpVector;
		const float HalfLW = PlaqueWidth * 0.5f;
		const float HalfLH = PlaqueHeight * 0.5f;

		// 1. Crisp Off-White Rectangular Plaque Background
		const FVector P0 = PlaqueCenter - LabelRight * HalfLW - LabelUp * HalfLH;
		const FVector P1 = PlaqueCenter + LabelRight * HalfLW - LabelUp * HalfLH;
		const FVector P2 = PlaqueCenter + LabelRight * HalfLW + LabelUp * HalfLH;
		const FVector P3 = PlaqueCenter - LabelRight * HalfLW + LabelUp * HalfLH;
		AddOrientedQuad(WhitePlaqueSec, P0, P1, P2, P3, WallOutNormal);

		static const uint8 RedRects[][4] = {
			{109,12,111,13}, {112,12,113,13}, {87,13,89,14}, {109,13,110,14}, {111,13,112,14}, {88,14,89,15}, {90,14,91,15}, {99,13,100,15}, {85,14,86,16}, {92,15,94,16}, {100,15,101,16}, {107,15,109,16},
			{111,15,112,16}, {91,16,94,17}, {109,16,111,17}, {114,14,115,17}, {89,17,91,18}, {93,17,94,18}, {107,16,108,18}, {110,17,111,18}, {86,18,87,19}, {88,18,90,19}, {104,18,106,19}, {108,18,109,19},
			{96,18,97,20}, {104,19,105,20}, {95,20,96,21}, {105,20,107,21}, {113,20,114,21}, {87,21,88,22}, {93,21,95,22}, {102,21,103,22}, {107,21,108,22}, {112,21,113,22}, {115,21,116,22}, {85,22,87,23},
			{88,22,90,23}, {92,22,93,23}, {95,22,97,23}, {105,22,106,23}, {109,22,110,23}, {111,22,112,23}, {113,22,114,23}, {116,22,117,23}, {87,23,88,24}, {93,23,94,24}, {104,23,105,24}, {107,23,109,24},
			{114,23,117,24}, {83,24,84,25}, {85,24,86,25}, {89,24,90,25}, {97,24,98,25}, {105,24,106,25}, {83,25,85,26}, {91,25,92,26}, {94,25,95,26}, {98,25,99,26}, {102,25,103,26}, {110,25,112,26},
			{113,25,115,26}, {86,26,88,27}, {90,26,92,27}, {105,26,106,27}, {112,26,115,27}, {85,27,86,28}, {87,27,88,28}, {90,27,91,28}, {92,27,93,28}, {99,27,100,28}, {104,27,105,28}, {109,27,111,28},
			{115,27,116,28}, {86,28,87,29}, {102,27,103,29}, {114,28,116,29}, {85,29,86,31}, {25,31,31,32}, {23,32,34,33}, {86,32,87,33}, {114,31,115,33}, {22,33,37,34}, {89,33,90,34}, {176,33,177,34},
			{21,34,41,35}, {112,34,113,35}, {175,34,180,35}, {20,35,44,36}, {85,35,86,36}, {113,35,114,36}, {116,35,117,36}, {173,35,181,36}, {19,36,47,37}, {87,36,88,37}, {95,35,96,37}, {109,36,110,37},
			{115,36,116,37}, {172,36,182,37}, {18,37,51,38}, {85,37,87,38}, {96,37,97,38}, {171,37,182,38}, {17,38,54,39}, {111,38,112,39}, {169,38,182,39}, {16,39,55,40}, {168,39,183,40}, {15,40,56,41},
			{89,39,90,41}, {15,41,17,42}, {25,41,57,42}, {169,40,183,42}, {29,42,57,43}, {32,43,57,44}, {32,44,33,45}, {35,44,57,45}, {29,45,34,46}, {38,45,57,46}, {170,42,183,46}, {27,46,33,47},
			{42,46,57,47}, {26,47,33,48}, {24,48,33,49}, {22,49,33,50}, {154,49,159,50}, {108,50,109,51}, {149,50,159,51}, {70,51,73,52}, {88,51,92,52}, {107,51,111,52}, {120,51,121,52}, {124,51,125,52},
			{126,51,128,52}, {130,51,131,52}, {144,51,160,52}, {69,52,75,53}, {87,52,94,53}, {105,52,112,53}, {67,53,76,54}, {85,53,95,54}, {104,53,113,54}, {138,52,161,54}, {66,54,76,55}, {84,54,95,55},
			{103,54,113,55}, {151,54,162,55}, {44,47,57,56}, {64,55,77,56}, {82,55,95,56}, {101,55,114,56}, {151,55,163,56}, {45,56,57,57}, {63,56,77,57}, {81,56,95,57}, {152,56,163,57}, {171,46,183,57},
			{64,57,77,58}, {100,56,114,58}, {82,57,95,59}, {101,58,114,59}, {153,57,164,59}, {44,57,57,60}, {154,59,165,60}, {83,59,95,61}, {154,60,166,61}, {44,60,58,62}, {155,61,166,62}, {45,62,58,63},
			{156,62,167,63}, {156,63,164,64}, {155,64,162,65}, {44,63,58,66}, {153,65,160,66}, {45,66,58,67}, {138,54,150,67}, {151,66,158,67}, {138,67,156,68}, {138,68,154,69}, {44,67,58,70}, {65,58,77,70},
			{138,69,152,70}, {45,70,59,71}, {20,50,33,72}, {45,71,57,72}, {66,70,77,72}, {102,59,114,72}, {20,72,43,73}, {44,72,55,73}, {170,57,183,73}, {20,73,54,74}, {66,72,78,74}, {102,72,115,74},
			{120,52,132,74}, {138,70,150,74}, {160,73,164,74}, {20,74,52,75}, {83,61,96,75}, {102,74,116,75}, {120,74,130,75}, {171,73,184,75}, {20,75,51,76}, {66,74,79,76}, {83,75,97,76}, {102,75,117,76},
			{120,75,127,76}, {138,74,163,76}, {171,75,185,76}, {20,76,50,77}, {66,76,81,77}, {83,76,98,77}, {103,76,119,77}, {120,76,124,77}, {171,76,187,77}, {20,77,49,78}, {66,77,99,78}, {103,77,122,78},
			{172,77,186,78}, {20,78,48,79}, {67,78,82,79}, {84,78,98,79}, {104,78,120,79}, {139,76,162,79}, {172,78,185,79}, {20,79,47,80}, {68,79,81,80}, {85,79,96,80}, {105,79,118,80}, {140,79,161,80},
			{173,79,184,80}, {20,80,46,81}, {69,80,80,81}, {86,80,95,81}, {106,80,116,81}, {141,80,161,81}, {174,80,183,81}, {20,81,45,82}, {71,81,78,82}, {87,81,94,82}, {107,81,115,82}, {142,81,160,82},
			{176,81,182,82}, {20,82,25,83}, {37,82,45,83}, {73,82,77,83}, {89,82,92,83}, {109,82,113,83}, {144,82,158,83}, {159,82,160,83}, {177,82,181,83}, {20,83,22,84}, {40,83,44,84}, {74,83,75,84},
			{111,83,112,84}, {179,83,180,84}, {42,84,43,85}
		};
		static const uint8 GoldRects[][4] = {
			{2,2,198,4}, {111,12,112,13}, {113,12,114,13}, {89,13,91,14}, {100,13,102,14}, {108,13,109,14}, {113,13,115,14}, {91,14,92,15}, {94,14,95,15}, {100,14,103,15}, {107,14,108,15}, {110,13,111,15},
			{86,13,87,16}, {89,14,90,16}, {109,15,111,16}, {112,14,114,16}, {90,16,91,17}, {95,16,96,17}, {97,16,99,17}, {100,16,103,17}, {113,16,114,17}, {85,16,88,18}, {96,17,98,18}, {101,17,102,18},
			{109,17,110,18}, {111,17,112,18}, {114,17,115,18}, {94,18,96,19}, {97,18,98,19}, {91,19,96,20}, {105,19,107,20}, {113,19,114,20}, {91,20,95,21}, {100,20,102,21}, {107,20,111,21}, {114,20,115,21},
			{89,21,93,22}, {97,21,102,22}, {103,21,105,22}, {108,21,111,22}, {113,21,114,22}, {87,22,88,23}, {90,22,92,23}, {97,22,105,23}, {110,22,111,23}, {117,22,118,23}, {86,23,87,24}, {94,23,95,24},
			{97,23,104,24}, {106,22,107,24}, {84,24,85,25}, {92,24,96,25}, {98,24,104,25}, {106,24,110,25}, {113,24,114,25}, {117,24,118,25}, {95,25,96,26}, {99,25,102,26}, {105,25,106,26}, {107,25,109,26},
			{62,26,63,27}, {81,26,82,27}, {93,25,94,27}, {96,26,97,27}, {100,26,102,27}, {104,26,105,27}, {107,26,108,27}, {110,26,111,27}, {128,26,130,27}, {132,26,135,27}, {62,27,64,28}, {70,27,72,28},
			{74,27,75,28}, {86,27,87,28}, {89,27,90,28}, {97,27,99,28}, {103,27,104,28}, {124,26,126,28}, {133,27,135,28}, {136,26,138,28}, {65,27,68,29}, {69,28,74,29}, {92,28,93,29}, {94,28,95,29},
			{99,28,100,29}, {101,27,102,29}, {105,28,107,29}, {108,28,109,29}, {128,27,131,29}, {133,28,134,29}, {61,28,64,30}, {65,29,66,30}, {67,29,70,30}, {71,29,74,30}, {75,28,76,30}, {91,29,92,30},
			{115,29,116,30}, {125,28,126,30}, {128,29,129,30}, {130,29,131,30}, {132,29,134,30}, {137,28,138,30}, {61,30,62,31}, {64,30,66,31}, {67,30,68,31}, {69,30,70,31}, {71,30,72,31}, {73,30,76,31},
			{113,30,115,31}, {124,30,127,31}, {128,30,131,31}, {132,30,133,31}, {136,30,138,31}, {115,31,116,32}, {87,32,90,33}, {93,30,94,33}, {107,30,109,33}, {112,31,114,33}, {85,33,89,34}, {91,33,92,34},
			{110,33,113,34}, {114,33,116,34}, {85,34,87,35}, {89,34,92,35}, {110,34,112,35}, {113,34,115,35}, {86,35,89,36}, {90,35,92,36}, {104,35,106,36}, {109,35,113,36}, {114,35,116,36}, {85,36,87,37},
			{89,36,93,37}, {96,35,98,37}, {104,36,107,37}, {114,36,115,37}, {116,36,117,37}, {88,37,92,38}, {110,36,113,38}, {117,37,118,38}, {84,37,85,39}, {88,38,89,39}, {90,38,92,39}, {109,38,111,39},
			{112,38,114,39}, {55,39,56,40}, {90,39,91,40}, {108,39,109,40}, {110,39,112,40}, {100,39,102,41}, {103,42,106,43}, {57,43,58,45}, {2,4,4,96}, {196,4,198,96}, {2,96,198,98}
		};
		static const uint8 BlackRects[][4] = {
			{102,13,103,14}, {87,15,88,16}, {98,15,100,16}, {101,15,103,16}, {88,16,89,17}, {103,16,104,17}, {111,16,112,17}, {91,17,92,18}, {94,17,95,18}, {108,16,109,18}, {115,17,116,18}, {87,18,88,19},
			{91,18,94,19}, {106,18,108,19}, {110,18,112,19}, {114,18,116,19}, {85,19,87,20}, {88,19,91,20}, {107,19,109,20}, {115,19,117,20}, {84,20,86,21}, {85,21,86,22}, {95,21,96,22}, {105,21,106,22},
			{93,22,94,23}, {112,22,113,23}, {114,22,115,23}, {84,23,85,24}, {90,23,93,24}, {110,23,114,24}, {117,23,118,24}, {86,24,89,25}, {90,24,92,25}, {96,23,97,25}, {104,24,105,25}, {110,24,111,25},
			{114,24,115,25}, {116,24,117,25}, {85,25,87,26}, {88,25,90,26}, {97,25,98,26}, {115,25,116,26}, {118,25,119,26}, {82,26,86,27}, {89,26,90,27}, {92,26,93,27}, {98,26,99,27}, {102,26,103,27},
			{106,26,107,27}, {109,26,110,27}, {93,27,96,28}, {106,27,109,28}, {111,27,114,28}, {87,28,92,29}, {109,28,111,29}, {113,28,114,29}, {92,29,93,30}, {86,29,88,31}, {85,31,86,32}, {87,31,89,32},
			{111,32,112,33}, {90,33,91,34}, {94,33,109,34}, {93,34,109,35}, {92,35,95,36}, {106,35,108,36}, {84,35,85,37}, {93,36,95,37}, {98,35,104,37}, {107,36,109,37}, {93,37,96,38}, {97,37,104,38},
			{106,37,109,38}, {115,37,116,38}, {89,38,90,39}, {92,38,108,39}, {114,38,117,39}, {85,38,88,40}, {91,39,92,40}, {112,39,114,40}, {88,40,89,41}, {90,40,92,41}, {94,39,99,41}, {102,39,108,41},
			{110,40,113,41}, {17,41,24,42}, {89,41,90,42}, {95,41,106,42}, {113,41,114,42}, {168,40,169,42}, {14,42,17,43}, {23,42,28,43}, {99,42,102,43}, {113,42,115,43}, {168,42,170,43}, {13,43,14,44},
			{28,43,31,44}, {89,42,91,44}, {100,43,101,44}, {111,43,113,44}, {33,44,35,45}, {35,45,38,46}, {38,46,41,47}, {41,47,44,48}, {119,52,120,53}, {150,55,151,56}, {150,56,152,57}, {99,57,100,58},
			{151,57,152,58}, {63,57,64,59}, {81,58,82,59}, {99,58,101,59}, {152,58,153,59}, {100,59,102,60}, {118,53,120,60}, {152,59,154,60}, {153,60,154,61}, {154,61,155,62}, {154,62,156,63}, {155,63,156,64},
			{164,63,166,64}, {162,64,165,65}, {160,65,163,66}, {158,66,161,67}, {156,67,159,68}, {154,68,156,69}, {152,69,154,70}, {150,70,152,71}, {43,48,44,72}, {57,71,58,72}, {56,72,58,73}, {54,73,56,74},
			{169,43,170,74}, {52,74,54,75}, {64,59,65,75}, {130,74,131,75}, {169,74,171,75}, {51,75,53,76}, {64,75,66,76}, {101,60,102,76}, {119,60,120,76}, {127,75,131,76}, {50,76,51,77}, {82,59,83,77},
			{101,76,103,77}, {125,76,128,77}, {137,54,138,77}, {170,75,171,77}, {49,77,50,78}, {65,76,66,78}, {102,77,103,78}, {122,77,125,78}, {137,77,139,78}, {170,77,172,78}, {48,78,49,79}, {65,78,67,79},
			{83,78,84,79}, {98,78,99,79}, {102,78,104,79}, {120,78,122,79}, {138,78,139,79}, {171,78,172,79}, {186,78,187,79}, {47,79,48,80}, {66,79,68,80}, {81,79,84,80}, {97,79,98,80}, {103,79,105,80},
			{118,79,120,80}, {138,79,140,80}, {172,79,173,80}, {185,79,186,80}, {46,80,47,81}, {67,80,69,81}, {80,80,82,81}, {84,80,85,81}, {95,80,97,81}, {104,80,106,81}, {116,80,118,81}, {139,80,141,81},
			{173,80,174,81}, {184,80,185,81}, {45,81,47,82}, {69,81,71,82}, {78,81,80,82}, {85,81,87,82}, {94,81,96,82}, {105,81,107,82}, {115,81,117,82}, {140,81,142,82}, {174,81,175,82}, {182,81,184,82},
			{26,82,37,83}, {45,82,46,83}, {70,82,72,83}, {77,82,79,83}, {86,82,88,83}, {93,82,94,83}, {107,82,109,83}, {113,82,115,83}, {142,82,144,83}, {175,82,177,83}, {181,82,183,83}, {19,52,20,84},
			{22,83,26,84}, {36,83,40,84}, {72,83,74,84}, {76,83,77,84}, {88,83,93,84}, {109,83,111,84}, {112,83,114,84}, {144,83,159,84}, {176,83,179,84}, {180,83,182,84}, {19,84,22,85}, {39,84,42,85},
			{44,83,45,85}, {74,84,76,85}, {91,84,92,85}, {110,84,113,85}, {179,84,181,85}, {19,85,20,86}, {42,85,44,86}
		};

		const FVector ReliefBase = PlaqueCenter + WallOutNormal * 0.10f;
		auto EmitRectList = [&](FDuvelMeshSection& TargetSec, const uint8 RectList[][4], int32 Count, float DepthOffset)
		{
			const FVector PlaneOrigin = ReliefBase + WallOutNormal * DepthOffset;
			for (int32 i = 0; i < Count; ++i)
			{
				const float U0 = (float(RectList[i][0]) / 200.0f) - 0.5f;
				const float U1 = (float(RectList[i][2]) / 200.0f) - 0.5f;
				const float VTop = 0.5f - (float(RectList[i][1]) / 100.0f);
				const float VBot = 0.5f - (float(RectList[i][3]) / 100.0f);

				const FVector Q0 = PlaneOrigin + LabelRight * (U0 * PlaqueWidth) + LabelUp * (VBot * PlaqueHeight);
				const FVector Q1 = PlaneOrigin + LabelRight * (U1 * PlaqueWidth) + LabelUp * (VBot * PlaqueHeight);
				const FVector Q2 = PlaneOrigin + LabelRight * (U1 * PlaqueWidth) + LabelUp * (VTop * PlaqueHeight);
				const FVector Q3 = PlaneOrigin + LabelRight * (U0 * PlaqueWidth) + LabelUp * (VTop * PlaqueHeight);
				AddOrientedQuad(TargetSec, Q0, Q1, Q2, Q3, WallOutNormal);
			}
		};

		EmitRectList(BlackShadowSec, BlackRects, sizeof(BlackRects) / sizeof(BlackRects[0]), 0.00f);
		EmitRectList(GoldCrestSec,   GoldRects,  sizeof(GoldRects)  / sizeof(GoldRects[0]),  0.03f);
		EmitRectList(RedDuvelSec,    RedRects,   sizeof(RedRects)   / sizeof(RedRects[0]),   0.06f);
	}

	// Builds an authentic Royal Navy Blue Belgian Duvel 24x33cl beer crate matching the reference photo:
	// - Deep royal navy blue HDPE shell with solid walls, protruding top rim lip, and 11 trapezoidal horizontal corner grip ribs
	// - Red inner ergonomic carry-handle inserts inside the left & right short-side handle slots
	// - Centered UV-mapped "Duvel - Anno 1871" white/gold/red heraldic label plaque on all 4 sides
	// - Internal 6x4 bottle divider grid + vertical interior wall buttresses + 24 stubby brown bottles with crown caps peeking at the top rim
	void AddBelgianDuvelCrate(
		FDuvelMeshSection& NavyCrateSec,
		FDuvelMeshSection& LabelPlaqueSec,
		FDuvelMeshSection& RedAccentSec,
		FDuvelMeshSection& GlassBottleSec,
		FDuvelMeshSection& CrownCapSec,
		FDuvelMeshSection& BlackShadowSec,
		const FVector& CrateBottomCenter,
		float YawDeg,
		const FVector& CrateSize,
		bool bPopulateBottles)
	{
		const FQuat RotQ(FVector::UpVector, FMath::DegreesToRadians(YawDeg));
		const FVector AxisX = RotQ.RotateVector(FVector(1.0f, 0.0f, 0.0f)); // Long crate axis (width)
		const FVector AxisY = RotQ.RotateVector(FVector(0.0f, 1.0f, 0.0f)); // Short crate axis (depth)
		const FVector AxisZ = FVector::UpVector;

		const float HalfW = CrateSize.X * 0.5f;
		const float HalfD = CrateSize.Y * 0.5f;
		const float H     = CrateSize.Z;
		const float WallT = 1.5f;

		// 1. Solid Bottom Base Plate & Protruding Bottom Rim Lip (Navy Blue HDPE)
		AddOrientedBox(NavyCrateSec, CrateBottomCenter + AxisZ * 1.1f, AxisX, AxisY, AxisZ, FVector(HalfW + 0.30f, HalfD + 0.30f, 1.1f));

		// 4 Reinforced Corner Pillars
		const float CornerW = 2.8f;
		for (int32 sx = -1; sx <= 1; sx += 2)
		{
			for (int32 sy = -1; sy <= 1; sy += 2)
			{
				const FVector PillarPos = CrateBottomCenter
					+ AxisX * (sx * (HalfW - CornerW * 0.45f))
					+ AxisY * (sy * (HalfD - CornerW * 0.45f))
					+ AxisZ * (H * 0.5f);
				AddOrientedBox(NavyCrateSec, PillarPos, AxisX, AxisY, AxisZ, FVector(CornerW * 0.55f, CornerW * 0.55f, H * 0.5f));
			}
		}

		// 2. Solid Front & Back Long Walls + Protruding Top Rim Overhang + 11 Trapezoidal Horizontal Corner Grip Ribs + Center Duvel Label Plaque
		for (int32 sy = -1; sy <= 1; sy += 2)
		{
			const FVector OutNormal = AxisY * float(sy);
			const FVector WallCenter = CrateBottomCenter + AxisY * (sy * (HalfD - WallT * 0.5f));

			// Full-height solid Navy Blue wall (no holes!)
			AddOrientedBox(NavyCrateSec, WallCenter + AxisZ * (H * 0.5f), AxisX, AxisY, AxisZ, FVector(HalfW, WallT * 0.5f, H * 0.5f));

			// Protruding Top Overhang Rim Lip (Z = H - 1.35 .. H)
			AddOrientedBox(NavyCrateSec, CrateBottomCenter + AxisY * (sy * (HalfD - WallT * 0.35f)) + AxisZ * (H - 0.70f), AxisX, AxisY, AxisZ, FVector(HalfW + 0.45f, WallT * 0.75f, 0.70f));

			// 11 Trapezoidal Horizontal Corner Grip Ribs on Left (-X) & Right (+X) sides of the long wall
			for (int32 r = 0; r < 11; ++r)
			{
				const float RibAlpha = float(r) / 10.0f;
				const float RibZ = 2.6f + RibAlpha * (H - 5.0f);
				// Tapers inward from 21% of crate width at bottom to 15.5% at top, matching the photo's angled grip ribs!
				const float RibSpanX = CrateSize.X * FMath::Lerp(0.21f, 0.155f, RibAlpha);
				for (int32 sx = -1; sx <= 1; sx += 2)
				{
					const float RibCenterX = sx * (HalfW + 0.35f - RibSpanX * 0.5f);
					const FVector RibPos = CrateBottomCenter
						+ AxisX * RibCenterX
						+ AxisY * (sy * (HalfD + 0.18f))
						+ AxisZ * RibZ;
					AddOrientedBox(NavyCrateSec, RibPos, AxisX, AxisY, AxisZ, FVector(RibSpanX * 0.5f, 0.28f, 0.32f));
				}
			}

			// Interior Vertical Wall Buttress Ribs (visible along inside top rim behind the bottles just like the photo)
			for (int32 b = 0; b < 6; ++b)
			{
				const float ButtressX = (-2.5f + b) * ((CrateSize.X - WallT * 2.4f) / 6.0f);
				const FVector ButtressPos = CrateBottomCenter
					+ AxisX * ButtressX
					+ AxisY * (sy * (HalfD - WallT * 1.05f))
					+ AxisZ * (H * 0.52f);
				AddOrientedBox(NavyCrateSec, ButtressPos, AxisX, AxisY, AxisZ, FVector(0.45f, 0.35f, H * 0.44f));
			}

			// Center UV-Mapped "Duvel - Anno 1871" Label Plaque (sitting 0.22 cm proud of the navy wall)
			const FVector PlaqueCenter = CrateBottomCenter
				+ AxisY * (sy * (HalfD + 0.22f))
				+ AxisZ * (H * 0.48f);
			AddDuvelLabelPlaqueOnWall(LabelPlaqueSec, RedAccentSec, CrownCapSec, BlackShadowSec, PlaqueCenter, OutNormal, CrateSize.X * 0.52f, H * 0.58f);
		}

		// 3. Short Left & Right Side Walls + Red Inner Ergonomic Handle Grips + Side Corner Ribs + Side Duvel Label Plaque
		for (int32 sx = -1; sx <= 1; sx += 2)
		{
			const FVector SideOutNormal = AxisX * float(sx);
			const FVector WallCenter = CrateBottomCenter + AxisX * (sx * (HalfW - WallT * 0.5f));

			// Solid lower & middle wall up to handle cutout (Z = 0 .. H - 4.8 cm)
			AddOrientedBox(NavyCrateSec, WallCenter + AxisZ * ((H - 4.8f) * 0.5f), AxisX, AxisY, AxisZ, FVector(WallT * 0.5f, HalfD, (H - 4.8f) * 0.5f));
			// Left & Right upper side shoulders flanking the center hand-grip slot (Z = H - 4.8 .. H - 1.4 cm)
			const float ShoulderSpanY = (HalfD - 5.5f);
			for (int32 sy = -1; sy <= 1; sy += 2)
			{
				AddOrientedBox(
					NavyCrateSec,
					WallCenter + AxisY * (sy * (HalfD - ShoulderSpanY * 0.5f)) + AxisZ * (H - 3.1f),
					AxisX, AxisY, AxisZ,
					FVector(WallT * 0.5f, ShoulderSpanY * 0.5f, 1.7f));
			}
			// Top Overhang Rim Bridge across the top of the short side (Z = H - 1.4 .. H)
			AddOrientedBox(NavyCrateSec, CrateBottomCenter + AxisX * (sx * (HalfW - WallT * 0.35f)) + AxisZ * (H - 0.70f), AxisX, AxisY, AxisZ, FVector(WallT * 0.75f, HalfD + 0.45f, 0.70f));

			// Red Inner Ergonomic Handle Grip Insert (recessed slightly inside the top handle cutout, visible from outside & above just like the photo!)
			const FVector RedGripPos = CrateBottomCenter
				+ AxisX * (sx * (HalfW - WallT * 0.95f))
				+ AxisZ * (H - 3.0f);
			AddOrientedBox(RedAccentSec, RedGripPos, AxisX, AxisY, AxisZ, FVector(0.45f, 5.8f, 1.55f));

			// 11 Horizontal Corner Grip Ribs wrapping onto the short sides
			for (int32 r = 0; r < 11; ++r)
			{
				const float RibAlpha = float(r) / 10.0f;
				const float RibZ = 2.6f + RibAlpha * (H - 5.0f);
				const float RibSpanY = CrateSize.Y * FMath::Lerp(0.18f, 0.14f, RibAlpha);
				for (int32 sy = -1; sy <= 1; sy += 2)
				{
					const float RibCenterY = sy * (HalfD + 0.35f - RibSpanY * 0.5f);
					const FVector RibPos = CrateBottomCenter
						+ AxisX * (sx * (HalfW + 0.18f))
						+ AxisY * RibCenterY
						+ AxisZ * RibZ;
					AddOrientedBox(NavyCrateSec, RibPos, AxisX, AxisY, AxisZ, FVector(0.28f, RibSpanY * 0.5f, 0.32f));
				}
			}

			// Side UV-Mapped "Duvel - Anno 1871" Label Plaque
			const FVector SidePlaqueCenter = CrateBottomCenter
				+ AxisX * (sx * (HalfW + 0.22f))
				+ AxisZ * (H * 0.44f);
			AddDuvelLabelPlaqueOnWall(LabelPlaqueSec, RedAccentSec, CrownCapSec, BlackShadowSec, SidePlaqueCenter, SideOutNormal, CrateSize.Y * 0.54f, H * 0.50f);
		}

		// 4. Internal 6x4 Bottle Compartment Divider Grid (Navy Blue)
		for (int32 ix = 1; ix <= 5; ++ix)
		{
			const float XOff = -HalfW + (CrateSize.X * ix) / 6.0f;
			AddOrientedBox(NavyCrateSec, CrateBottomCenter + AxisX * XOff + AxisZ * (H * 0.44f), AxisX, AxisY, AxisZ, FVector(0.32f, HalfD - WallT, H * 0.38f));
		}
		for (int32 iy = 1; iy <= 3; ++iy)
		{
			const float YOff = -HalfD + (CrateSize.Y * iy) / 4.0f;
			AddOrientedBox(NavyCrateSec, CrateBottomCenter + AxisY * YOff + AxisZ * (H * 0.44f), AxisX, AxisY, AxisZ, FVector(HalfW - WallT, 0.32f, H * 0.38f));
		}

		// 5. Populate all 24 compartments (6 columns x 4 rows) with stubby brown glass Duvel bottles whose crown caps peek right at the top rim!
		if (bPopulateBottles)
		{
			const float CellW = (CrateSize.X - WallT * 2.5f) / 6.0f;
			const float CellD = (CrateSize.Y - WallT * 2.5f) / 4.0f;
			const float BottleScale = 0.94f;
			// Position bottle bottom so top of crown cap (16.9 * 0.94 = 15.88 cm) reaches Z = H - 0.35 cm (just like the reference photo!)
			const float BottleBaseZ = FMath::Max(1.6f, (H - 0.35f) - 16.9f * BottleScale);
			for (int32 r = 0; r < 4; ++r)
			{
				for (int32 c = 0; c < 6; ++c)
				{
					const float LocalX = (-2.5f + c) * CellW;
					const float LocalY = (-1.5f + r) * CellD;
					const FVector BottleBase = CrateBottomCenter + AxisX * LocalX + AxisY * LocalY + AxisZ * BottleBaseZ;
					AddDuvelBottle(GlassBottleSec, LabelPlaqueSec, RedAccentSec, CrownCapSec, BottleBase, BottleScale, false);
				}
			}
		}
	}
}

AFPSCharacterPreviewActor::AFPSCharacterPreviewActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;

	// 1. Root Scene
	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	RootScene->SetMobility(EComponentMobility::Movable);
	RootComponent = RootScene;

	// 2. Camera: Calibrated to frame the full Task Force Echo Operator, Turntable & Hangar Armory Table
	PreviewCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PreviewCamera"));
	PreviewCamera->SetupAttachment(RootComponent);
	PreviewCamera->SetRelativeLocation(FVector(435.0f, 0.0f, 96.0f));
	PreviewCamera->SetRelativeRotation(FRotator(-1.0f, 180.0f, 0.0f));
	PreviewCamera->FieldOfView = 52.0f;
	PreviewCamera->PostProcessSettings.bOverride_AutoExposureMethod = true;
	PreviewCamera->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Histogram;
	PreviewCamera->PostProcessSettings.bOverride_AutoExposureBias = true;
	PreviewCamera->PostProcessSettings.AutoExposureBias = -0.35f;
	PreviewCamera->PostProcessSettings.bOverride_AutoExposureMinBrightness = true;
	PreviewCamera->PostProcessSettings.AutoExposureMinBrightness = 0.40f;
	PreviewCamera->PostProcessSettings.bOverride_AutoExposureMaxBrightness = true;
	PreviewCamera->PostProcessSettings.AutoExposureMaxBrightness = 2.0f;
	PreviewCamera->PostProcessSettings.bOverride_AutoExposureSpeedUp = true;
	PreviewCamera->PostProcessSettings.AutoExposureSpeedUp = 20.0f;
	PreviewCamera->PostProcessSettings.bOverride_AutoExposureSpeedDown = true;
	PreviewCamera->PostProcessSettings.AutoExposureSpeedDown = 20.0f;
	PreviewCamera->PostProcessSettings.bOverride_BloomIntensity = true;
	PreviewCamera->PostProcessSettings.BloomIntensity = 0.25f;

	// 3. Studio Lighting: 3-point tactical dynamic lighting
	StudioKeyLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("StudioKeyLight"));
	StudioKeyLight->SetupAttachment(RootComponent);
	StudioKeyLight->SetMobility(EComponentMobility::Movable);
	StudioKeyLight->SetRelativeLocation(FVector(190.0f, -120.0f, 210.0f));
	StudioKeyLight->SetRelativeRotation(FRotator(-35.0f, 145.0f, 0.0f));
	StudioKeyLight->SetIntensity(1800.0f);
	StudioKeyLight->SetLightColor(FLinearColor(1.0f, 0.78f, 0.40f)); // Warm amber key light
	StudioKeyLight->SetAttenuationRadius(1500.0f);
	StudioKeyLight->SetInnerConeAngle(25.0f);
	StudioKeyLight->SetOuterConeAngle(65.0f);

	StudioRimLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("StudioRimLight"));
	StudioRimLight->SetupAttachment(RootComponent);
	StudioRimLight->SetMobility(EComponentMobility::Movable);
	StudioRimLight->SetRelativeLocation(FVector(-160.0f, 120.0f, 175.0f));
	StudioRimLight->SetRelativeRotation(FRotator(-25.0f, -35.0f, 0.0f));
	StudioRimLight->SetIntensity(2200.0f);
	StudioRimLight->SetLightColor(FLinearColor(0.0f, 0.898f, 1.0f)); // Tactical cyan rim light (#00E5FF)
	StudioRimLight->SetAttenuationRadius(1400.0f);
	StudioRimLight->SetInnerConeAngle(30.0f);
	StudioRimLight->SetOuterConeAngle(70.0f);
	StudioRimLight->SetCastShadows(false);

	StudioFillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StudioFillLight"));
	StudioFillLight->SetupAttachment(RootComponent);
	StudioFillLight->SetMobility(EComponentMobility::Movable);
	StudioFillLight->SetRelativeLocation(FVector(150.0f, 0.0f, 120.0f));
	StudioFillLight->SetIntensity(1100.0f);
	StudioFillLight->SetLightColor(FLinearColor(0.85f, 0.92f, 1.0f)); // Clean fill
	StudioFillLight->SetAttenuationRadius(1500.0f);
	StudioFillLight->SetCastShadows(false);

	StudioAmbientLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StudioAmbientLight"));
	StudioAmbientLight->SetupAttachment(RootComponent);
	StudioAmbientLight->SetMobility(EComponentMobility::Movable);
	StudioAmbientLight->SetRelativeLocation(FVector(0.0f, 0.0f, 150.0f));
	StudioAmbientLight->SetIntensity(500.0f);
	StudioAmbientLight->SetLightColor(FLinearColor(0.35f, 0.45f, 0.55f));
	StudioAmbientLight->SetAttenuationRadius(1500.0f);
	StudioAmbientLight->SetCastShadows(false);

	StudioSunLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("StudioSunLight"));
	StudioSunLight->SetupAttachment(RootComponent);
	StudioSunLight->SetMobility(EComponentMobility::Movable);
	StudioSunLight->SetRelativeRotation(FRotator(-30.0f, 165.0f, 0.0f));
	StudioSunLight->SetIntensity(0.0f);
	StudioSunLight->SetLightColor(FLinearColor(1.0f, 0.95f, 0.88f));
	StudioSunLight->SetCastShadows(false);

	// 4. Turntable Root (Rotates in Tick)
	TurntableRoot = CreateDefaultSubobject<USceneComponent>(TEXT("TurntableRoot"));
	TurntableRoot->SetupAttachment(RootComponent);
	TurntableRoot->SetRelativeLocation(FVector::ZeroVector);

	// 5. Basic Shape & Operator Resources
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseMatFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	UStaticMesh* CylinderMesh = CylinderFinder.Succeeded() ? CylinderFinder.Object : nullptr;
	UStaticMesh* CubeMesh = CubeFinder.Succeeded() ? CubeFinder.Object : nullptr;
	UStaticMesh* SphereMesh = SphereFinder.Succeeded() ? SphereFinder.Object : nullptr;
	UMaterialInterface* BaseMat = BaseMatFinder.Succeeded() ? BaseMatFinder.Object : nullptr;

	auto SetupMeshComp = [this, BaseMat](TObjectPtr<UStaticMeshComponent>& Comp, const TCHAR* Name, UStaticMesh* Mesh, const FVector& Loc, const FRotator& Rot, const FVector& Scale)
	{
		Comp = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Comp->SetupAttachment(TurntableRoot);
		Comp->SetMobility(EComponentMobility::Movable);
		if (Mesh) Comp->SetStaticMesh(Mesh);
		if (BaseMat) Comp->SetMaterial(0, BaseMat);
		Comp->SetRelativeLocation(Loc);
		Comp->SetRelativeRotation(Rot);
		Comp->SetRelativeScale3D(Scale);
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetCastShadow(true);
	};

	bIsRotating = false;

	// 5. Tactical Turntable Pedestal (Radius 135cm, Height 11cm with beveled rim)
	PedestalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PedestalMesh"));
	PedestalMesh->SetupAttachment(TurntableRoot);
	PedestalMesh->SetMobility(EComponentMobility::Movable);
	PedestalMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -11.0f));
	PedestalMesh->SetRelativeRotation(FRotator::ZeroRotator);
	PedestalMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
	PedestalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PedestalMesh->SetCastShadow(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlinthMeshFinder(TEXT("/Game/Environment/Hangar/SM_Hangar_Plinth.SM_Hangar_Plinth"));
	if (PlinthMeshFinder.Succeeded())
	{
		PedestalMesh->SetStaticMesh(PlinthMeshFinder.Object);
	}
	else if (CylinderMesh)
	{
		PedestalMesh->SetStaticMesh(CylinderMesh);
		PedestalMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -2.0f));
		PedestalMesh->SetRelativeScale3D(FVector(1.4f, 1.4f, 0.04f));
	}

	// 5.5 Static Posed Operator Fallback (kept hidden when live skinned BodyMesh is active)
	OperatorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OperatorMesh"));
	OperatorMesh->SetupAttachment(TurntableRoot);
	OperatorMesh->SetMobility(EComponentMobility::Movable);
	OperatorMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	OperatorMesh->SetRelativeRotation(FRotator(0.0f, 15.0f, 0.0f));
	OperatorMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
	OperatorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OperatorMesh->SetCastShadow(true);
	OperatorMesh->SetVisibility(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PosedSoldierFinder(TEXT("/Game/Characters/BaseMale/SM_Soldier_Posed.SM_Soldier_Posed"));
	if (PosedSoldierFinder.Succeeded())
	{
		OperatorMesh->SetStaticMesh(PosedSoldierFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> SkelSoldierFinder(TEXT("/Game/Characters/BaseMale/SK_BaseMale.SK_BaseMale"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseBodyMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_Operator_BaseBody.M_Operator_BaseBody"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ArmorMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_Operator_Armor.M_Operator_Armor"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> VisorMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_Soldier_Visor.M_Soldier_Visor"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> AKSteelMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Steel.M_AK47_Steel"));

	if (BaseBodyMatFinder.Succeeded()) OperatorMesh->SetMaterial(0, BaseBodyMatFinder.Object);
	if (VisorMatFinder.Succeeded()) OperatorMesh->SetMaterial(1, VisorMatFinder.Object);

	if (AKSteelMatFinder.Succeeded()) PedestalMesh->SetMaterial(0, AKSteelMatFinder.Object);
	if (VisorMatFinder.Succeeded()) PedestalMesh->SetMaterial(1, VisorMatFinder.Object);

	// 6. Live Animated Skeletal Operator (SK_BaseMale driven by UFPSAnimInstance)
	BodyMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(TurntableRoot);
	BodyMesh->SetMobility(EComponentMobility::Movable);
	BodyMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -2.4f));
	BodyMesh->SetRelativeRotation(FRotator(0.0f, 105.0f, 0.0f)); // Faces +X camera with heroic 15-deg turn
	BodyMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->bAlwaysCreatePhysicsState = false;
	BodyMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	BodyMesh->SetCastShadow(true);
	BodyMesh->SetVisibility(true);

	if (SkelSoldierFinder.Succeeded())
	{
		BodyMesh->SetSkeletalMesh(SkelSoldierFinder.Object);
	}
	if (BaseBodyMatFinder.Succeeded())
	{
		BodyMesh->SetMaterial(0, BaseBodyMatFinder.Object);
	}
	if (VisorMatFinder.Succeeded())
	{
		BodyMesh->SetMaterial(1, VisorMatFinder.Object);
	}
	BodyMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	BodyMesh->SetAnimInstanceClass(UFPSAnimInstance::StaticClass());

	// 6.2 Modular 3D Belt Gear Kit (STANAG Fast-Mag Pouches, M67 Frag, M84 Flashbang)
	BeltGearComponent = CreateDefaultSubobject<UFPSBeltGearComponent>(TEXT("BeltGearComponent"));
	BreathVaporComponent = CreateDefaultSubobject<UFPSBreathVaporComponent>(TEXT("BreathVaporComponent"));
	OperatorGearComponent = CreateDefaultSubobject<UFPSOperatorGearComponent>(TEXT("OperatorGearComponent"));

	// 6.5 Hangar Armory Weapon Table & Loadout Rifle Display (Screen Left next to Turntable)
	TableWeaponRoot = CreateDefaultSubobject<USceneComponent>(TEXT("TableWeaponRoot"));
	TableWeaponRoot->SetupAttachment(RootComponent);
	TableWeaponRoot->SetMobility(EComponentMobility::Movable);
	TableWeaponRoot->SetRelativeLocation(FVector(25.0f, 82.0f, 0.0f));
	TableWeaponRoot->SetRelativeRotation(FRotator(0.0f, -24.0f, 0.0f));

	TablePedestalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TablePedestalMesh"));
	TablePedestalMesh->SetupAttachment(TableWeaponRoot);
	TablePedestalMesh->SetMobility(EComponentMobility::Movable);
	if (CubeMesh) TablePedestalMesh->SetStaticMesh(CubeMesh);
	if (AKSteelMatFinder.Succeeded()) TablePedestalMesh->SetMaterial(0, AKSteelMatFinder.Object);
	TablePedestalMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 24.0f));
	TablePedestalMesh->SetRelativeScale3D(FVector(0.42f, 0.78f, 0.48f));
	TablePedestalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TablePedestalMesh->SetCastShadow(true);

	TableRackStandMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TableRackStandMesh"));
	TableRackStandMesh->SetupAttachment(TableWeaponRoot);
	TableRackStandMesh->SetMobility(EComponentMobility::Movable);
	if (CubeMesh) TableRackStandMesh->SetStaticMesh(CubeMesh);
	if (AKSteelMatFinder.Succeeded()) TableRackStandMesh->SetMaterial(0, AKSteelMatFinder.Object);
	TableRackStandMesh->SetRelativeLocation(FVector(2.0f, 0.0f, 50.0f));
	TableRackStandMesh->SetRelativeScale3D(FVector(0.14f, 0.44f, 0.04f));
	TableRackStandMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TableRackStandMesh->SetCastShadow(true);

	TableRifleReceiverMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TableRifleReceiverMesh"));
	TableRifleReceiverMesh->SetupAttachment(TableWeaponRoot);
	TableRifleReceiverMesh->SetMobility(EComponentMobility::Movable);
	TableRifleReceiverMesh->SetRelativeLocation(FVector(4.0f, 0.0f, 60.0f));
	TableRifleReceiverMesh->SetRelativeRotation(FRotator(6.0f, -90.0f, -16.0f));
	TableRifleReceiverMesh->SetRelativeScale3D(FVector(0.88f, 0.88f, 0.88f));
	TableRifleReceiverMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TableRifleReceiverMesh->SetCastShadow(true);

	TableRifleMagMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TableRifleMagMesh"));
	TableRifleMagMesh->SetupAttachment(TableRifleReceiverMesh);
	TableRifleMagMesh->SetMobility(EComponentMobility::Movable);
	TableRifleMagMesh->SetRelativeLocation(FVector(6.0f, 0.0f, -1.5f));
	TableRifleMagMesh->SetRelativeRotation(FRotator::ZeroRotator);
	TableRifleMagMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
	TableRifleMagMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TableRifleMagMesh->SetCastShadow(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultRecFinder(TEXT("/Game/Weapons/Meshes/SM_W01_NF98_Receiver.SM_W01_NF98_Receiver"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultMagFinder(TEXT("/Game/Weapons/Meshes/SM_W01_NF98_Mag.SM_W01_NF98_Mag"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WeaponGunmetalFinder(TEXT("/Game/Weapons/Materials/MI_Weapon_Gunmetal.MI_Weapon_Gunmetal"));
	if (DefaultRecFinder.Succeeded())
	{
		TableRifleReceiverMesh->SetStaticMesh(DefaultRecFinder.Object);
	}
	if (DefaultMagFinder.Succeeded())
	{
		TableRifleMagMesh->SetStaticMesh(DefaultMagFinder.Object);
	}
	if (WeaponGunmetalFinder.Succeeded())
	{
		TableRifleReceiverMesh->SetMaterial(0, WeaponGunmetalFinder.Object);
		TableRifleMagMesh->SetMaterial(0, WeaponGunmetalFinder.Object);
	}

	TableWeaponKeyLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("TableWeaponKeyLight"));
	TableWeaponKeyLight->SetupAttachment(TableWeaponRoot);
	TableWeaponKeyLight->SetMobility(EComponentMobility::Movable);
	TableWeaponKeyLight->SetRelativeLocation(FVector(65.0f, -25.0f, 145.0f));
	TableWeaponKeyLight->SetRelativeRotation(FRotator(-42.0f, 165.0f, 0.0f));
	TableWeaponKeyLight->SetIntensity(1400.0f);
	TableWeaponKeyLight->SetLightColor(FLinearColor(0.85f, 0.94f, 1.0f));
	TableWeaponKeyLight->SetAttenuationRadius(320.0f);
	TableWeaponKeyLight->SetInnerConeAngle(22.0f);
	TableWeaponKeyLight->SetOuterConeAngle(48.0f);
	TableWeaponKeyLight->SetCastShadows(false);

	// Hand-held rifle meshes (shown when user clicks HIGH ADS in Operator Customization)
	HeldRifleReceiverMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeldRifleReceiverMesh"));
	HeldRifleReceiverMesh->SetupAttachment(TurntableRoot);
	HeldRifleReceiverMesh->SetMobility(EComponentMobility::Movable);
	HeldRifleReceiverMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HeldRifleReceiverMesh->SetCastShadow(true);
	HeldRifleReceiverMesh->SetVisibility(true);

	HeldRifleMagMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeldRifleMagMesh"));
	HeldRifleMagMesh->SetupAttachment(HeldRifleReceiverMesh);
	HeldRifleMagMesh->SetMobility(EComponentMobility::Movable);
	HeldRifleMagMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HeldRifleMagMesh->SetCastShadow(true);
	HeldRifleMagMesh->SetVisibility(true);

	if (DefaultRecFinder.Succeeded()) HeldRifleReceiverMesh->SetStaticMesh(DefaultRecFinder.Object);
	if (DefaultMagFinder.Succeeded()) HeldRifleMagMesh->SetStaticMesh(DefaultMagFinder.Object);
	if (WeaponGunmetalFinder.Succeeded())
	{
		HeldRifleReceiverMesh->SetMaterial(0, WeaponGunmetalFinder.Object);
		HeldRifleMagMesh->SetMaterial(0, WeaponGunmetalFinder.Object);
	}

	AK47Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AK47Mesh"));
	AK47Mesh->SetupAttachment(TurntableRoot);
	AK47Mesh->SetVisibility(false);

	// 7. Human Male Anatomical Skeleton (180 cm anthropometry fallback, kept hidden)
	SetupMeshComp(PelvisMesh, TEXT("PelvisMesh"), CubeMesh, FVector(0.0f, 0.0f, 96.0f), FRotator::ZeroRotator, FVector(0.24f, 0.32f, 0.16f));
	SetupMeshComp(SpineLowerMesh, TEXT("SpineLowerMesh"), CylinderMesh, FVector(0.0f, 0.0f, 110.0f), FRotator::ZeroRotator, FVector(0.22f, 0.26f, 0.14f));
	SetupMeshComp(SpineUpperMesh, TEXT("SpineUpperMesh"), CylinderMesh, FVector(0.0f, 0.0f, 124.0f), FRotator::ZeroRotator, FVector(0.24f, 0.28f, 0.14f));
	SetupMeshComp(ChestMesh, TEXT("ChestMesh"), CubeMesh, FVector(0.0f, 0.0f, 138.0f), FRotator::ZeroRotator, FVector(0.26f, 0.38f, 0.16f));
	SetupMeshComp(VestMesh, TEXT("VestMesh"), CubeMesh, FVector(1.0f, 0.0f, 132.0f), FRotator::ZeroRotator, FVector(0.30f, 0.36f, 0.26f));
	SetupMeshComp(NeckMesh, TEXT("NeckMesh"), CylinderMesh, FVector(0.0f, 0.0f, 148.0f), FRotator::ZeroRotator, FVector(0.12f, 0.12f, 0.10f));
	SetupMeshComp(HeadMesh, TEXT("HeadMesh"), SphereMesh, FVector(0.0f, 0.0f, 164.0f), FRotator::ZeroRotator, FVector(0.20f, 0.18f, 0.22f));

	SetupMeshComp(HelmetMesh, TEXT("HelmetMesh"), SphereMesh, FVector(0.0f, 0.0f, 167.0f), FRotator::ZeroRotator, FVector(0.23f, 0.21f, 0.20f));
	SetupMeshComp(NVGMesh, TEXT("NVGMesh"), CubeMesh, FVector(12.0f, 0.0f, 166.0f), FRotator::ZeroRotator, FVector(0.12f, 0.16f, 0.06f));
	SetupMeshComp(BalaclavaMesh, TEXT("BalaclavaMesh"), CylinderMesh, FVector(1.0f, 0.0f, 161.0f), FRotator::ZeroRotator, FVector(0.20f, 0.18f, 0.12f));
	SetupMeshComp(CapMesh, TEXT("CapMesh"), SphereMesh, FVector(0.0f, 0.0f, 168.0f), FRotator::ZeroRotator, FVector(0.21f, 0.20f, 0.12f));

	SetupMeshComp(RightClavicleMesh, TEXT("RightClavicleMesh"), CylinderMesh, FVector(-2.0f, 16.0f, 142.0f), FRotator(0.0f, 0.0f, 30.0f), FVector(0.10f, 0.10f, 0.12f));
	SetupMeshComp(RightUpperArmMesh, TEXT("RightUpperArmMesh"), CylinderMesh, FVector(6.0f, 20.0f, 136.0f), FRotator(-35.0f, 15.0f, 0.0f), FVector(0.12f, 0.12f, 0.28f));
	SetupMeshComp(RightForearmMesh, TEXT("RightForearmMesh"), CylinderMesh, FVector(26.0f, 16.0f, 137.0f), FRotator(15.0f, -25.0f, 0.0f), FVector(0.11f, 0.11f, 0.26f));
	SetupMeshComp(RightHandMesh, TEXT("RightHandMesh"), SphereMesh, FVector(44.0f, 11.0f, 137.0f), FRotator::ZeroRotator, FVector(0.09f, 0.08f, 0.09f));

	SetupMeshComp(LeftClavicleMesh, TEXT("LeftClavicleMesh"), CylinderMesh, FVector(-2.0f, -16.0f, 142.0f), FRotator(0.0f, 0.0f, -30.0f), FVector(0.10f, 0.10f, 0.12f));
	SetupMeshComp(LeftUpperArmMesh, TEXT("LeftUpperArmMesh"), CylinderMesh, FVector(14.0f, -14.0f, 134.0f), FRotator(-20.0f, -35.0f, 0.0f), FVector(0.12f, 0.12f, 0.28f));
	SetupMeshComp(LeftForearmMesh, TEXT("LeftForearmMesh"), CylinderMesh, FVector(40.0f, -4.0f, 138.0f), FRotator(15.0f, 45.0f, 0.0f), FVector(0.11f, 0.11f, 0.32f));
	SetupMeshComp(LeftHandMesh, TEXT("LeftHandMesh"), SphereMesh, FVector(62.0f, 5.0f, 140.0f), FRotator::ZeroRotator, FVector(0.09f, 0.08f, 0.09f));

	SetupMeshComp(LeftThighMesh, TEXT("LeftThighMesh"), CylinderMesh, FVector(0.0f, -11.0f, 74.0f), FRotator::ZeroRotator, FVector(0.16f, 0.16f, 0.42f));
	SetupMeshComp(LeftCalfMesh, TEXT("LeftCalfMesh"), CylinderMesh, FVector(0.0f, -11.0f, 32.0f), FRotator::ZeroRotator, FVector(0.14f, 0.14f, 0.42f));
	SetupMeshComp(LeftBootMesh, TEXT("LeftBootMesh"), CubeMesh, FVector(4.0f, -11.0f, 5.0f), FRotator::ZeroRotator, FVector(0.24f, 0.12f, 0.10f));

	SetupMeshComp(RightThighMesh, TEXT("RightThighMesh"), CylinderMesh, FVector(0.0f, 11.0f, 74.0f), FRotator::ZeroRotator, FVector(0.16f, 0.16f, 0.42f));
	SetupMeshComp(RightCalfMesh, TEXT("RightCalfMesh"), CylinderMesh, FVector(0.0f, 11.0f, 32.0f), FRotator::ZeroRotator, FVector(0.14f, 0.14f, 0.42f));
	SetupMeshComp(RightBootMesh, TEXT("RightBootMesh"), CubeMesh, FVector(4.0f, 11.0f, 5.0f), FRotator::ZeroRotator, FVector(0.24f, 0.12f, 0.10f));

	// 8. Tactical Weapon Subsystem (Fallback)
	WeaponRoot = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponRoot"));
	WeaponRoot->SetupAttachment(TurntableRoot);
	WeaponRoot->SetRelativeLocation(BaseWeaponLoc);
	WeaponRoot->SetRelativeRotation(BaseWeaponRot);

	auto SetupWeaponPart = [this, BaseMat](TObjectPtr<UStaticMeshComponent>& Comp, const TCHAR* Name, UStaticMesh* Mesh, const FVector& Loc, const FRotator& Rot, const FVector& Scale)
	{
		Comp = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Comp->SetupAttachment(WeaponRoot);
		if (Mesh) Comp->SetStaticMesh(Mesh);
		if (BaseMat) Comp->SetMaterial(0, BaseMat);
		Comp->SetRelativeLocation(Loc);
		Comp->SetRelativeRotation(Rot);
		Comp->SetRelativeScale3D(Scale);
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetCastShadow(true);
	};

	SetupWeaponPart(WeaponReceiverMesh, TEXT("WeaponReceiverMesh"), CubeMesh, FVector(0.0f, 0.0f, 0.0f), FRotator::ZeroRotator, FVector(0.26f, 0.06f, 0.09f));
	SetupWeaponPart(WeaponBarrelMesh, TEXT("WeaponBarrelMesh"), CylinderMesh, FVector(24.0f, 0.0f, 1.0f), FRotator(0.0f, 90.0f, 0.0f), FVector(0.03f, 0.03f, 0.36f));
	SetupWeaponPart(WeaponOpticMesh, TEXT("WeaponOpticMesh"), CubeMesh, FVector(-2.0f, 0.0f, 7.0f), FRotator::ZeroRotator, FVector(0.09f, 0.05f, 0.05f));
	SetupWeaponPart(WeaponMagMesh, TEXT("WeaponMagMesh"), CubeMesh, FVector(3.0f, 0.0f, -9.0f), FRotator(0.0f, 12.0f, 0.0f), FVector(0.08f, 0.04f, 0.18f));
	SetupWeaponPart(WeaponStockMesh, TEXT("WeaponStockMesh"), CubeMesh, FVector(-18.0f, 0.0f, -1.0f), FRotator::ZeroRotator, FVector(0.16f, 0.05f, 0.08f));

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(TurntableRoot);
	WeaponMesh->SetVisibility(false);
}

void AFPSCharacterPreviewActor::BeginPlay()
{
	Super::BeginPlay();

	// 1. Activate Live Skinned BodyMesh (`SK_BaseMale` driven by `UFPSAnimInstance`)
	bool bUsingLiveSkinnedBody = false;
	if (BodyMesh)
	{
		if (!BodyMesh->GetSkeletalMeshAsset())
		{
			if (USkeletalMesh* LoadedSkel = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/BaseMale/SK_BaseMale.SK_BaseMale")))
			{
				BodyMesh->SetSkeletalMesh(LoadedSkel);
			}
		}

		if (BodyMesh->GetSkeletalMeshAsset())
		{
			bUsingLiveSkinnedBody = true;
			BodyMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -2.4f));
			BodyMesh->SetRelativeRotation(FRotator(0.0f, 105.0f, 0.0f));
			BodyMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
			BodyMesh->SetVisibility(true, false);
			BodyMesh->SetHiddenInGame(false, false);

			if (UMaterialInterface* BodyMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_Operator_BaseBody.M_Operator_BaseBody")))
			{
				BodyMesh->SetMaterial(0, BodyMat);
			}
			if (UMaterialInterface* VisorMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_Soldier_Visor.M_Soldier_Visor")))
			{
				BodyMesh->SetMaterial(1, VisorMat);
			}

			BodyMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
			BodyMesh->SetAnimInstanceClass(UFPSAnimInstance::StaticClass());
			BodyMesh->InitAnim(true);

			// Initialize modular 3D belt gear on the Hangar operator (`BeltMagComp_1`, `BeltMagComp_2`, `BeltFragComp`, `BeltFlashComp`)
			if (BeltGearComponent)
			{
				BeltGearComponent->InitializeBeltKit(BodyMesh);
			}
			if (BreathVaporComponent)
			{
				BreathVaporComponent->InitializeBreathVapor(BodyMesh);
			}
		}
	}

	// 2. Fallback to static OperatorMesh only if SK_BaseMale is unavailable
	if (OperatorMesh)
	{
		if (!bUsingLiveSkinnedBody)
		{
			if (!OperatorMesh->GetStaticMesh())
			{
				if (UStaticMesh* LoadedPosed = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Characters/BaseMale/SM_Soldier_Posed.SM_Soldier_Posed")))
				{
					OperatorMesh->SetStaticMesh(LoadedPosed);
				}
			}
			OperatorMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
			OperatorMesh->SetRelativeRotation(FRotator(0.0f, 15.0f, 0.0f));
			OperatorMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
			OperatorMesh->SetVisibility(true, true);
			OperatorMesh->SetHiddenInGame(false, true);

			if (UMaterialInterface* BodyMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_Operator_BaseBody.M_Operator_BaseBody")))
				OperatorMesh->SetMaterial(0, BodyMat);
			if (UMaterialInterface* VisorMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_Soldier_Visor.M_Soldier_Visor")))
				OperatorMesh->SetMaterial(1, VisorMat);
		}
		else
		{
			OperatorMesh->SetVisibility(false, true);
			OperatorMesh->SetHiddenInGame(true, true);
		}
	}

	// 3. Ensure PedestalMesh uses low-profile SM_Hangar_Plinth at Z=-11cm (top deck flush at Z=0 under boots)
	if (PedestalMesh)
	{
		if (UStaticMesh* LoadedPlinth = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Environment/Hangar/SM_Hangar_Plinth.SM_Hangar_Plinth")))
		{
			PedestalMesh->SetStaticMesh(LoadedPlinth);
			PedestalMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -11.0f));
			PedestalMesh->SetRelativeRotation(FRotator::ZeroRotator);
			PedestalMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
		}
		else
		{
			PedestalMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -2.0f));
			PedestalMesh->SetRelativeRotation(FRotator::ZeroRotator);
			PedestalMesh->SetRelativeScale3D(FVector(1.4f, 1.4f, 0.04f));
		}
		PedestalMesh->SetVisibility(true, true);
		PedestalMesh->SetHiddenInGame(false, true);

		if (UMaterialInterface* RimSteel = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Environment/Hangar/Materials/M_Turntable_Steel.M_Turntable_Steel")))
			PedestalMesh->SetMaterial(0, RimSteel);
		if (UMaterialInterface* CyanNeon = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Environment/Hangar/Materials/M_Turntable_CyanNeon.M_Turntable_CyanNeon")))
			PedestalMesh->SetMaterial(1, CyanNeon);
		if (UMaterialInterface* GripDeck = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Environment/Hangar/Materials/M_Turntable_Deck.M_Turntable_Deck")))
			PedestalMesh->SetMaterial(2, GripDeck);
	}

	// Style Hangar Armory Table pedestal & stand
	if (TablePedestalMesh)
	{
		if (UMaterialInterface* RimSteel = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Environment/Hangar/Materials/M_Turntable_Steel.M_Turntable_Steel")))
		{
			TablePedestalMesh->SetMaterial(0, RimSteel);
		}
	}
	if (TableRackStandMesh)
	{
		if (UMaterialInterface* RimSteel = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Environment/Hangar/Materials/M_Turntable_Steel.M_Turntable_Steel")))
		{
			TableRackStandMesh->SetMaterial(0, RimSteel);
		}
	}

	// 4. Hide all legacy primitive boxes
	auto HideComp = [](UPrimitiveComponent* Comp)
	{
		if (Comp)
		{
			Comp->SetVisibility(false);
			Comp->SetHiddenInGame(true);
		}
	};
	HideComp(AK47Mesh);
	HideComp(PelvisMesh);
	HideComp(SpineLowerMesh);
	HideComp(SpineUpperMesh);
	HideComp(ChestMesh);
	HideComp(NeckMesh);
	HideComp(HeadMesh);
	HideComp(RightClavicleMesh);
	HideComp(RightUpperArmMesh);
	HideComp(RightForearmMesh);
	HideComp(RightHandMesh);
	HideComp(LeftClavicleMesh);
	HideComp(LeftUpperArmMesh);
	HideComp(LeftForearmMesh);
	HideComp(LeftHandMesh);
	HideComp(LeftThighMesh);
	HideComp(LeftCalfMesh);
	HideComp(RightThighMesh);
	HideComp(RightCalfMesh);
	HideComp(LeftBootMesh);
	HideComp(RightBootMesh);
	HideComp(VestMesh);
	HideComp(HelmetMesh);
	HideComp(NVGMesh);
	HideComp(BalaclavaMesh);
	HideComp(CapMesh);
	HideComp(WeaponReceiverMesh);
	HideComp(WeaponBarrelMesh);
	HideComp(WeaponOpticMesh);
	HideComp(WeaponMagMesh);
	HideComp(WeaponStockMesh);
	HideComp(TablePedestalMesh);
	HideComp(TableRackStandMesh);
	HideComp(TableRifleReceiverMesh);
	HideComp(TableRifleMagMesh);

	// 5. Sync persistent equipped loadout from UFPSEconomySubsystem if available
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
		{
			CurrentCamo = static_cast<EOperatorCamo>(FMath::Clamp(Economy->GetEquippedCamoIndex(), 0, 3));
			CurrentHeadgear = static_cast<EOperatorHeadgear>(FMath::Clamp(Economy->GetEquippedHeadgearIndex(), 0, 3));
			CurrentVest = static_cast<EOperatorArmorVest>(FMath::Clamp(Economy->GetEquippedArmorIndex(), 0, 2));
			CurrentWeaponFinish = static_cast<EOperatorWeaponFinish>(FMath::Clamp(Economy->GetEquippedWeaponFinishIndex(), 0, 3));
		}
	}

	int32 CmdStance = CurrentStanceIndex;
	if (FParse::Value(FCommandLine::Get(), TEXT("PreviewStance="), CmdStance))
	{
		CurrentStanceIndex = FMath::Clamp(CmdStance, 0, 2);
	}

	// Disable BreathVaporComponent in packaged builds where M_BreathVapor shader map is stripped
	if (BreathVaporComponent)
	{
		BreathVaporComponent->SetComponentTickEnabled(false);
	}
	TArray<UStaticMeshComponent*> AllStaticComps;
	GetComponents<UStaticMeshComponent>(AllStaticComps);
	for (UStaticMeshComponent* Comp : AllStaticComps)
	{
		if (Comp && Comp->GetName().Contains(TEXT("BreathPuff")))
		{
			Comp->SetVisibility(false, true);
			Comp->SetHiddenInGame(true, true);
		}
	}

	// 6. Build Authentic Royal Navy Blue 3D Belgian Duvel Beer Crate Stack on the Turntable Deck
	if (TurntableRoot && !FindComponentByTag<UProceduralMeshComponent>(FName(TEXT("DuvelCrateStack"))))
	{
		UProceduralMeshComponent* DuvelCrateComp = NewObject<UProceduralMeshComponent>(this, TEXT("DuvelCrateStackComp"));
		if (DuvelCrateComp)
		{
			DuvelCrateComp->ComponentTags.Add(FName(TEXT("DuvelCrateStack")));
			DuvelCrateComp->SetupAttachment(TurntableRoot);
			DuvelCrateComp->SetMobility(EComponentMobility::Movable);
			DuvelCrateComp->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
			DuvelCrateComp->SetRelativeRotation(FRotator(0.0f, 105.0f, 0.0f));
			DuvelCrateComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			DuvelCrateComp->SetCastShadow(true);
			DuvelCrateComp->RegisterComponent();

			FDuvelMeshSection SecNavyCrate, SecLabelPlaque, SecRedAccent, SecGlassBottle, SecCrownCap, SecWhiteBottleLabel;

			// Crate 1 (Bottom Seat Crate, Z = 0..25.5 cm)
			AddBelgianDuvelCrate(
				SecNavyCrate, SecLabelPlaque, SecRedAccent, SecGlassBottle, SecCrownCap, SecWhiteBottleLabel,
				FVector(0.0f, 11.5f, 0.0f), -8.0f, FVector(48.0f, 34.0f, 25.5f), false);

			// Crate 2 (Top Seat Crate, Z = 25.5..51.0 cm, with molded navy seat lid at Z = 51.0 cm supporting the seated operator)
			AddBelgianDuvelCrate(
				SecNavyCrate, SecLabelPlaque, SecRedAccent, SecGlassBottle, SecCrownCap, SecWhiteBottleLabel,
				FVector(0.0f, 11.5f, 25.5f), 5.0f, FVector(48.0f, 34.0f, 25.5f), false);
			{
				const FQuat TopSeatQ(FVector::UpVector, FMath::DegreesToRadians(5.0f));
				AddOrientedBox(
					SecNavyCrate,
					FVector(0.0f, 11.5f, 50.0f),
					TopSeatQ.RotateVector(FVector(1.0f, 0.0f, 0.0f)),
					TopSeatQ.RotateVector(FVector(0.0f, 1.0f, 0.0f)),
					FVector::UpVector,
					FVector(23.5f, 16.5f, 1.2f));
			}

			// Crate 3 (Side Display Duvel Crate beside his right leg at Screen-Left, showcasing the full Navy Blue ribbed shell,
			// red inner handle grips, "Duvel - Anno 1871" heraldic label plaque, and all 24 bottle crown caps peeking at the top rim!)
			AddBelgianDuvelCrate(
				SecNavyCrate, SecLabelPlaque, SecRedAccent, SecGlassBottle, SecCrownCap, SecWhiteBottleLabel,
				FVector(51.0f, -4.0f, 0.0f), -18.0f, FVector(43.0f, 31.0f, 24.0f), true);

			auto CommitSection = [&](int32 SecIdx, const FDuvelMeshSection& Sec)
			{
				if (Sec.Vertices.Num() > 0)
				{
					DuvelCrateComp->CreateMeshSection(SecIdx, Sec.Vertices, Sec.Triangles, Sec.Normals, Sec.UVs, Sec.Colors, Sec.Tangents, false);
				}
			};

			CommitSection(0, SecNavyCrate);
			CommitSection(1, SecLabelPlaque);
			CommitSection(2, SecRedAccent);
			CommitSection(3, SecGlassBottle);
			CommitSection(4, SecCrownCap);
			CommitSection(5, SecWhiteBottleLabel);

			// Create 16x16 white diffuse & flat normal textures
			UTexture2D* WhiteTex = UTexture2D::CreateTransient(16, 16, PF_B8G8R8A8);
			if (WhiteTex && WhiteTex->GetPlatformData() && WhiteTex->GetPlatformData()->Mips.Num() > 0)
			{
				WhiteTex->SRGB = true;
				uint8* MipData = static_cast<uint8*>(WhiteTex->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE));
				for (int32 i = 0; i < 16 * 16 * 4; ++i) MipData[i] = 255;
				WhiteTex->GetPlatformData()->Mips[0].BulkData.Unlock();
				WhiteTex->UpdateResource();
			}

			UTexture2D* FlatNorm = UTexture2D::CreateTransient(16, 16, PF_B8G8R8A8);
			if (FlatNorm && FlatNorm->GetPlatformData() && FlatNorm->GetPlatformData()->Mips.Num() > 0)
			{
				FlatNorm->SRGB = false;
				FlatNorm->CompressionSettings = TC_Normalmap;
				uint8* MipData = static_cast<uint8*>(FlatNorm->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE));
				for (int32 i = 0; i < 16 * 16; ++i)
				{
					MipData[i * 4 + 0] = 255;
					MipData[i * 4 + 1] = 128;
					MipData[i * 4 + 2] = 128;
					MipData[i * 4 + 3] = 255;
				}
				FlatNorm->GetPlatformData()->Mips[0].BulkData.Unlock();
				FlatNorm->UpdateResource();
			}

			// Load the 1024x512 rectified Duvel label plaque texture (T_Duvel_Label_1024x512.bgra)
			static UTexture2D* CachedDuvelLabelTex = nullptr;
			if (!CachedDuvelLabelTex || !CachedDuvelLabelTex->IsValidLowLevel())
			{
				const FString FileName(TEXT("T_Duvel_Label_1024x512.bgra"));
				const FString Candidates[] = {
					FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Characters/BaseMale/Textures"), FileName),
					FPaths::Combine(FPaths::LaunchDir(), TEXT("../../../NobleFermi/Content/Characters/BaseMale/Textures"), FileName),
					FPaths::Combine(FPaths::LaunchDir(), TEXT("Content/Characters/BaseMale/Textures"), FileName),
					FString(TEXT("C:/Users/Boris/Documents/antigravity/noble-fermi/Content/Characters/BaseMale/Textures/")) + FileName
				};
				TArray<uint8> RawBytes;
				for (const FString& Path : Candidates)
				{
					if (FPaths::FileExists(Path) && FFileHelper::LoadFileToArray(RawBytes, *Path) && RawBytes.Num() == 1024 * 512 * 4)
					{
						UTexture2D* Tex = UTexture2D::CreateTransient(1024, 512, PF_B8G8R8A8);
						if (Tex && Tex->GetPlatformData() && Tex->GetPlatformData()->Mips.Num() > 0)
						{
							Tex->SRGB = true;
							Tex->CompressionSettings = TC_Default;
							Tex->Filter = TF_Trilinear;
							Tex->NeverStream = true;
							void* MipData = Tex->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
							FMemory::Memcpy(MipData, RawBytes.GetData(), RawBytes.Num());
							Tex->GetPlatformData()->Mips[0].BulkData.Unlock();
							Tex->UpdateResource();
							Tex->AddToRoot();
							CachedDuvelLabelTex = Tex;
							break;
						}
					}
				}
			}

			UMaterialInterface* BasePBR = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_Belt_Gear_PBR.M_Belt_Gear_PBR"));
			if (!BasePBR)
			{
				BasePBR = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
			}

			UMaterialInterface* TexturedMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_Operator_BaseBody.M_Operator_BaseBody"));
			if (!TexturedMat && BodyMesh && BodyMesh->GetMaterial(0))
			{
				TexturedMat = BodyMesh->GetMaterial(0);
			}
			if (!TexturedMat)
			{
				TexturedMat = BasePBR;
			}

			auto ApplyDuvelMat = [&](int32 SecIdx, const FLinearColor& Col, float Roughness, float Metallic)
			{
				if (!BasePBR) return;
				UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BasePBR, this);
				if (!MID) return;
				if (WhiteTex)
				{
					MID->SetTextureParameterValue(TEXT("Texture_Diffuse"), WhiteTex);
					MID->SetTextureParameterValue(TEXT("BaseColorTexture"), WhiteTex);
				}
				if (FlatNorm)
				{
					MID->SetTextureParameterValue(TEXT("Texture_Normal"), FlatNorm);
				}
				MID->SetVectorParameterValue(TEXT("Color_Tint"), Col);
				MID->SetVectorParameterValue(TEXT("Tint"), Col);
				MID->SetVectorParameterValue(TEXT("BaseColor"), Col);
				MID->SetVectorParameterValue(TEXT("Color"), Col);
				MID->SetVectorParameterValue(TEXT("CamoColorPrimary"), Col);
				MID->SetScalarParameterValue(TEXT("Scalar_Roughness"), Roughness);
				MID->SetScalarParameterValue(TEXT("Roughness"), Roughness);
				MID->SetScalarParameterValue(TEXT("RoughnessBase"), Roughness);
				MID->SetScalarParameterValue(TEXT("Scalar_Metallic"), Metallic);
				MID->SetScalarParameterValue(TEXT("Metallic"), Metallic);
				DuvelCrateComp->SetMaterial(SecIdx, MID);
			};

			// Section 0: Authentic Duvel Deep Royal Navy Blue HDPE Crate Plastic (#1D2B64)
			ApplyDuvelMat(0, FLinearColor(0.015f, 0.032f, 0.145f, 1.0f), 0.34f, 0.05f);
			// Section 1: Crisp Off-White Plaque Background
			ApplyDuvelMat(1, FLinearColor(0.94f, 0.94f, 0.91f, 1.0f), 0.42f, 0.02f);
			// Section 2: Duvel Gothic Calligraphy Crimson Red & Inner Ergonomic Handle Grips (#C4161C)
			ApplyDuvelMat(2, FLinearColor(0.74f, 0.020f, 0.022f, 1.0f), 0.34f, 0.06f);
			// Section 3: Dark Brown Stubby Glass Bottles
			ApplyDuvelMat(3, FLinearColor(0.08f, 0.028f, 0.008f, 1.0f), 0.14f, 0.15f);
			// Section 4: Gold Border Frame, "Anno 1871", Heraldic Coat of Arms & Crimped Metallic Crown Caps
			ApplyDuvelMat(4, FLinearColor(0.82f, 0.60f, 0.14f, 1.0f), 0.24f, 0.80f);
			// Section 5: Deep Ink Black Calligraphic Drop-Shadow & Heraldic Crest Shield
			ApplyDuvelMat(5, FLinearColor(0.015f, 0.016f, 0.020f, 1.0f), 0.45f, 0.04f);
		}
	}

	SetEquippedWeaponArchetype(CurrentWeaponArchetypeIndex);
	ApplyCamoColors();
	ApplyHeadgearVisibility();
	ApplyVestConfiguration();
	ApplyWeaponFinish();
	SetPreviewStance(CurrentStanceIndex);
}

void AFPSCharacterPreviewActor::CalcCamera(float DeltaTime, struct FMinimalViewInfo& OutResult)
{
	if (PreviewCamera)
	{
		PreviewCamera->GetCameraView(DeltaTime, OutResult);

		// Subtle splash-to-menu camera settle that ALWAYS keeps the operator's planted boots and turntable deck 100% in frame
		if (CurrentStanceIndex == 0 && IntroTransitionTimer < 2.4f)
		{
			const float Alpha = FMath::SmoothStep(0.0f, 1.0f, FMath::Clamp(IntroTransitionTimer / 2.4f, 0.0f, 1.0f));
			const FVector StartLoc = GetActorTransform().TransformPosition(FVector(412.0f, -4.0f, 97.0f));
			OutResult.Location = FMath::Lerp(StartLoc, OutResult.Location, Alpha);
			OutResult.FOV = FMath::Lerp(50.5f, OutResult.FOV, Alpha);
		}
	}
	else
	{
		Super::CalcCamera(DeltaTime, OutResult);
	}
}

void AFPSCharacterPreviewActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Detect when the game window first becomes visible/active after the splash & loading screen
	if (!bHasTriggeredSplashTransition)
	{
		bool bWindowReady = FParse::Param(FCommandLine::Get(), TEXT("TakeMenuShot"));
		if (!bWindowReady && FSlateApplication::IsInitialized())
		{
			TSharedPtr<SWindow> TopWin = FSlateApplication::Get().GetActiveTopLevelWindow();
			if (TopWin.IsValid() && TopWin->IsVisible())
			{
				bWindowReady = true;
			}
		}

		if (bWindowReady || AimBreathTimer > 3.5f)
		{
			bHasTriggeredSplashTransition = true;
			IntroTransitionTimer = 0.0f;
			if (BodyMesh)
			{
				if (UFPSAnimInstance* FPSAnim = Cast<UFPSAnimInstance>(BodyMesh->GetAnimInstance()))
				{
					FPSAnim->RestartWeaponInspectionCycle();
				}
			}
		}
	}
	IntroTransitionTimer += DeltaTime;

	// 1. Turntable 360-degree continuous rotation
	if (bIsRotating && TurntableRoot)
	{
		TurntableRoot->AddLocalRotation(FRotator(0.0f, RotationSpeed * DeltaTime, 0.0f));
	}

	// 2. Pin HeldRifleReceiverMesh & HeldRifleMagMesh to operator's hands across all stances (Low-Ready, Mag-Check/Barrel-Wipe, High-Ready ADS, Patrol Walk)
	if (BodyMesh && HeldRifleReceiverMesh && HeldRifleReceiverMesh->IsVisible())
	{
		const FVector HandRPos = BodyMesh->GetBoneLocation(FName(TEXT("hand_r")), EBoneSpaces::WorldSpace);
		const FVector HandLPos = BodyMesh->GetBoneLocation(FName(TEXT("hand_l")), EBoneSpaces::WorldSpace);
		const float HandDist = FVector::Dist(HandRPos, HandLPos);

		UFPSAnimInstance* FPSAnim = Cast<UFPSAnimInstance>(BodyMesh->GetAnimInstance());
		const float HighReadyA = FPSAnim ? FMath::Clamp(FPSAnim->HighReadyAlpha, 0.0f, 1.0f) : 0.0f;
		const bool bInspecting = (FPSAnim && FPSAnim->bIsInspectingWeapon && HighReadyA < 0.25f);
		const float W = bInspecting ? FMath::Clamp(FPSAnim->WeaponInspectionAlpha, 0.0f, 1.0f) : 0.0f;

		// Pin HeldRifleReceiverMesh securely between operator's hands across all stances
		const FTransform MeshTransform = BodyMesh->GetComponentTransform();

		const FVector UpVec = FVector::UpVector;
		const FVector CompUpDir = FPSAnim ? FPSAnim->GetCompUpDir() : FVector::UpVector;
		const FVector CompForwardDir = FPSAnim ? FPSAnim->GetCompForwardDir() : FVector(0.0f, -1.0f, 0.0f);
		const FVector CompRightDir = FPSAnim ? FPSAnim->GetCompRightDir() : FVector(1.0f, 0.0f, 0.0f);
		const FVector LowReadyDirCS = (-CompRightDir * 0.58f + CompForwardDir * 0.76f - CompUpDir * 0.28f).GetSafeNormal();

		const float PitchRad = FMath::DegreesToRadians(FMath::Clamp(FPSAnim ? FPSAnim->AimPitch : 0.0f, -65.0f, 65.0f));
		const FVector HighReadyDirCS = (
			-CompRightDir * (0.18f * (1.0f - (FPSAnim ? FPSAnim->SmoothADSAlpha : 0.0f)))
			+ CompForwardDir * FMath::Cos(PitchRad)
			+ CompUpDir * FMath::Sin(PitchRad)
		).GetSafeNormal();

		const FVector TargetDirCS = FMath::Lerp(LowReadyDirCS, HighReadyDirCS, HighReadyA).GetSafeNormal();
		const FVector BarrelDir = MeshTransform.GetRotation().RotateVector(TargetDirCS);

		FVector RightVec = FVector::CrossProduct(UpVec, BarrelDir).GetSafeNormal();
		if (RightVec.ContainsNaN() || RightVec.IsNearlyZero(1e-3f))
		{
			RightVec = BodyMesh->GetComponentQuat().GetRightVector();
		}
		const FVector OrthoUp = FVector::CrossProduct(BarrelDir, RightVec).GetSafeNormal();
		FQuat RifleQuat = FRotationMatrix::MakeFromXZ(BarrelDir, OrthoUp).ToQuat();

		if (bInspecting)
		{
			// Cant rifle slightly outward during inspection so the operator can view the ejection port / magwell
			float CantRollBlend = 0.0f;
			if (W < 0.10f) CantRollBlend = FMath::SmoothStep(0.0f, 1.0f, W / 0.10f);
			else if (W < 0.46f) CantRollBlend = 1.0f;
			else if (W < 0.56f) CantRollBlend = 1.0f - FMath::SmoothStep(0.0f, 1.0f, (W - 0.46f) / 0.10f);
			if (CantRollBlend > 0.01f)
			{
				const FQuat CantRollQuat(BarrelDir, FMath::DegreesToRadians(-22.0f * CantRollBlend));
				RifleQuat = (CantRollQuat * RifleQuat).GetNormalized();
			}
		}

		// Offset so right hand wrist/palm sits firmly against the right flank of the pistol grip
		const FVector GripOffset(10.56f, -2.2f, 7.04f);
		const FVector ReceiverWorldPos = HandRPos + RifleQuat.RotateVector(GripOffset);

		if (!RifleQuat.ContainsNaN() && !ReceiverWorldPos.ContainsNaN())
		{
			HeldRifleReceiverMesh->SetWorldLocationAndRotation(ReceiverWorldPos, RifleQuat);
			// Animate HeldRifleMagMesh into LeftPalmCenterWorld during magazine inspection (W in [0.11, 0.41])
			float MagHandAlpha = 0.0f;
			if (bInspecting && W >= 0.11f && W <= 0.41f)
			{
				if (W < 0.14f)
				{
					MagHandAlpha = FMath::SmoothStep(0.0f, 1.0f, (W - 0.11f) / 0.03f);
				}
				else if (W < 0.37f)
				{
					MagHandAlpha = 1.0f;
				}
				else
				{
					MagHandAlpha = 1.0f - FMath::SmoothStep(0.0f, 1.0f, (W - 0.37f) / 0.04f);
				}
			}

			if (BeltGearComponent)
			{
				// Keep BeltGearComponent reload grab disabled in Hangar preview so HeldRifleMagMesh remains the sole inspected magazine
				BeltGearComponent->UpdateReloadBeltGrabState(false, 0.0f, true);
			}

			if (HeldRifleMagMesh)
			{
				const FVector MagOffsets[5] = {
					FVector(1.5f, 0.0f, -3.5f),
					FVector(4.5f, 0.0f, -1.2f),
					FVector(7.5f, 0.0f, -1.8f),
					FVector(6.5f, 0.0f, -1.6f),
					FVector(5.0f, 0.0f, -1.4f)
				};
				const FVector SeatedLocalOffset = MagOffsets[FMath::Clamp(CurrentWeaponArchetypeIndex, 0, 4)];

				float MagSlideDown = 0.0f;
				if (bInspecting && W >= 0.12f && W <= 0.44f)
				{
					if (W < 0.22f)
					{
						MagSlideDown = FMath::SmoothStep(0.0f, 1.0f, (W - 0.12f) / 0.10f) * 4.5f;
					}
					else if (W < 0.35f)
					{
						MagSlideDown = 4.5f;
					}
					else if (W < 0.41f)
					{
						MagSlideDown = (1.0f - FMath::SmoothStep(0.0f, 1.0f, (W - 0.35f) / 0.06f)) * 4.5f;
					}
				}

				const FVector InspectedLocalOffset = SeatedLocalOffset - FVector(0.0f, 0.0f, MagSlideDown);
				HeldRifleMagMesh->SetRelativeLocationAndRotation(InspectedLocalOffset, FRotator::ZeroRotator);
			}
		}
	}

	UpdateAimingPose(DeltaTime);
}

void AFPSCharacterPreviewActor::UpdateAimingPose(float DeltaTime)
{
	AimBreathTimer += DeltaTime;
}

void AFPSCharacterPreviewActor::SetEquippedWeaponArchetype(int32 ArchetypeIndex)
{
	CurrentWeaponArchetypeIndex = FMath::Clamp(ArchetypeIndex, 0, 4);

	const TCHAR* ReceiverPaths[5] = {
		TEXT("/Game/Weapons/Meshes/SM_W01_NF98_Receiver.SM_W01_NF98_Receiver"),
		TEXT("/Game/Weapons/Meshes/SM_W06_Spectre45_Receiver.SM_W06_Spectre45_Receiver"),
		TEXT("/Game/Weapons/Meshes/SM_W02_VK74_Receiver.SM_W02_VK74_Receiver"),
		TEXT("/Game/Weapons/Meshes/SM_W03_Falcon50_Receiver.SM_W03_Falcon50_Receiver"),
		TEXT("/Game/Weapons/Meshes/SM_W04_Breacher12_Receiver.SM_W04_Breacher12_Receiver")
	};

	const TCHAR* MagPaths[5] = {
		TEXT("/Game/Weapons/Meshes/SM_W01_NF98_Mag.SM_W01_NF98_Mag"),
		TEXT("/Game/Weapons/Meshes/SM_W06_Spectre45_Mag.SM_W06_Spectre45_Mag"),
		TEXT("/Game/Weapons/Meshes/SM_W02_VK74_Mag.SM_W02_VK74_Mag"),
		TEXT("/Game/Weapons/Meshes/SM_W03_Falcon50_Mag.SM_W03_Falcon50_Mag"),
		TEXT("/Game/Weapons/Meshes/SM_W04_Breacher12_Mag.SM_W04_Breacher12_Mag")
	};

	const FVector MagOffsets[5] = {
		FVector(1.5f, 0.0f, -3.5f),
		FVector(4.5f, 0.0f, -1.2f),
		FVector(7.5f, 0.0f, -1.8f),
		FVector(6.5f, 0.0f, -1.6f),
		FVector(5.0f, 0.0f, -1.4f)
	};

	UStaticMesh* RecMesh = LoadObject<UStaticMesh>(nullptr, ReceiverPaths[CurrentWeaponArchetypeIndex]);
	UStaticMesh* MagMesh = LoadObject<UStaticMesh>(nullptr, MagPaths[CurrentWeaponArchetypeIndex]);

	if (TableRifleReceiverMesh && RecMesh)
	{
		TableRifleReceiverMesh->SetStaticMesh(RecMesh);
	}
	if (TableRifleMagMesh && MagMesh)
	{
		TableRifleMagMesh->SetStaticMesh(MagMesh);
		TableRifleMagMesh->SetRelativeLocation(MagOffsets[CurrentWeaponArchetypeIndex]);
	}
	if (HeldRifleReceiverMesh && RecMesh)
	{
		HeldRifleReceiverMesh->SetStaticMesh(RecMesh);
	}
	if (HeldRifleMagMesh && MagMesh)
	{
		HeldRifleMagMesh->SetStaticMesh(MagMesh);
		HeldRifleMagMesh->SetRelativeLocation(MagOffsets[CurrentWeaponArchetypeIndex]);
	}

	ApplyWeaponFinish();
}

void AFPSCharacterPreviewActor::SetCamo(EOperatorCamo NewCamo)
{
	CurrentCamo = NewCamo;
	ApplyCamoColors();
}

void AFPSCharacterPreviewActor::SetHeadgear(EOperatorHeadgear NewHeadgear)
{
	CurrentHeadgear = NewHeadgear;
	ApplyHeadgearVisibility();
}

void AFPSCharacterPreviewActor::SetArmorVest(EOperatorArmorVest NewVest)
{
	CurrentVest = NewVest;
	ApplyVestConfiguration();
}

void AFPSCharacterPreviewActor::SetWeaponFinish(EOperatorWeaponFinish NewFinish)
{
	CurrentWeaponFinish = NewFinish;
	ApplyWeaponFinish();
}

void AFPSCharacterPreviewActor::SetPreviewStance(int32 StanceIndex)
{
	// 0 = Grounded Battle-Ready Low-Ready + Mag/Barrel Inspection, 1 = High-Ready ADS, 2 = Patrol Walk
	CurrentStanceIndex = FMath::Clamp(StanceIndex, 0, 2);
	bIsAiming = (CurrentStanceIndex == 1);

	if (BodyMesh)
	{
		if (UFPSAnimInstance* FPSAnim = Cast<UFPSAnimInstance>(BodyMesh->GetAnimInstance()))
		{
			FPSAnim->bIsMenuPreview = true;
			FPSAnim->MenuPoseIndex = CurrentStanceIndex;
			FPSAnim->bUnarmedRelaxedIdle = false;
			FPSAnim->bForceHighReady = (CurrentStanceIndex == 1);
			FPSAnim->bIsADS = (CurrentStanceIndex == 1);
			if (CurrentStanceIndex == 0)
			{
				FPSAnim->RestartWeaponInspectionCycle();
			}
		}
	}

	// Always equip the primary rifle & magazine in the operator's hands across all Hangar stances
	if (HeldRifleReceiverMesh)
	{
		HeldRifleReceiverMesh->SetVisibility(true, true);
		HeldRifleReceiverMesh->SetHiddenInGame(false, true);
	}
	if (HeldRifleMagMesh)
	{
		HeldRifleMagMesh->SetVisibility(true, true);
		HeldRifleMagMesh->SetHiddenInGame(false, true);
	}
	if (TableRifleReceiverMesh)
	{
		TableRifleReceiverMesh->SetVisibility(false, true);
		TableRifleReceiverMesh->SetHiddenInGame(true, true);
	}
	if (UProceduralMeshComponent* DuvelCrateComp = FindComponentByTag<UProceduralMeshComponent>(FName(TEXT("DuvelCrateStack"))))
	{
		const bool bShowCrates = (CurrentStanceIndex != 2);
		DuvelCrateComp->SetVisibility(bShowCrates, true);
		DuvelCrateComp->SetHiddenInGame(!bShowCrates, true);
	}
}

void AFPSCharacterPreviewActor::SetAimingStance(bool bInAiming)
{
	SetPreviewStance((CurrentStanceIndex + 1) % 3);
}

void AFPSCharacterPreviewActor::ResetFacing()
{
	if (TurntableRoot)
	{
		TurntableRoot->SetRelativeRotation(FRotator::ZeroRotator);
	}
	if (BodyMesh)
	{
		if (UFPSAnimInstance* FPSAnim = Cast<UFPSAnimInstance>(BodyMesh->GetAnimInstance()))
		{
			FPSAnim->RestartWeaponInspectionCycle();
		}
	}
}

void AFPSCharacterPreviewActor::ApplyCamoColors()
{
	// Balanced natural tactical tints for Vanguard Spec-Ops Soldier (no overexposure)
	FLinearColor CamoTint = FLinearColor(0.55f, 0.58f, 0.64f, 1.0f); // UrbanDigital (Task Force Echo Slate)
	if (CurrentCamo == EOperatorCamo::WoodlandTactical)
	{
		CamoTint = FLinearColor(0.46f, 0.58f, 0.42f, 1.0f); // Tactical Woodland Olive
	}
	else if (CurrentCamo == EOperatorCamo::DesertSpecOps)
	{
		CamoTint = FLinearColor(0.74f, 0.66f, 0.52f, 1.0f); // Desert Sand SpecOps
	}
	else if (CurrentCamo == EOperatorCamo::StealthBlackout)
	{
		CamoTint = FLinearColor(0.24f, 0.26f, 0.30f, 1.0f); // Stealth Blackout Dark Obsidian
	}

	// Load 2048x2048 Ultra-Sharp Crye-G3 Tactical Ripstop & BDU Camo Diffuse + Normal Maps (with NeverStream=true!)
	static UTexture2D* CachedHDDiffuse = nullptr;
	static UTexture2D* CachedHDNormal = nullptr;
	if (!CachedHDDiffuse || !CachedHDDiffuse->IsValidLowLevel())
	{
		auto LoadRaw2048Tex = [](const FString& FileName, bool bSRGB, TextureCompressionSettings CompSettings) -> UTexture2D*
		{
			const FString Candidates[] = {
				FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Characters/BaseMale/Textures"), FileName),
				FPaths::Combine(FPaths::LaunchDir(), TEXT("../../../NobleFermi/Content/Characters/BaseMale/Textures"), FileName),
				FPaths::Combine(FPaths::LaunchDir(), TEXT("Content/Characters/BaseMale/Textures"), FileName),
				FString(TEXT("C:/Users/Boris/Documents/antigravity/noble-fermi/Content/Characters/BaseMale/Textures/")) + FileName
			};
			TArray<uint8> RawBytes;
			for (const FString& Path : Candidates)
			{
				if (FPaths::FileExists(Path) && FFileHelper::LoadFileToArray(RawBytes, *Path) && RawBytes.Num() == 2048 * 2048 * 4)
				{
					UTexture2D* Tex = UTexture2D::CreateTransient(2048, 2048, PF_B8G8R8A8);
					if (Tex && Tex->GetPlatformData() && Tex->GetPlatformData()->Mips.Num() > 0)
					{
						Tex->SRGB = bSRGB;
						Tex->CompressionSettings = CompSettings;
						Tex->Filter = TF_Trilinear;
						Tex->NeverStream = true;
						void* MipData = Tex->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
						FMemory::Memcpy(MipData, RawBytes.GetData(), RawBytes.Num());
						Tex->GetPlatformData()->Mips[0].BulkData.Unlock();
						Tex->UpdateResource();
						Tex->AddToRoot();
						return Tex;
					}
				}
			}
			return nullptr;
		};
		CachedHDDiffuse = LoadRaw2048Tex(TEXT("T_Soldier_Diffuse_2048.bgra"), true, TC_Default);
		CachedHDNormal  = LoadRaw2048Tex(TEXT("T_Soldier_Normal_2048.bgra"), false, TC_Normalmap);
	}

	auto ConfigureMeshMaterials = [&](UMeshComponent* MeshComp)
	{
		if (!MeshComp) return;

		UMaterialInstanceDynamic* BodyMID = Cast<UMaterialInstanceDynamic>(MeshComp->GetMaterial(0));
		if (!BodyMID)
		{
			BodyMID = MeshComp->CreateDynamicMaterialInstance(0);
		}
		if (BodyMID)
		{
			if (CachedHDDiffuse)
			{
				BodyMID->SetTextureParameterValue(TEXT("Texture_Diffuse"), CachedHDDiffuse);
			}
			if (CachedHDNormal)
			{
				BodyMID->SetTextureParameterValue(TEXT("Texture_Normal"), CachedHDNormal);
			}
			BodyMID->SetVectorParameterValue(TEXT("CamoColorPrimary"), CamoTint);
			BodyMID->SetVectorParameterValue(TEXT("BaseColor"), CamoTint);
			BodyMID->SetScalarParameterValue(TEXT("AmbientFillScale"), 0.0f);
			BodyMID->SetScalarParameterValue(TEXT("RoughnessBase"), 0.62f);
		}

		UMaterialInstanceDynamic* VisorMID = Cast<UMaterialInstanceDynamic>(MeshComp->GetMaterial(1));
		if (!VisorMID)
		{
			VisorMID = MeshComp->CreateDynamicMaterialInstance(1);
		}
		if (VisorMID)
		{
			VisorMID->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.65f, 0.75f, 0.85f, 1.0f));
			VisorMID->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor::Black);
			VisorMID->SetScalarParameterValue(TEXT("EmissiveIntensity"), 0.0f);
		}
	};

	ConfigureMeshMaterials(BodyMesh);
	ConfigureMeshMaterials(OperatorMesh);
}

void AFPSCharacterPreviewActor::ApplyHeadgearVisibility()
{
	// Integrated into master soldier mesh
}

void AFPSCharacterPreviewActor::ApplyVestConfiguration()
{
	// Integrated into master soldier mesh
}

void AFPSCharacterPreviewActor::ApplyWeaponFinish()
{
	FLinearColor FinishColor(0.07f, 0.08f, 0.10f, 1.0f); // Dark Tactical Blued Gunmetal Steel
	float FinishRoughness = 0.36f;
	float FinishMetallic = 0.85f;

	if (CurrentWeaponFinish == EOperatorWeaponFinish::DesertTanCerakote)
	{
		FinishColor = FLinearColor(0.55f, 0.45f, 0.32f, 1.0f);
		FinishRoughness = 0.50f;
		FinishMetallic = 0.25f;
	}
	else if (CurrentWeaponFinish == EOperatorWeaponFinish::CarbonFiberSpecOps)
	{
		FinishColor = FLinearColor(0.03f, 0.035f, 0.045f, 1.0f);
		FinishRoughness = 0.28f;
		FinishMetallic = 0.55f;
	}
	else if (CurrentWeaponFinish == EOperatorWeaponFinish::GoldOpalDamascene)
	{
		FinishColor = FLinearColor(0.85f, 0.64f, 0.18f, 1.0f);
		FinishRoughness = 0.18f;
		FinishMetallic = 0.94f;
	}

	auto ApplyToWeaponMesh = [&](UStaticMeshComponent* WeapComp, const FLinearColor& ColorOverride)
	{
		if (!WeapComp) return;
		UMaterialInterface* WeaponMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_Belt_Gear_PBR.M_Belt_Gear_PBR"));
		if (!WeaponMat)
		{
			WeaponMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_Operator_BaseBody.M_Operator_BaseBody"));
		}
		const int32 NumMats = FMath::Max(1, WeapComp->GetNumMaterials());
		for (int32 i = 0; i < NumMats; ++i)
		{
			UMaterialInstanceDynamic* MID = WeapComp->CreateDynamicMaterialInstance(i, WeaponMat ? WeaponMat : WeapComp->GetMaterial(i));
			if (MID)
			{
				MID->SetVectorParameterValue(TEXT("CamoColorPrimary"), ColorOverride);
				MID->SetVectorParameterValue(TEXT("BaseColor"), ColorOverride);
				MID->SetVectorParameterValue(TEXT("Color"), ColorOverride);
				MID->SetScalarParameterValue(TEXT("AmbientFillScale"), 0.0f);
				MID->SetScalarParameterValue(TEXT("RoughnessBase"), FinishRoughness);
				MID->SetScalarParameterValue(TEXT("Roughness"), FinishRoughness);
				MID->SetScalarParameterValue(TEXT("Metallic"), FinishMetallic);
			}
		}
	};

	const FLinearColor MagColor = (CurrentWeaponFinish == EOperatorWeaponFinish::DesertTanCerakote)
		? FLinearColor(0.36f, 0.29f, 0.20f, 1.0f)
		: FLinearColor(0.14f, 0.15f, 0.18f, 1.0f); // High-contrast stamped parkerized tactical steel

	ApplyToWeaponMesh(TableRifleReceiverMesh, FinishColor);
	ApplyToWeaponMesh(TableRifleMagMesh, MagColor);
	ApplyToWeaponMesh(HeldRifleReceiverMesh, FinishColor);
	ApplyToWeaponMesh(HeldRifleMagMesh, MagColor);
}
