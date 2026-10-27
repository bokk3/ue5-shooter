// =========================================================================================
// NOBLE FERMI // PROCEDURAL ANTI-TILING COASTAL PBR MATERIAL & TEXTURE LIBRARY
// =========================================================================================

#include "Map/FPSCoastalMaterialLibrary.h"
#include "Materials/Material.h"
#include "UObject/Package.h"

static TMap<uint32, TObjectPtr<UMaterialInterface>> GCoastalMaterialCache;
static TMap<uint32, TObjectPtr<UTexture2D>> GCoastalBaseColorCache;
static TMap<uint32, TObjectPtr<UTexture2D>> GCoastalNormalCache;
static TMap<uint8, TObjectPtr<UTexture2D>> GCoastalRoughnessCache;

static uint32 MakeCacheKey(ECoastalSurfaceType SurfaceType, FVector2D WindDirection)
{
	const int32 Angle = FMath::RoundToInt(FMath::Atan2(WindDirection.Y, WindDirection.X) * (180.0f / UE_PI));
	return (static_cast<uint32>(SurfaceType) << 16) | (static_cast<uint32>(Angle + 360) & 0xFFFF);
}

float UFPSCoastalMaterialLibrary::SeamlessNoise(float U, float V, float ScaleX, float ScaleY)
{
	U = FMath::Fmod(FMath::Fmod(U, 1.0f) + 1.0f, 1.0f);
	V = FMath::Fmod(FMath::Fmod(V, 1.0f) + 1.0f, 1.0f);

	const float X = U * ScaleX;
	const float Y = V * ScaleY;

	const int32 ModX = FMath::Max(1, FMath::RoundToInt(ScaleX));
	const int32 ModY = FMath::Max(1, FMath::RoundToInt(ScaleY));

	int32 X0 = FMath::FloorToInt(X);
	int32 Y0 = FMath::FloorToInt(Y);
	const int32 X1 = (X0 + 1) % ModX;
	const int32 Y1 = (Y0 + 1) % ModY;
	X0 = (X0 % ModX + ModX) % ModX;
	Y0 = (Y0 % ModY + ModY) % ModY;

	const float Fx = X - FMath::FloorToFloat(X);
	const float Fy = Y - FMath::FloorToFloat(Y);

	const float Sx = Fx * Fx * Fx * (Fx * (Fx * 6.0f - 15.0f) + 10.0f);
	const float Sy = Fy * Fy * Fy * (Fy * (Fy * 6.0f - 15.0f) + 10.0f);

	auto PseudoHash = [](int32 Ix, int32 Iy) -> float
	{
		uint32 H = static_cast<uint32>(Ix * 374761393 + Iy * 668265263);
		H = (H ^ (H >> 13)) * 1274126177;
		return static_cast<float>(H & 0x7FFFFFFF) / 2147483647.0f;
	};

	const float V00 = PseudoHash(X0, Y0);
	const float V10 = PseudoHash(X1, Y0);
	const float V01 = PseudoHash(X0, Y1);
	const float V11 = PseudoHash(X1, Y1);

	const float V0 = FMath::Lerp(V00, V10, Sx);
	const float V1 = FMath::Lerp(V01, V11, Sx);
	return FMath::Lerp(V0, V1, Sy);
}

float UFPSCoastalMaterialLibrary::VoronoiCell(float U, float V, int32 GridCells, FVector& OutCellNormal)
{
	U = FMath::Fmod(FMath::Fmod(U, 1.0f) + 1.0f, 1.0f);
	V = FMath::Fmod(FMath::Fmod(V, 1.0f) + 1.0f, 1.0f);

	const float Fx = U * GridCells;
	const float Fy = V * GridCells;
	const int32 CellX = FMath::FloorToInt(Fx);
	const int32 CellY = FMath::FloorToInt(Fy);

	float MinDist = 10.0f;
	int32 BestCellX = CellX;
	int32 BestCellY = CellY;

	for (int32 Dx = -1; Dx <= 1; ++Dx)
	{
		for (int32 Dy = -1; Dy <= 1; ++Dy)
		{
			const int32 Cx = (CellX + Dx + GridCells * 2) % GridCells;
			const int32 Cy = (CellY + Dy + GridCells * 2) % GridCells;

			uint32 H = static_cast<uint32>(Cx * 374761393 + Cy * 668265263);
			H = (H ^ (H >> 13)) * 1274126177;
			const float Px = static_cast<float>(H & 0xFFFF) / 65535.0f;
			const float Py = static_cast<float>((H >> 16) & 0xFFFF) / 65535.0f;

			const float DiffX = (CellX + Dx + Px) - Fx;
			const float DiffY = (CellY + Dy + Py) - Fy;
			const float Dist = FMath::Sqrt(DiffX * DiffX + DiffY * DiffY);
			if (Dist < MinDist)
			{
				MinDist = Dist;
				BestCellX = Cx;
				BestCellY = Cy;
			}
		}
	}

	const uint32 Seed = static_cast<uint32>(BestCellX * 1013904223 + BestCellY * 1664525);
	const float TiltX = (static_cast<float>(Seed & 0xFF) / 255.0f - 0.5f) * 0.75f;
	const float TiltY = (static_cast<float>((Seed >> 8) & 0xFF) / 255.0f - 0.5f) * 0.75f;
	OutCellNormal = FVector(TiltX, TiltY, 1.0f).GetSafeNormal();

	return FMath::Clamp(MinDist, 0.0f, 1.0f);
}

UTexture2D* UFPSCoastalMaterialLibrary::CreateTransientTexture(int32 Width, int32 Height, const TArray<FColor>& Pixels, bool bSRGB, bool bIsNormal)
{
	if (Width <= 0 || Height <= 0 || Pixels.Num() != Width * Height)
	{
		return nullptr;
	}

	UTexture2D* Texture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
	if (!Texture)
	{
		return nullptr;
	}

	Texture->SRGB = bSRGB;
	Texture->CompressionSettings = bIsNormal ? TC_Normalmap : TC_Default;
	Texture->Filter = TF_Trilinear;
	Texture->AddressX = TA_Wrap;
	Texture->AddressY = TA_Wrap;

	if (Texture->GetPlatformData() && Texture->GetPlatformData()->Mips.Num() > 0)
	{
		FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
		void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
		if (Data)
		{
			FMemory::Memcpy(Data, Pixels.GetData(), Pixels.Num() * sizeof(FColor));
			Mip.BulkData.Unlock();
		}
	}

	Texture->UpdateResource();
	Texture->AddToRoot();
	return Texture;
}

void UFPSCoastalMaterialLibrary::GenerateSandTextures(FVector2D WindDir, TArray<FColor>& OutBaseColor, TArray<FColor>& OutNormal, TArray<FColor>& OutRoughness)
{
	const int32 Width = 256;
	const int32 Height = 256;
	const int32 Total = Width * Height;

	OutBaseColor.SetNumUninitialized(Total);
	OutNormal.SetNumUninitialized(Total);
	OutRoughness.SetNumUninitialized(Total);

	FVector2D WDir = WindDir.GetSafeNormal();
	if (WDir.IsNearlyZero())
	{
		WDir = FVector2D(1.0f, 0.0f);
	}
	const FVector2D RippleDir(-WDir.Y, WDir.X);
	int32 Ku = FMath::RoundToInt(RippleDir.X * 12.0f);
	int32 Kv = FMath::RoundToInt(RippleDir.Y * 12.0f);
	if (Ku == 0 && Kv == 0) Ku = 12;

	// Warm North Sea golden-grey sand
	const FLinearColor BaseSand(0.40f, 0.38f, 0.33f, 1.0f);

	for (int32 Y = 0; Y < Height; ++Y)
	{
		const float V = static_cast<float>(Y) / static_cast<float>(Height);
		for (int32 X = 0; X < Width; ++X)
		{
			const float U = static_cast<float>(X) / static_cast<float>(Width);
			const int32 Index = Y * Width + X;

			// Dual-frequency non-harmonic macro/micro variation (1.0x + 0.187x)
			const float Micro = SeamlessNoise(U, V, 16.0f, 16.0f);
			const float Macro = SeamlessNoise(U, V, 3.0f, 3.0f);
			const float BlendNoise = Micro * 0.65f + Macro * 0.35f;

			FLinearColor LinearCol = BaseSand * (0.92f + BlendNoise * 0.16f);

			// CRITICAL: Strictly clamp linear BaseColor luminance between 0.36 and 0.42
			// to prevent AEM_Manual exposure glare and keep cyan Fermi Bubbles high-contrast!
			const float Lum = 0.2126f * LinearCol.R + 0.7152f * LinearCol.G + 0.0722f * LinearCol.B;
			if (Lum < 0.36f)
			{
				LinearCol *= (0.36f / FMath::Max(Lum, 0.001f));
			}
			else if (Lum > 0.42f)
			{
				LinearCol *= (0.42f / Lum);
			}

			OutBaseColor[Index] = LinearCol.ToFColor(true);

			// Directional transverse wind ripples (5cm - 15cm wavelength)
			const float Phase = (U * Ku + V * Kv) * 2.0f * UE_PI;
			const float DuDh = -Ku * 2.0f * UE_PI * (FMath::Sin(Phase) * 0.55f + FMath::Sin(Phase * 2.0f + 0.5f) * 0.40f) * 0.015f;
			const float DvDh = -Kv * 2.0f * UE_PI * (FMath::Sin(Phase) * 0.55f + FMath::Sin(Phase * 2.0f + 0.5f) * 0.40f) * 0.015f;
			const FVector Norm = FVector(-DuDh, -DvDh, 1.0f).GetSafeNormal();

			const uint8 Nr = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt((Norm.X * 0.5f + 0.5f) * 255.0f), 0, 255));
			const uint8 Ng = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt((Norm.Y * 0.5f + 0.5f) * 255.0f), 0, 255));
			const uint8 Nb = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt((Norm.Z * 0.5f + 0.5f) * 255.0f), 0, 255));
			OutNormal[Index] = FColor(Nr, Ng, Nb, 255);

			// Granular Sand Roughness (~0.88)
			const uint8 RoughVal = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt((0.85f + Micro * 0.06f) * 255.0f), 0, 255));
			OutRoughness[Index] = FColor(RoughVal, RoughVal, RoughVal, 255);
		}
	}
}

void UFPSCoastalMaterialLibrary::GenerateTimberTextures(TArray<FColor>& OutBaseColor, TArray<FColor>& OutNormal, TArray<FColor>& OutRoughness)
{
	const int32 Width = 256;
	const int32 Height = 256;
	const int32 Total = Width * Height;

	OutBaseColor.SetNumUninitialized(Total);
	OutNormal.SetNumUninitialized(Total);
	OutRoughness.SetNumUninitialized(Total);

	// Silvered maritime oak/pine planks
	const FLinearColor DryTimber(0.32f, 0.30f, 0.27f, 1.0f);
	const FLinearColor DampTimber(0.18f, 0.16f, 0.14f, 1.0f);

	for (int32 Y = 0; Y < Height; ++Y)
	{
		const float V = static_cast<float>(Y) / static_cast<float>(Height);
		// 4 distinct planks along V (each 0.25 tall)
		const float PlankV = FMath::Fmod(V * 4.0f, 1.0f);
		const float DistFromEdge = FMath::Min(PlankV, 1.0f - PlankV);
		const bool bIsGap = (DistFromEdge < 0.035f);

		for (int32 X = 0; X < Width; ++X)
		{
			const float U = static_cast<float>(X) / static_cast<float>(Width);
			const int32 Index = Y * Width + X;

			// Longitudinal high-frequency wood grain along U
			const float Grain = SeamlessNoise(U * 24.0f, V * 3.0f, 24.0f, 3.0f);
			const float Macro = SeamlessNoise(U * 2.0f, V * 1.0f, 2.0f, 1.0f);

			// Low-tide dampness darkening at bottom edge (near waterline)
			const float DampFactor = FMath::Clamp((V - 0.70f) / 0.25f, 0.0f, 1.0f) * (0.6f + Macro * 0.4f);

			FLinearColor LinearCol = FMath::Lerp(DryTimber, DampTimber, DampFactor);
			LinearCol *= (0.88f + Grain * 0.24f);

			float NormalY = 0.0f;
			if (bIsGap)
			{
				LinearCol *= 0.25f; // Ambient Occlusion groove
				NormalY = (PlankV < 0.5f) ? -0.7f : 0.7f;
			}

			OutBaseColor[Index] = LinearCol.ToFColor(true);

			const float NormalX = (Grain - 0.5f) * 0.25f;
			const FVector Norm = FVector(NormalX, NormalY, 1.0f).GetSafeNormal();
			const uint8 Nr = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt((Norm.X * 0.5f + 0.5f) * 255.0f), 0, 255));
			const uint8 Ng = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt((Norm.Y * 0.5f + 0.5f) * 255.0f), 0, 255));
			const uint8 Nb = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt((Norm.Z * 0.5f + 0.5f) * 255.0f), 0, 255));
			OutNormal[Index] = FColor(Nr, Ng, Nb, 255);

			// Roughness: 0.70 dry wood, 0.48 damp wood
			const float Roughness = FMath::Lerp(0.70f, 0.48f, DampFactor);
			const uint8 RoughVal = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(Roughness * 255.0f), 0, 255));
			OutRoughness[Index] = FColor(RoughVal, RoughVal, RoughVal, 255);
		}
	}
}

void UFPSCoastalMaterialLibrary::GenerateConcreteTextures(TArray<FColor>& OutBaseColor, TArray<FColor>& OutNormal, TArray<FColor>& OutRoughness)
{
	const int32 Width = 256;
	const int32 Height = 256;
	const int32 Total = Width * Height;

	OutBaseColor.SetNumUninitialized(Total);
	OutNormal.SetNumUninitialized(Total);
	OutRoughness.SetNumUninitialized(Total);

	// Reinforced Seawall Concrete
	const FLinearColor BaseConcrete(0.35f, 0.35f, 0.34f, 1.0f);
	const FLinearColor SaltStain(0.44f, 0.44f, 0.42f, 1.0f);

	for (int32 Y = 0; Y < Height; ++Y)
	{
		const float V = static_cast<float>(Y) / static_cast<float>(Height);
		for (int32 X = 0; X < Width; ++X)
		{
			const float U = static_cast<float>(X) / static_cast<float>(Width);
			const int32 Index = Y * Width + X;

			const float PoreNoise = SeamlessNoise(U, V, 32.0f, 32.0f);
			const float MacroSalt = SeamlessNoise(U, V, 4.0f, 4.0f);

			const float SaltFactor = FMath::Clamp((MacroSalt - 0.52f) * 3.5f, 0.0f, 1.0f);
			FLinearColor LinearCol = FMath::Lerp(BaseConcrete, SaltStain, SaltFactor);
			LinearCol *= (0.92f + PoreNoise * 0.16f);

			OutBaseColor[Index] = LinearCol.ToFColor(true);

			// Pitted concrete micro-normal
			const float Du = (SeamlessNoise(U + 0.02f, V, 32.0f, 32.0f) - PoreNoise) * 0.4f;
			const float Dv = (SeamlessNoise(U, V + 0.02f, 32.0f, 32.0f) - PoreNoise) * 0.4f;
			const FVector Norm = FVector(-Du, -Dv, 1.0f).GetSafeNormal();

			const uint8 Nr = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt((Norm.X * 0.5f + 0.5f) * 255.0f), 0, 255));
			const uint8 Ng = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt((Norm.Y * 0.5f + 0.5f) * 255.0f), 0, 255));
			const uint8 Nb = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt((Norm.Z * 0.5f + 0.5f) * 255.0f), 0, 255));
			OutNormal[Index] = FColor(Nr, Ng, Nb, 255);

			// Roughness: 0.78 calibrated base
			const float Roughness = FMath::Lerp(0.78f, 0.68f, SaltFactor);
			const uint8 RoughVal = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(Roughness * 255.0f), 0, 255));
			OutRoughness[Index] = FColor(RoughVal, RoughVal, RoughVal, 255);
		}
	}
}

void UFPSCoastalMaterialLibrary::GenerateBasaltTextures(TArray<FColor>& OutBaseColor, TArray<FColor>& OutNormal, TArray<FColor>& OutRoughness)
{
	const int32 Width = 256;
	const int32 Height = 256;
	const int32 Total = Width * Height;

	OutBaseColor.SetNumUninitialized(Total);
	OutNormal.SetNumUninitialized(Total);
	OutRoughness.SetNumUninitialized(Total);

	// Dark volcanic armor stone (Linear Albedo = 0.14)
	const FLinearColor BasaltBase(0.14f, 0.14f, 0.14f, 1.0f);

	for (int32 Y = 0; Y < Height; ++Y)
	{
		const float V = static_cast<float>(Y) / static_cast<float>(Height);
		for (int32 X = 0; X < Width; ++X)
		{
			const float U = static_cast<float>(X) / static_cast<float>(Width);
			const int32 Index = Y * Width + X;

			FVector FacetNormal;
			const float CellDist = VoronoiCell(U, V, 10, FacetNormal);
			const float MicroGrain = SeamlessNoise(U, V, 28.0f, 28.0f);

			// Edge crevice darkening
			const float EdgeDarkening = FMath::Clamp(CellDist * 2.2f, 0.45f, 1.0f);
			FLinearColor LinearCol = BasaltBase * (0.85f + MicroGrain * 0.30f) * EdgeDarkening;

			OutBaseColor[Index] = LinearCol.ToFColor(true);

			const FVector Norm = FacetNormal;
			const uint8 Nr = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt((Norm.X * 0.5f + 0.5f) * 255.0f), 0, 255));
			const uint8 Ng = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt((Norm.Y * 0.5f + 0.5f) * 255.0f), 0, 255));
			const uint8 Nb = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt((Norm.Z * 0.5f + 0.5f) * 255.0f), 0, 255));
			OutNormal[Index] = FColor(Nr, Ng, Nb, 255);

			// Roughness: 0.65 calibrated
			const uint8 RoughVal = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt((0.65f + MicroGrain * 0.08f) * 255.0f), 0, 255));
			OutRoughness[Index] = FColor(RoughVal, RoughVal, RoughVal, 255);
		}
	}
}

void UFPSCoastalMaterialLibrary::GenerateAsphaltTextures(TArray<FColor>& OutBaseColor, TArray<FColor>& OutNormal, TArray<FColor>& OutRoughness)
{
	const int32 Width = 256;
	const int32 Height = 256;
	const int32 Total = Width * Height;

	OutBaseColor.SetNumUninitialized(Total);
	OutNormal.SetNumUninitialized(Total);
	OutRoughness.SetNumUninitialized(Total);

	// Charcoal macadam (Linear Albedo = 0.18)
	const FLinearColor AsphaltBase(0.18f, 0.18f, 0.17f, 1.0f);

	for (int32 Y = 0; Y < Height; ++Y)
	{
		const float V = static_cast<float>(Y) / static_cast<float>(Height);
		for (int32 X = 0; X < Width; ++X)
		{
			const float U = static_cast<float>(X) / static_cast<float>(Width);
			const int32 Index = Y * Width + X;

			const float Pebble = SeamlessNoise(U, V, 48.0f, 48.0f);
			const float Macro = SeamlessNoise(U, V, 4.0f, 4.0f);

			// High-frequency aggregate pebble speckle
			float Specks = (Pebble > 0.65f) ? 1.35f : ((Pebble < 0.35f) ? 0.80f : 1.0f);
			FLinearColor LinearCol = AsphaltBase * Specks * (0.90f + Macro * 0.20f);

			OutBaseColor[Index] = LinearCol.ToFColor(true);

			// Gravel micro-normal
			const float Du = (SeamlessNoise(U + 0.015f, V, 48.0f, 48.0f) - Pebble) * 0.5f;
			const float Dv = (SeamlessNoise(U, V + 0.015f, 48.0f, 48.0f) - Pebble) * 0.5f;
			const FVector Norm = FVector(-Du, -Dv, 1.0f).GetSafeNormal();

			const uint8 Nr = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt((Norm.X * 0.5f + 0.5f) * 255.0f), 0, 255));
			const uint8 Ng = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt((Norm.Y * 0.5f + 0.5f) * 255.0f), 0, 255));
			const uint8 Nb = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt((Norm.Z * 0.5f + 0.5f) * 255.0f), 0, 255));
			OutNormal[Index] = FColor(Nr, Ng, Nb, 255);

			// Roughness: 0.72 calibrated
			const uint8 RoughVal = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt((0.72f + Pebble * 0.06f) * 255.0f), 0, 255));
			OutRoughness[Index] = FColor(RoughVal, RoughVal, RoughVal, 255);
		}
	}
}

UTexture2D* UFPSCoastalMaterialLibrary::GetOrCreateBaseColorTexture(ECoastalSurfaceType SurfaceType, FVector2D WindDirection)
{
	const uint32 Key = MakeCacheKey(SurfaceType, WindDirection);
	if (TObjectPtr<UTexture2D>* Found = GCoastalBaseColorCache.Find(Key))
	{
		if (Found->Get()) return Found->Get();
	}

	TArray<FColor> BaseColors, Normals, Roughnesses;
	switch (SurfaceType)
	{
	case ECoastalSurfaceType::FineBeachSand:
		GenerateSandTextures(WindDirection, BaseColors, Normals, Roughnesses);
		break;
	case ECoastalSurfaceType::WeatheredPierTimber:
		GenerateTimberTextures(BaseColors, Normals, Roughnesses);
		break;
	case ECoastalSurfaceType::MarineCastConcrete:
		GenerateConcreteTextures(BaseColors, Normals, Roughnesses);
		break;
	case ECoastalSurfaceType::BasaltRiprapRock:
		GenerateBasaltTextures(BaseColors, Normals, Roughnesses);
		break;
	case ECoastalSurfaceType::WeatheredAsphalt:
	default:
		GenerateAsphaltTextures(BaseColors, Normals, Roughnesses);
		break;
	}

	UTexture2D* BaseTex = CreateTransientTexture(256, 256, BaseColors, true, false);
	UTexture2D* NormTex = CreateTransientTexture(256, 256, Normals, false, true);
	UTexture2D* RoughTex = CreateTransientTexture(256, 256, Roughnesses, false, false);

	if (BaseTex) GCoastalBaseColorCache.Add(Key, BaseTex);
	if (NormTex) GCoastalNormalCache.Add(Key, NormTex);
	if (RoughTex) GCoastalRoughnessCache.Add(static_cast<uint8>(SurfaceType), RoughTex);

	return BaseTex;
}

UTexture2D* UFPSCoastalMaterialLibrary::GetOrCreateNormalTexture(ECoastalSurfaceType SurfaceType, FVector2D WindDirection)
{
	const uint32 Key = MakeCacheKey(SurfaceType, WindDirection);
	if (TObjectPtr<UTexture2D>* Found = GCoastalNormalCache.Find(Key))
	{
		if (Found->Get()) return Found->Get();
	}

	// Generating base color generates normal as well
	GetOrCreateBaseColorTexture(SurfaceType, WindDirection);

	if (TObjectPtr<UTexture2D>* Found = GCoastalNormalCache.Find(Key))
	{
		return Found->Get();
	}
	return nullptr;
}

UTexture2D* UFPSCoastalMaterialLibrary::GetOrCreateRoughnessTexture(ECoastalSurfaceType SurfaceType)
{
	const uint8 Key = static_cast<uint8>(SurfaceType);
	if (TObjectPtr<UTexture2D>* Found = GCoastalRoughnessCache.Find(Key))
	{
		if (Found->Get()) return Found->Get();
	}

	GetOrCreateBaseColorTexture(SurfaceType, FVector2D(1.0f, 0.0f));

	if (TObjectPtr<UTexture2D>* Found = GCoastalRoughnessCache.Find(Key))
	{
		return Found->Get();
	}
	return nullptr;
}

UMaterialInterface* UFPSCoastalMaterialLibrary::LoadFavelaMaterial(const TCHAR* MaterialName)
{
	const FString Path = FString::Printf(TEXT("/Game/Environment/Favela/Materials/%s.%s"), MaterialName, MaterialName);
	return Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, *Path));
}

UMaterialInterface* UFPSCoastalMaterialLibrary::GetOrCreateCoastalMaterial(UObject* WorldContext, ECoastalSurfaceType SurfaceType, FVector2D WindDirection)
{
	const uint32 Key = MakeCacheKey(SurfaceType, WindDirection);
	if (TObjectPtr<UMaterialInterface>* Found = GCoastalMaterialCache.Find(Key))
	{
		if (Found->Get()) return Found->Get();
	}

	const TCHAR* FavMatName = TEXT("MI_Favela_Concrete");
	switch (SurfaceType)
	{
	case ECoastalSurfaceType::FineBeachSand:
		FavMatName = TEXT("MI_Favela_Sand");
		break;
	case ECoastalSurfaceType::WeatheredPierTimber:
		FavMatName = TEXT("MI_Favela_Wood");
		break;
	case ECoastalSurfaceType::MarineCastConcrete:
		FavMatName = TEXT("MI_Favela_Concrete");
		break;
	case ECoastalSurfaceType::BasaltRiprapRock:
	case ECoastalSurfaceType::FavelaCobble:
		FavMatName = TEXT("MI_Favela_Cobble");
		break;
	case ECoastalSurfaceType::WeatheredAsphalt:
		FavMatName = TEXT("MI_Favela_Asphalt");
		break;
	case ECoastalSurfaceType::FavelaBrick:
		FavMatName = TEXT("MI_Favela_Brick");
		break;
	case ECoastalSurfaceType::FavelaCinder:
		FavMatName = TEXT("MI_Favela_Cinder");
		break;
	case ECoastalSurfaceType::FavelaStucco:
		FavMatName = TEXT("MI_Favela_Stucco");
		break;
	case ECoastalSurfaceType::FavelaDirt:
		FavMatName = TEXT("MI_Favela_Dirt");
		break;
	case ECoastalSurfaceType::FavelaMetal:
		FavMatName = TEXT("MI_Favela_Metal");
		break;
	case ECoastalSurfaceType::FavelaPuddle:
		FavMatName = TEXT("MI_Favela_Puddle");
		break;
	}

	if (UMaterialInterface* FavMat = LoadFavelaMaterial(FavMatName))
	{
		FavMat->AddToRoot();
		GCoastalMaterialCache.Add(Key, FavMat);
		return FavMat;
	}

	UTexture2D* BaseColorTex = GetOrCreateBaseColorTexture(SurfaceType, WindDirection);
	UTexture2D* NormalTex = GetOrCreateNormalTexture(SurfaceType, WindDirection);
	UTexture2D* RoughnessTex = GetOrCreateRoughnessTexture(SurfaceType);

	UMaterialInterface* BaseMat = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")));
	if (!BaseMat)
	{
		BaseMat = UMaterial::GetDefaultMaterial(MD_Surface);
	}
	if (!BaseMat)
	{
		return nullptr;
	}

	UObject* Outer = WorldContext ? WorldContext : (UObject*)GetTransientPackage();
	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, Outer);
	if (!MID)
	{
		return BaseMat;
	}

	const FLinearColor Albedo = GetSurfaceBaseColor(SurfaceType);
	const float Roughness = GetSurfaceRoughness(SurfaceType);
	const float Metallic = GetSurfaceMetallic(SurfaceType);
	const float Specular = GetSurfaceSpecular(SurfaceType);

	MID->SetVectorParameterValue(TEXT("Color"), Albedo);
	MID->SetVectorParameterValue(TEXT("BaseColor"), Albedo);
	MID->SetVectorParameterValue(TEXT("Albedo"), Albedo);
	MID->SetVectorParameterValue(TEXT("DiffuseColor"), Albedo);
	MID->SetScalarParameterValue(TEXT("Roughness"), Roughness);
	MID->SetScalarParameterValue(TEXT("RoughnessBase"), Roughness);
	MID->SetScalarParameterValue(TEXT("Metallic"), Metallic);
	MID->SetScalarParameterValue(TEXT("Specular"), Specular);

	if (BaseColorTex)
	{
		MID->SetTextureParameterValue(TEXT("Texture"), BaseColorTex);
		MID->SetTextureParameterValue(TEXT("BaseColorTexture"), BaseColorTex);
	}
	if (NormalTex)
	{
		MID->SetTextureParameterValue(TEXT("NormalMap"), NormalTex);
	}
	if (RoughnessTex)
	{
		MID->SetTextureParameterValue(TEXT("RoughnessMap"), RoughnessTex);
	}

	MID->AddToRoot();
	GCoastalMaterialCache.Add(Key, MID);
	return MID;
}

void UFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(UStaticMeshComponent* MeshComp, ECoastalSurfaceType SurfaceType, int32 MaterialSlot, FVector2D WindDirection)
{
	if (!MeshComp) return;

	UMaterialInterface* Mat = GetOrCreateCoastalMaterial(MeshComp, SurfaceType, WindDirection);
	if (Mat)
	{
		MeshComp->SetMaterial(MaterialSlot, Mat);
	}

	// Surface acoustic tagging for FPSAudioSubsystem
	FName PhysTag;
	switch (SurfaceType)
	{
	case ECoastalSurfaceType::FineBeachSand:
	case ECoastalSurfaceType::FavelaDirt:
		PhysTag = FName(TEXT("Sand"));
		break;
	case ECoastalSurfaceType::WeatheredPierTimber:
		PhysTag = FName(TEXT("Wood"));
		break;
	case ECoastalSurfaceType::FavelaMetal:
		PhysTag = FName(TEXT("Metal"));
		break;
	case ECoastalSurfaceType::FavelaPuddle:
		PhysTag = FName(TEXT("Water"));
		break;
	case ECoastalSurfaceType::MarineCastConcrete:
	case ECoastalSurfaceType::WeatheredAsphalt:
	case ECoastalSurfaceType::FavelaBrick:
	case ECoastalSurfaceType::FavelaCinder:
	case ECoastalSurfaceType::FavelaStucco:
	case ECoastalSurfaceType::FavelaCobble:
		PhysTag = FName(TEXT("Concrete"));
		break;
	case ECoastalSurfaceType::BasaltRiprapRock:
		PhysTag = FName(TEXT("Rock"));
		break;
	}

	if (!PhysTag.IsNone() && !MeshComp->ComponentTags.Contains(PhysTag))
	{
		MeshComp->ComponentTags.Add(PhysTag);
	}
}

FLinearColor UFPSCoastalMaterialLibrary::GetSurfaceBaseColor(ECoastalSurfaceType SurfaceType)
{
	switch (SurfaceType)
	{
	case ECoastalSurfaceType::FineBeachSand:
		return FLinearColor(0.40f, 0.38f, 0.33f, 1.0f); // Luminance calibrated ~0.38
	case ECoastalSurfaceType::WeatheredPierTimber:
		return FLinearColor(0.32f, 0.30f, 0.27f, 1.0f);
	case ECoastalSurfaceType::MarineCastConcrete:
		return FLinearColor(0.35f, 0.35f, 0.34f, 1.0f);
	case ECoastalSurfaceType::BasaltRiprapRock:
		return FLinearColor(0.14f, 0.14f, 0.14f, 1.0f); // Calibrated Linear Albedo 0.14
	case ECoastalSurfaceType::WeatheredAsphalt:
	default:
		return FLinearColor(0.18f, 0.18f, 0.17f, 1.0f); // Calibrated Linear Albedo 0.18
	}
}

float UFPSCoastalMaterialLibrary::GetSurfaceRoughness(ECoastalSurfaceType SurfaceType)
{
	switch (SurfaceType)
	{
	case ECoastalSurfaceType::FineBeachSand:
		return 0.88f;
	case ECoastalSurfaceType::WeatheredPierTimber:
		return 0.70f;
	case ECoastalSurfaceType::MarineCastConcrete:
		return 0.78f; // Calibrated 0.78
	case ECoastalSurfaceType::BasaltRiprapRock:
		return 0.65f; // Calibrated 0.65
	case ECoastalSurfaceType::WeatheredAsphalt:
	default:
		return 0.72f; // Calibrated 0.72
	}
}

float UFPSCoastalMaterialLibrary::GetSurfaceMetallic(ECoastalSurfaceType SurfaceType)
{
	return 0.0f; // All coastal terrain archetypes are non-metallic dielectrics
}

float UFPSCoastalMaterialLibrary::GetSurfaceSpecular(ECoastalSurfaceType SurfaceType)
{
	switch (SurfaceType)
	{
	case ECoastalSurfaceType::BasaltRiprapRock:
		return 0.55f;
	case ECoastalSurfaceType::FineBeachSand:
		return 0.35f;
	default:
		return 0.50f;
	}
}

void UFPSCoastalMaterialLibrary::ClearMaterialCache()
{
	GCoastalMaterialCache.Empty();
	GCoastalBaseColorCache.Empty();
	GCoastalNormalCache.Empty();
	GCoastalRoughnessCache.Empty();
}

// =========================================================================================
// INSTANCED FAVELA ENVIRONMENT KIT BUILDER
// =========================================================================================

static UInstancedStaticMeshComponent* CreateFavelaISM(AActor* Owner, USceneComponent* Root, const FName& Name, UStaticMesh* Mesh, const TCHAR* MatName, bool bCollides, const FName& PhysTag)
{
	if (!Owner || !Root || !Mesh) return nullptr;
	UInstancedStaticMeshComponent* ISM = NewObject<UInstancedStaticMeshComponent>(Owner, Name);
	if (!ISM) return nullptr;
	ISM->SetupAttachment(Root);
	ISM->SetMobility(EComponentMobility::Movable);
	ISM->SetStaticMesh(Mesh);
	ISM->NumCustomDataFloats = 6;
	ISM->SetCastShadow(bCollides);
	ISM->SetCollisionProfileName(bCollides ? TEXT("BlockAll") : TEXT("NoCollision"));
	if (!PhysTag.IsNone())
	{
		ISM->ComponentTags.Add(PhysTag);
	}
	if (UMaterialInterface* Mat = UFPSCoastalMaterialLibrary::LoadFavelaMaterial(MatName))
	{
		ISM->SetMaterial(0, Mat);
	}
	ISM->RegisterComponent();
	return ISM;
}

void FFPSFavelaEnvironmentKit::Init(AActor* InOwner, USceneComponent* InRoot)
{
	Owner = InOwner;
	Root = InRoot;

	CubeMesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
	CylinderMesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
	RoofSheetMesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, TEXT("/Game/Environment/Favela/Meshes/SM_Favela_RoofSheet.SM_Favela_RoofSheet")));
	CableMesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, TEXT("/Game/Environment/Favela/Meshes/SM_Favela_Cable.SM_Favela_Cable")));
	WeedsMesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, TEXT("/Game/Environment/Favela/Meshes/SM_Favela_Weeds.SM_Favela_Weeds")));
	IvyMesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, TEXT("/Game/Environment/Favela/Meshes/SM_Favela_Ivy.SM_Favela_Ivy")));
	BananaMesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, TEXT("/Game/Environment/Favela/Meshes/SM_Favela_Banana.SM_Favela_Banana")));
	TireMesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, TEXT("/Game/Environment/Favela/Meshes/SM_Favela_Tire.SM_Favela_Tire")));
	WaterTankMesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, TEXT("/Game/Environment/Favela/Meshes/SM_Favela_WaterTank.SM_Favela_WaterTank")));

	BrickISM = CreateFavelaISM(Owner, Root, TEXT("Favela_Brick_ISM"), CubeMesh, TEXT("MI_Favela_Brick"), true, TEXT("Concrete"));
	CinderISM = CreateFavelaISM(Owner, Root, TEXT("Favela_Cinder_ISM"), CubeMesh, TEXT("MI_Favela_Cinder"), true, TEXT("Concrete"));
	StuccoISM = CreateFavelaISM(Owner, Root, TEXT("Favela_Stucco_ISM"), CubeMesh, TEXT("MI_Favela_Stucco"), true, TEXT("Concrete"));
	ConcreteISM = CreateFavelaISM(Owner, Root, TEXT("Favela_Concrete_ISM"), CubeMesh, TEXT("MI_Favela_Concrete"), true, TEXT("Concrete"));
	CobbleISM = CreateFavelaISM(Owner, Root, TEXT("Favela_Cobble_ISM"), CubeMesh, TEXT("MI_Favela_Cobble"), false, TEXT("Concrete"));
	AsphaltISM = CreateFavelaISM(Owner, Root, TEXT("Favela_Asphalt_ISM"), CubeMesh, TEXT("MI_Favela_Asphalt"), false, TEXT("Concrete"));
	DirtISM = CreateFavelaISM(Owner, Root, TEXT("Favela_Dirt_ISM"), CubeMesh, TEXT("MI_Favela_Dirt"), false, TEXT("Sand"));
	WoodISM = CreateFavelaISM(Owner, Root, TEXT("Favela_Wood_ISM"), CubeMesh, TEXT("MI_Favela_Wood"), false, TEXT("Wood"));
	MetalISM = CreateFavelaISM(Owner, Root, TEXT("Favela_Metal_ISM"), CubeMesh, TEXT("MI_Favela_Metal"), false, TEXT("Metal"));
	DarkISM = CreateFavelaISM(Owner, Root, TEXT("Favela_Dark_ISM"), CubeMesh, TEXT("MI_Favela_Dark"), false, NAME_None);
	PuddleISM = CreateFavelaISM(Owner, Root, TEXT("Favela_Puddle_ISM"), CubeMesh, TEXT("MI_Favela_Puddle"), false, TEXT("Water"));

	RoofSheetISM = CreateFavelaISM(Owner, Root, TEXT("Favela_RoofSheet_ISM"), RoofSheetMesh ? RoofSheetMesh : CubeMesh, TEXT("MI_Favela_RoofMetal"), false, TEXT("Metal"));
	TireISM = CreateFavelaISM(Owner, Root, TEXT("Favela_Tire_ISM"), TireMesh ? TireMesh : CylinderMesh, TEXT("MI_Favela_Rubber"), false, NAME_None);
	WaterTankISM = CreateFavelaISM(Owner, Root, TEXT("Favela_WaterTank_ISM"), WaterTankMesh ? WaterTankMesh : CylinderMesh, TEXT("MI_Favela_Plastic"), false, NAME_None);
	WeedsISM = CreateFavelaISM(Owner, Root, TEXT("Favela_Weeds_ISM"), WeedsMesh, TEXT("MI_Favela_Weeds"), false, NAME_None);
	IvyISM = CreateFavelaISM(Owner, Root, TEXT("Favela_Ivy_ISM"), IvyMesh, TEXT("MI_Favela_Ivy"), false, NAME_None);
	BananaISM = CreateFavelaISM(Owner, Root, TEXT("Favela_Banana_ISM"), BananaMesh, TEXT("MI_Favela_Banana"), false, NAME_None);
	CableISM = CreateFavelaISM(Owner, Root, TEXT("Favela_Cable_ISM"), CableMesh, TEXT("MI_Favela_Cable"), false, NAME_None);
}

void FFPSFavelaEnvironmentKit::AddBox(UInstancedStaticMeshComponent* ISM, const FVector& Center, const FVector& SizeCm, const FRotator& Rot, const FLinearColor& Tint, float Peel, float BaseZ)
{
	if (!ISM) return;
	const FTransform Xf(Rot, Center, SizeCm / 100.0f);
	const int32 Idx = ISM->AddInstance(Xf, false);
	const TArray<float> Custom = { Tint.R, Tint.G, Tint.B, Peel, BaseZ, 0.0f };
	ISM->SetCustomData(Idx, Custom, false);
}

void FFPSFavelaEnvironmentKit::AddMesh(UInstancedStaticMeshComponent* ISM, const FVector& Loc, const FRotator& Rot, const FVector& Scale, const FLinearColor& Tint, float Peel, float BaseZ)
{
	if (!ISM) return;
	const FTransform Xf(Rot, Loc, Scale);
	const int32 Idx = ISM->AddInstance(Xf, false);
	const TArray<float> Custom = { Tint.R, Tint.G, Tint.B, Peel, BaseZ, 0.0f };
	ISM->SetCustomData(Idx, Custom, false);
}

FLinearColor UFPSCoastalMaterialLibrary::GetFavelaStuccoPalette(int32 PaletteIdx)
{
	switch (FMath::Abs(PaletteIdx) % 6)
	{
	case 0: return FLinearColor(0.10f, 0.76f, 0.90f, 1.0f); // Copacabana Turquoise / Cyan
	case 1: return FLinearColor(0.96f, 0.44f, 0.20f, 1.0f); // Warm Terracotta Sienna / Tangerine
	case 2: return FLinearColor(0.98f, 0.78f, 0.16f, 1.0f); // Sun-Bleached Mustard Gold
	case 3: return FLinearColor(0.94f, 0.36f, 0.46f, 1.0f); // Vibrant Tropical Coral Rose
	case 4: return FLinearColor(0.18f, 0.82f, 0.48f, 1.0f); // Carioca Mint Green
	default: return FLinearColor(0.94f, 0.90f, 0.78f, 1.0f); // Aged Sun Plaster Cream
	}
}

void UFPSCoastalMaterialLibrary::ApplyFavelaSurfaceCustom(UStaticMeshComponent* MeshComp, ECoastalSurfaceType SurfaceType, const FLinearColor& Tint, float PeelAmount, int32 MaterialSlot)
{
	if (!MeshComp) return;

	const TCHAR* FavMatName = TEXT("MI_Favela_Concrete");
	switch (SurfaceType)
	{
	case ECoastalSurfaceType::FavelaStucco:
		FavMatName = TEXT("MI_Favela_Stucco");
		break;
	case ECoastalSurfaceType::FavelaBrick:
		FavMatName = TEXT("MI_Favela_Brick");
		break;
	case ECoastalSurfaceType::FavelaCinder:
		FavMatName = TEXT("MI_Favela_Cinder");
		break;
	case ECoastalSurfaceType::FavelaCobble:
		FavMatName = TEXT("MI_Favela_Cobble");
		break;
	case ECoastalSurfaceType::FavelaMetal:
		FavMatName = TEXT("MI_Favela_Metal");
		break;
	case ECoastalSurfaceType::WeatheredPierTimber:
		FavMatName = TEXT("MI_Favela_Wood");
		break;
	default:
		FavMatName = TEXT("MI_Favela_Concrete");
		break;
	}

	UMaterialInterface* ParentMat = LoadFavelaMaterial(FavMatName);
	if (!ParentMat)
	{
		ParentMat = GetOrCreateCoastalMaterial(MeshComp, SurfaceType);
	}
	if (!ParentMat) return;

	UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(MeshComp->GetMaterial(MaterialSlot));
	if (!MID || MID->Parent != ParentMat)
	{
		MID = UMaterialInstanceDynamic::Create(ParentMat, MeshComp);
		MeshComp->SetMaterial(MaterialSlot, MID);
	}
	if (MID)
	{
		MID->SetVectorParameterValue(TEXT("Tint"), Tint);
		MID->SetScalarParameterValue(TEXT("PeelAmount"), PeelAmount);
	}

	MeshComp->ComponentTags.AddUnique(FName(TEXT("Concrete")));
}

void UFPSCoastalMaterialLibrary::ApplyFavelaStuccoWithColor(UStaticMeshComponent* MeshComp, int32 PaletteIdx, float PeelAmount, int32 MaterialSlot)
{
	const FLinearColor Tint = GetFavelaStuccoPalette(PaletteIdx);
	ApplyFavelaSurfaceCustom(MeshComp, ECoastalSurfaceType::FavelaStucco, Tint, PeelAmount, MaterialSlot);
}

void FFPSFavelaEnvironmentKit::AddRoofCorrugated(const FVector& RoofCenterTop, float LengthX, float WidthY, float YawDeg, float PitchDeg, int32 Seed, bool bAddWaterTank)
{
	FRandomStream Rng(Seed * 7919 + 17);
	const FRotator BaseRot(0.0f, YawDeg, 0.0f);
	auto ToWorld = [&](const FVector& Local) -> FVector { return RoofCenterTop + BaseRot.RotateVector(Local); };

	// Timber purlin rafters under the corrugated sheets
	AddBox(WoodISM, ToWorld(FVector(-LengthX * 0.32f, 0.0f, 5.0f)), FVector(12.0f, WidthY + 35.0f, 10.0f), BaseRot);
	AddBox(WoodISM, ToWorld(FVector(0.0f, 0.0f, 5.0f)), FVector(12.0f, WidthY + 35.0f, 10.0f), BaseRot);
	AddBox(WoodISM, ToWorld(FVector(LengthX * 0.32f, 0.0f, 5.0f)), FVector(12.0f, WidthY + 35.0f, 10.0f), BaseRot);

	// Overlapping corrugated zinc sheets (SM_Favela_RoofSheet is 240cm x 110cm)
	const int32 ColsY = FMath::Max(2, FMath::RoundToInt((WidthY + 50.0f) / 100.0f));
	const int32 RowsX = (LengthX > 310.0f) ? 2 : 1;
	const float SheetLen = (LengthX + 55.0f) / RowsX;
	const float SheetWid = (WidthY + 55.0f) / ColsY;

	for (int32 rx = 0; rx < RowsX; ++rx)
	{
		for (int32 cy = 0; cy < ColsY; ++cy)
		{
			const float LocalX = (RowsX == 1) ? 0.0f : ((rx - 0.5f) * (SheetLen - 18.0f));
			const float LocalY = -WidthY * 0.5f + (cy + 0.5f) * (WidthY / ColsY);
			const float JitterYaw = Rng.FRandRange(-2.6f, 2.6f);
			const float JitterRoll = Rng.FRandRange(-1.8f, 1.8f);
			const float ZLift = 12.0f + (cy % 2) * 2.5f + rx * 4.0f;

			const FRotator SheetRot = (FQuat(BaseRot) * FQuat(FRotator(PitchDeg, JitterYaw, JitterRoll))).Rotator();

			// Authentic Brazilian painted corrugated tin roof palette
			static const FLinearColor RoofPalette[] = {
				FLinearColor(0.92f, 0.94f, 0.96f, 1.0f), // Galvanized zinc silver
				FLinearColor(0.10f, 0.48f, 0.95f, 1.0f), // Cobalt blue painted tin
				FLinearColor(0.88f, 0.22f, 0.16f, 1.0f), // Oxide red painted tin
				FLinearColor(0.18f, 0.68f, 0.38f, 1.0f), // Amazon moss green
				FLinearColor(0.96f, 0.74f, 0.16f, 1.0f), // Saffron gold painted tin
				FLinearColor(0.72f, 0.35f, 0.18f, 1.0f)  // Weathered rust orange
			};
			const int32 ColorChoice = (Seed + rx * 3 + cy * 5) % 6;
			const FLinearColor BaseTint = RoofPalette[ColorChoice];
			const float Jitter = Rng.FRandRange(0.88f, 1.12f);
			const FLinearColor Tint(BaseTint.R * Jitter, BaseTint.G * Jitter, BaseTint.B * Jitter, 1.0f);

			if (RoofSheetMesh)
			{
				AddMesh(RoofSheetISM, ToWorld(FVector(LocalX, LocalY, ZLift)), SheetRot, FVector(SheetLen / 240.0f, (SheetWid + 12.0f) / 110.0f, 1.35f), Tint);
			}
			else
			{
				AddBox(RoofSheetISM, ToWorld(FVector(LocalX, LocalY, ZLift)), FVector(SheetLen, SheetWid + 10.0f, 4.0f), SheetRot, Tint);
			}
		}
	}

	// Makeshift hold-down weights: discarded car tires & loose cinder blocks resting on the sheets
	const int32 NumTires = 2 + (Seed % 2);
	for (int32 t = 0; t < NumTires; ++t)
	{
		const float TX = Rng.FRandRange(-LengthX * 0.36f, LengthX * 0.36f);
		const float TY = Rng.FRandRange(-WidthY * 0.36f, WidthY * 0.36f);
		AddMesh(TireISM, ToWorld(FVector(TX, TY, 14.0f)), FRotator(PitchDeg, Rng.FRandRange(0.0f, 360.0f), 0.0f), FVector(1.05f, 1.05f, 1.0f));
	}
	for (int32 b = 0; b < 3; ++b)
	{
		const float BX = Rng.FRandRange(-LengthX * 0.42f, LengthX * 0.42f);
		const float BY = Rng.FRandRange(-WidthY * 0.42f, WidthY * 0.42f);
		AddBox(CinderISM, ToWorld(FVector(BX, BY, 19.0f)), FVector(39.0f, 19.0f, 14.0f), FRotator(PitchDeg, Rng.FRandRange(0.0f, 180.0f), 0.0f));
	}

	if (bAddWaterTank)
	{
		const FVector TankOffset(Rng.FRandRange(-LengthX * 0.22f, LengthX * 0.22f), Rng.FRandRange(-WidthY * 0.22f, WidthY * 0.22f), 16.0f);
		AddWaterTank(ToWorld(TankOffset), YawDeg, Seed);
	}
}

void FFPSFavelaEnvironmentKit::AddWaterTank(const FVector& Center, float YawDeg, int32 Seed)
{
	FRandomStream Rng(Seed * 5381 + 13);
	const FRotator Rot(0.0f, YawDeg, 0.0f);

	// Reinforced concrete base slab
	AddBox(ConcreteISM, Center + FVector(0.0f, 0.0f, 8.0f), FVector(125.0f, 125.0f, 16.0f), Rot);

	// Iconic Brazilian bright-blue 1000L polyethylene Fortlev water tank (caixa d'agua)
	const FLinearColor TankBlue = (Seed % 2 == 0)
		? FLinearColor(0.04f, 0.48f, 0.98f, 1.0f)
		: FLinearColor(0.03f, 0.58f, 0.92f, 1.0f);

	if (WaterTankMesh)
	{
		AddMesh(WaterTankISM, Center + FVector(0.0f, 0.0f, 18.0f), FRotator(0.0f, Rng.FRandRange(0.0f, 360.0f), 0.0f), FVector(1.10f, 1.10f, 1.10f), TankBlue);
	}
	else
	{
		AddBox(WaterTankISM, Center + FVector(0.0f, 0.0f, 65.0f), FVector(95.0f, 95.0f, 110.0f), Rot, TankBlue);
	}

	// Dark PVC top lid / inspection hatch
	AddBox(DarkISM, Center + FVector(0.0f, 0.0f, 125.0f), FVector(48.0f, 48.0f, 8.0f), Rot);

	// PVC water pipe drop line
	AddBox(DarkISM, Center + Rot.RotateVector(FVector(50.0f, 0.0f, 45.0f)), FVector(6.0f, 6.0f, 85.0f), Rot);
}

void FFPSFavelaEnvironmentKit::AddBougainvilleaCluster(const FVector& BaseLoc, const FRotator& Rot, const FLinearColor& FlowerColor, int32 Count)
{
	if (!IvyISM || Count <= 0) return;
	for (int32 i = 0; i < Count; ++i)
	{
		const float Spread = (i - (Count - 1) * 0.5f) * 36.0f;
		const FVector Loc = BaseLoc + Rot.RotateVector(FVector(0.0f, Spread, -i * 18.0f));
		const FRotator MeshRot = Rot + FRotator(FMath::FRandRange(-5.0f, 5.0f), FMath::FRandRange(-10.0f, 10.0f), FMath::FRandRange(-8.0f, 8.0f));
		const FVector Scale(FMath::FRandRange(1.05f, 1.35f), FMath::FRandRange(1.10f, 1.45f), FMath::FRandRange(1.15f, 1.55f));
		AddMesh(IvyISM, Loc, MeshRot, Scale, FlowerColor);
	}
}

void FFPSFavelaEnvironmentKit::AddMarketCanopy(const FVector& Center, const FVector& SizeCm, const FRotator& Rot, const FLinearColor& CanopyColor, int32 Seed)
{
	FRandomStream Rng(Seed * 4789 + 23);
	auto ToWorld = [&](const FVector& Local) -> FVector { return Center + Rot.RotateVector(Local); };

	const float HalfX = SizeCm.X * 0.5f;
	const float HalfY = SizeCm.Y * 0.5f;
	const float PostH = SizeCm.Z;

	// 4 Rough timber corner posts (caibros de eucalipto)
	for (float dx : { -1.0f, 1.0f })
	{
		for (float dy : { -1.0f, 1.0f })
		{
			const FVector PostLoc(dx * (HalfX - 6.0f), dy * (HalfY - 6.0f), PostH * 0.5f);
			AddBox(WoodISM, ToWorld(PostLoc), FVector(10.0f, 10.0f, PostH), Rot);
		}
	}

	// Upper timber frame perimeter
	AddBox(WoodISM, ToWorld(FVector(0.0f, -HalfY + 6.0f, PostH - 5.0f)), FVector(SizeCm.X, 10.0f, 10.0f), Rot);
	AddBox(WoodISM, ToWorld(FVector(0.0f, HalfY - 6.0f, PostH - 5.0f)), FVector(SizeCm.X, 10.0f, 10.0f), Rot);
	AddBox(WoodISM, ToWorld(FVector(-HalfX + 6.0f, 0.0f, PostH - 5.0f)), FVector(10.0f, SizeCm.Y, 10.0f), Rot);
	AddBox(WoodISM, ToWorld(FVector(HalfX - 6.0f, 0.0f, PostH - 5.0f)), FVector(10.0f, SizeCm.Y, 10.0f), Rot);

	// Slanted vibrant plastic tarp / canvas awning (lona de feira)
	const float SlopePitch = 9.5f;
	const FRotator TarpRot = (FQuat(Rot) * FQuat(FRotator(SlopePitch, 0.0f, 0.0f))).Rotator();
	if (RoofSheetMesh)
	{
		AddMesh(RoofSheetISM, ToWorld(FVector(0.0f, 0.0f, PostH + 6.0f)), TarpRot, FVector(SizeCm.X / 240.0f, SizeCm.Y / 110.0f, 1.2f), CanopyColor);
	}
	else
	{
		AddBox(ConcreteISM, ToWorld(FVector(0.0f, 0.0f, PostH + 6.0f)), FVector(SizeCm.X + 20.0f, SizeCm.Y + 20.0f, 6.0f), TarpRot, CanopyColor);
	}

	// Timber market counter table beneath the canopy
	AddBox(WoodISM, ToWorld(FVector(0.0f, 0.0f, 42.0f)), FVector(SizeCm.X * 0.75f, SizeCm.Y * 0.65f, 84.0f), Rot);

	// Discarded car tires and timber fruit crates around the base
	AddMesh(TireISM, ToWorld(FVector(HalfX - 25.0f, -HalfY - 20.0f, 12.0f)), FRotator(0.0f, Rng.FRandRange(0.0f, 360.0f), 0.0f), FVector(1.05f));
	AddBox(WoodISM, ToWorld(FVector(-HalfX + 30.0f, HalfY + 25.0f, 18.0f)), FVector(52.0f, 38.0f, 36.0f), Rot + FRotator(0.0f, 15.0f, 0.0f));
}

void FFPSFavelaEnvironmentKit::AddShanty(const FVector& BaseCenter, float SizeX, float SizeY, float YawDeg, int32 NumStories, int32 Seed, bool bCantileverUpper, bool bHasBalcony, bool bAddWaterTank)
{
	FRandomStream Rng(Seed * 4099 + 31);
	const FRotator HouseRot(0.0f, YawDeg, 0.0f);
	auto ToWorld = [&](const FVector& Local) -> FVector { return BaseCenter + HouseRot.RotateVector(Local); };

	const float StoryH = 290.0f;
	float TopSX = SizeX;
	float TopSY = SizeY;

	for (int32 s = 0; s < NumStories; ++s)
	{
		const float FloorZ = s * StoryH;
		const float SX = (s > 0 && bCantileverUpper) ? (SizeX + 44.0f) : SizeX;
		const float SY = (s > 0 && bCantileverUpper) ? (SizeY + 26.0f) : SizeY;
		TopSX = SX;
		TopSY = SY;

		// Pick authentic wall material: 66% vibrant Santa Marta painted stucco, 17% exposed brick, 17% cinder block
		const int32 MatChoice = (Seed + s * 7) % 6;
		UInstancedStaticMeshComponent* WallISM = StuccoISM;
		FLinearColor WallTint = UFPSCoastalMaterialLibrary::GetFavelaStuccoPalette(Seed + s * 5);
		float PeelVal = Rng.FRandRange(0.22f, 0.42f);

		if (MatChoice == 0)
		{
			WallISM = BrickISM;
			WallTint = FLinearColor::White;
			PeelVal = 0.0f;
		}
		else if (MatChoice == 1)
		{
			WallISM = CinderISM;
			WallTint = FLinearColor::White;
			PeelVal = 0.0f;
		}

		// Main masonry block
		AddBox(WallISM, ToWorld(FVector(0.0f, 0.0f, FloorZ + StoryH * 0.5f - 6.0f)), FVector(SX, SY, StoryH - 14.0f), HouseRot, WallTint, PeelVal, BaseCenter.Z + FloorZ);

		// Reinforced concrete ceiling slab (laje) with slight overhang
		AddBox(ConcreteISM, ToWorld(FVector(0.0f, 0.0f, FloorZ + StoryH - 7.0f)), FVector(SX + 22.0f, SY + 22.0f, 16.0f), HouseRot, FLinearColor::White, 1.0f, BaseCenter.Z + FloorZ);

		// 4 Structural reinforced concrete corner columns (pilares de concreto)
		for (int32 dx : { -1, 1 })
		{
			for (int32 dy : { -1, 1 })
			{
				const FVector ColLocal(dx * (SX * 0.5f - 8.0f), dy * (SY * 0.5f - 8.0f), FloorZ + StoryH * 0.5f);
				AddBox(ConcreteISM, ToWorld(ColLocal), FVector(26.0f, 26.0f, StoryH), HouseRot, FLinearColor::White, 1.0f, BaseCenter.Z + FloorZ);

				// Exposed vertical rusted rebar spires on top story (esperas de pilar)
				if (s == NumStories - 1)
				{
					const float RebarZ = FloorZ + StoryH + 52.0f;
					AddBox(MetalISM, ToWorld(FVector(ColLocal.X - 6.0f, ColLocal.Y - 6.0f, RebarZ)), FVector(4.2f, 4.2f, 105.0f), HouseRot);
					AddBox(MetalISM, ToWorld(FVector(ColLocal.X + 6.0f, ColLocal.Y + 6.0f, RebarZ)), FVector(4.2f, 4.2f, 92.0f), HouseRot);
				}
			}
		}

		// Ground floor doorway on front face (+X)
		if (s == 0)
		{
			const float DoorY = (SY > 340.0f) ? -SY * 0.22f : 0.0f;
			AddBox(DarkISM, ToWorld(FVector(SX * 0.5f + 1.5f, DoorY, FloorZ + 102.0f)), FVector(8.0f, 92.0f, 204.0f), HouseRot);
			AddBox(ConcreteISM, ToWorld(FVector(SX * 0.5f + 4.0f, DoorY, FloorZ + 210.0f)), FVector(16.0f, 108.0f, 14.0f), HouseRot);
			AddBox(WoodISM, ToWorld(FVector(SX * 0.5f + 3.0f, DoorY - 46.0f, FloorZ + 102.0f)), FVector(10.0f, 8.0f, 204.0f), HouseRot);
			AddBox(WoodISM, ToWorld(FVector(SX * 0.5f + 3.0f, DoorY + 46.0f, FloorZ + 102.0f)), FVector(10.0f, 8.0f, 204.0f), HouseRot);
		}

		// Front windows (+X face) with concrete sills and crooked wooden shutters
		const int32 NumWinFront = (SY > 380.0f) ? 2 : 1;
		for (int32 w = 0; w < NumWinFront; ++w)
		{
			const float WinY = (NumWinFront == 1)
				? ((s == 0 && SY > 340.0f) ? SY * 0.24f : 0.0f)
				: ((w == 0) ? -SY * 0.25f : SY * 0.25f);
			if (s == 0 && FMath::Abs(WinY) < 40.0f) continue; // Don't overlap center door

			const float WinZ = FloorZ + 158.0f;
			AddBox(DarkISM, ToWorld(FVector(SX * 0.5f + 1.5f, WinY, WinZ)), FVector(8.0f, 88.0f, 92.0f), HouseRot);
			AddBox(ConcreteISM, ToWorld(FVector(SX * 0.5f + 5.0f, WinY, WinZ - 50.0f)), FVector(18.0f, 102.0f, 10.0f), HouseRot);
			AddBox(WoodISM, ToWorld(FVector(SX * 0.5f + 5.0f, WinY - 52.0f, WinZ)), FVector(6.0f, 26.0f, 90.0f), HouseRot + FRotator(0.0f, -12.0f, 1.5f));
			AddBox(WoodISM, ToWorld(FVector(SX * 0.5f + 5.0f, WinY + 52.0f, WinZ)), FVector(6.0f, 26.0f, 90.0f), HouseRot + FRotator(0.0f, 14.0f, -1.5f));
		}

		// Side window (+Y or -Y face)
		for (int32 sideSign : { -1, 1 })
		{
			const float SideWinZ = FloorZ + 158.0f;
			AddBox(DarkISM, ToWorld(FVector(0.0f, sideSign * (SY * 0.5f + 1.5f), SideWinZ)), FVector(84.0f, 8.0f, 86.0f), HouseRot);
			AddBox(ConcreteISM, ToWorld(FVector(0.0f, sideSign * (SY * 0.5f + 5.0f), SideWinZ - 47.0f)), FVector(98.0f, 18.0f, 10.0f), HouseRot);
		}

		// Second-story cantilevered balcony with weathered wood & corrugated tin balustrade
		if (s == 1 && bHasBalcony)
		{
			const float BalWidth = SY * 0.65f;
			const float BalX = SX * 0.5f + 46.0f;
			AddBox(WoodISM, ToWorld(FVector(BalX, 0.0f, FloorZ + 4.0f)), FVector(92.0f, BalWidth, 10.0f), HouseRot);
			AddBox(MetalISM, ToWorld(FVector(BalX + 43.0f, 0.0f, FloorZ + 52.0f)), FVector(5.0f, BalWidth, 86.0f), HouseRot);
			AddBox(WoodISM, ToWorld(FVector(BalX + 43.0f, 0.0f, FloorZ + 96.0f)), FVector(10.0f, BalWidth + 8.0f, 7.0f), HouseRot);
		}
	}

	// Climbing wall ivy on front & side masonry
	if (IvyISM)
	{
		AddMesh(IvyISM, ToWorld(FVector(TopSX * 0.5f + 2.0f, -TopSY * 0.28f, 10.0f)), HouseRot, FVector(1.0f, 1.15f, 1.25f));
		if (Seed % 2 == 0)
		{
			AddMesh(IvyISM, ToWorld(FVector(0.0f, TopSY * 0.5f + 2.0f, 20.0f)), HouseRot + FRotator(0.0f, 90.0f, 0.0f), FVector(1.0f, 1.1f, 1.15f));
		}
	}

	// Foundation weeds along base of shanty
	if (WeedsISM)
	{
		for (int32 k = -1; k <= 1; ++k)
		{
			AddMesh(WeedsISM, ToWorld(FVector(SizeX * 0.5f + 18.0f, k * (SizeY * 0.34f), 0.0f)), FRotator(0.0f, Rng.FRandRange(0.0f, 360.0f), 0.0f), FVector(Rng.FRandRange(0.85f, 1.25f)));
		}
	}

	// Corrugated tin roof sheets + tires + cinder blocks + blue water tank on top slab
	const float TopZ = NumStories * StoryH;
	const float Pitch = Rng.FRandRange(5.5f, 10.5f) * ((Seed % 2 == 0) ? 1.0f : -1.0f);
	AddRoofCorrugated(ToWorld(FVector(0.0f, 0.0f, TopZ)), TopSX + 30.0f, TopSY + 30.0f, YawDeg, Pitch, Seed, bAddWaterTank);
}

void FFPSFavelaEnvironmentKit::AddDrainageGutter(const FVector& StartXY, const FVector& EndXY, float GroundZ, int32 Seed)
{
	const FVector Delta = EndXY - StartXY;
	const float Len = Delta.Size2D();
	if (Len < 50.0f) return;

	const FRotator Rot = Delta.Rotation();
	const FVector Mid = (StartXY + EndXY) * 0.5f;
	const FVector Right = FRotationMatrix(Rot).GetUnitAxis(EAxis::Y);

	// Recessed dark wet rainwater channel in the center (valeta)
	AddBox(PuddleISM, FVector(Mid.X, Mid.Y, GroundZ + 1.8f), FVector(Len, 36.0f, 3.5f), Rot);

	// Raised concrete curb lips on both sides of the sluice
	AddBox(ConcreteISM, FVector(Mid.X, Mid.Y, GroundZ + 4.5f) + Right * 26.0f, FVector(Len, 16.0f, 9.0f), Rot);
	AddBox(ConcreteISM, FVector(Mid.X, Mid.Y, GroundZ + 4.5f) - Right * 26.0f, FVector(Len, 16.0f, 9.0f), Rot);

	// Sprouting gutter weeds along the concrete curb seams
	const int32 NumWeeds = FMath::Max(4, FMath::RoundToInt(Len / 220.0f));
	AddWeedLine(StartXY + Right * 34.0f + FVector(0, 0, GroundZ), EndXY + Right * 34.0f + FVector(0, 0, GroundZ), NumWeeds, Seed, 18.0f);
}

void FFPSFavelaEnvironmentKit::AddPuddle(const FVector& CenterXY, float LengthX, float WidthY, float YawDeg, float GroundZ)
{
	AddBox(PuddleISM, FVector(CenterXY.X, CenterXY.Y, GroundZ + 1.4f), FVector(LengthX, WidthY, 2.5f), FRotator(0.0f, YawDeg, 0.0f));
}

FVector FFPSFavelaEnvironmentKit::AddUtilityPole(const FVector& BaseLoc, float HeightCm, float YawDeg, bool bWithTransformer)
{
	const FRotator Rot(0.0f, YawDeg, 0.0f);
	// Creosote wooden utility pole shaft + concrete base collar
	AddBox(ConcreteISM, BaseLoc + FVector(0.0f, 0.0f, 45.0f), FVector(28.0f, 28.0f, 90.0f), Rot);
	AddBox(WoodISM, BaseLoc + FVector(0.0f, 0.0f, HeightCm * 0.5f), FVector(20.0f, 20.0f, HeightCm), Rot);

	// Upper & lower wooden crossarms with ceramic insulators
	const float TopArmZ = HeightCm - 35.0f;
	const float LowArmZ = HeightCm - 95.0f;
	AddBox(WoodISM, BaseLoc + FVector(0.0f, 0.0f, TopArmZ), FVector(14.0f, 165.0f, 14.0f), Rot);
	AddBox(WoodISM, BaseLoc + FVector(0.0f, 0.0f, LowArmZ), FVector(14.0f, 145.0f, 14.0f), Rot);

	// Pole-mounted cylindrical transformer drum (transformador de poste)
	if (bWithTransformer)
	{
		const FVector TransLoc = BaseLoc + Rot.RotateVector(FVector(24.0f, 0.0f, HeightCm - 165.0f));
		AddBox(MetalISM, TransLoc, FVector(42.0f, 42.0f, 78.0f), Rot);
	}

	return BaseLoc + FVector(0.0f, 0.0f, TopArmZ);
}

void FFPSFavelaEnvironmentKit::AddCableSpan(const FVector& Start, const FVector& End, float SagCm, float PhaseRad, float AmpScale, float ThicknessScale)
{
	if (!CableISM) return;
	const FVector Delta = End - Start;
	const float Len = Delta.Size();
	if (Len < 40.0f) return;

	FVector2D Dir2D(Delta.X, Delta.Y);
	if (!Dir2D.Normalize())
	{
		Dir2D = FVector2D(1.0f, 0.0f);
	}
	const FVector Side(-Dir2D.Y, Dir2D.X, 0.0f);

	const FTransform Xf(Delta.Rotation(), Start, FVector(Len / 100.0f, ThicknessScale, ThicknessScale));
	const int32 Idx = CableISM->AddInstance(Xf, false);
	const TArray<float> Custom = { SagCm, PhaseRad, AmpScale, (float)Side.X, (float)Side.Y, 0.0f };
	CableISM->SetCustomData(Idx, Custom, false);
}

void FFPSFavelaEnvironmentKit::AddCableBundle(const FVector& A, const FVector& B, int32 NumStrands, float BaseSagCm, int32 Seed)
{
	FRandomStream Rng(Seed * 3571 + 9);
	const FVector Delta = B - A;
	FVector Right(-Delta.Y, Delta.X, 0.0f);
	if (!Right.Normalize())
	{
		Right = FVector(0.0f, 1.0f, 0.0f);
	}

	for (int32 i = 0; i < NumStrands; ++i)
	{
		const float Spread = (i - (NumStrands - 1) * 0.5f) * 24.0f;
		const FVector OffsetA = Right * (Spread + Rng.FRandRange(-8.0f, 8.0f)) + FVector(0.0f, 0.0f, Rng.FRandRange(-28.0f, 18.0f));
		const FVector OffsetB = Right * (Spread + Rng.FRandRange(-8.0f, 8.0f)) + FVector(0.0f, 0.0f, Rng.FRandRange(-28.0f, 18.0f));
		const float Sag = BaseSagCm * Rng.FRandRange(0.75f, 1.45f);
		const float Phase = Rng.FRandRange(0.0f, 6.283f);
		const float Amp = Rng.FRandRange(0.85f, 1.35f);
		const float Thick = Rng.FRandRange(1.25f, 1.65f);
		AddCableSpan(A + OffsetA, B + OffsetB, Sag, Phase, Amp, Thick);
	}
}

void FFPSFavelaEnvironmentKit::AddWeedLine(const FVector& Start, const FVector& End, int32 Count, int32 Seed, float SpreadCm)
{
	if (!WeedsISM || Count <= 0) return;
	FRandomStream Rng(Seed * 2749 + 5);
	for (int32 i = 0; i < Count; ++i)
	{
		const float Alpha = (Count == 1) ? 0.5f : (float)i / (float)(Count - 1);
		const FVector Pos = FMath::Lerp(Start, End, Alpha) + FVector(Rng.FRandRange(-SpreadCm, SpreadCm), Rng.FRandRange(-SpreadCm, SpreadCm), 0.0f);
		const float Scale = Rng.FRandRange(0.85f, 1.45f);
		const FLinearColor Tint(Rng.FRandRange(0.85f, 1.15f), Rng.FRandRange(0.90f, 1.15f), Rng.FRandRange(0.80f, 1.05f));
		AddMesh(WeedsISM, Pos, FRotator(0.0f, Rng.FRandRange(0.0f, 360.0f), 0.0f), FVector(Scale, Scale, Scale * Rng.FRandRange(0.9f, 1.25f)), Tint);
	}
}

void FFPSFavelaEnvironmentKit::AddBananaCluster(const FVector& BaseLoc, int32 Count, int32 Seed)
{
	if (!BananaISM) return;
	FRandomStream Rng(Seed * 6133 + 19);
	for (int32 i = 0; i < Count; ++i)
	{
		const FVector Offset(Rng.FRandRange(-65.0f, 65.0f), Rng.FRandRange(-65.0f, 65.0f), 0.0f);
		const float Scale = Rng.FRandRange(0.95f, 1.45f);
		const FLinearColor Tint(Rng.FRandRange(0.90f, 1.12f), Rng.FRandRange(0.92f, 1.12f), Rng.FRandRange(0.85f, 1.05f));
		AddMesh(BananaISM, BaseLoc + Offset, FRotator(Rng.FRandRange(-4.0f, 4.0f), Rng.FRandRange(0.0f, 360.0f), 0.0f), FVector(Scale), Tint);
	}
	if (WeedsISM)
	{
		for (int32 w = 0; w < 4; ++w)
		{
			AddMesh(WeedsISM, BaseLoc + FVector(Rng.FRandRange(-85.0f, 85.0f), Rng.FRandRange(-85.0f, 85.0f), 0.0f), FRotator(0.0f, Rng.FRandRange(0.0f, 360.0f), 0.0f), FVector(1.25f));
		}
	}
}

// =========================================================================================
// THEATER 0: FAVELA // ROCHA VELHA (60m x 60m Hillside Shanty Arena)
// =========================================================================================
void UFPSCoastalMaterialLibrary::PopulateRochaVelhaFavela(AActor* MapActor, USceneComponent* RootComp)
{
	if (!MapActor || !RootComp) return;

	FFPSFavelaEnvironmentKit Kit;
	Kit.Init(MapActor, RootComp);

	// 1. Layered Ground Detail: Cracked Asphalt Center Roadway, Red Clay Shoulders, Drainage Sluices & Puddles
	Kit.AddBox(Kit.AsphaltISM, FVector(-1200.0f, 0.0f, 1.2f), FVector(2200.0f, 520.0f, 2.5f));
	Kit.AddBox(Kit.DirtISM, FVector(-1400.0f, -720.0f, 1.0f), FVector(2400.0f, 360.0f, 2.0f));
	Kit.AddBox(Kit.DirtISM, FVector(-1400.0f, 780.0f, 1.0f), FVector(2400.0f, 360.0f, 2.0f));
	Kit.AddBox(Kit.DirtISM, FVector(1650.0f, 0.0f, 1.0f), FVector(1600.0f, 950.0f, 2.0f));

	// Dual open concrete stormwater drainage gutters (valetas) running along the main street
	Kit.AddDrainageGutter(FVector(-2300.0f, -310.0f, 0.0f), FVector(2300.0f, -310.0f, 0.0f), 0.0f, 101);
	Kit.AddDrainageGutter(FVector(-2300.0f, 440.0f, 0.0f), FVector(2300.0f, 440.0f, 0.0f), 0.0f, 102);

	// Sky-reflecting rainwater puddles in the plaza and alleyways
	Kit.AddPuddle(FVector(-1750.0f, 80.0f, 0.0f), 340.0f, 210.0f, 12.0f);
	Kit.AddPuddle(FVector(-1150.0f, -150.0f, 0.0f), 420.0f, 240.0f, -8.0f);
	Kit.AddPuddle(FVector(-550.0f, 210.0f, 0.0f), 380.0f, 195.0f, 18.0f);
	Kit.AddPuddle(FVector(650.0f, -120.0f, 0.0f), 440.0f, 250.0f, -14.0f);
	Kit.AddPuddle(FVector(1550.0f, 180.0f, 0.0f), 360.0f, 220.0f, 9.0f);

	// 2. Central 3-Story Favela Laje Watchtower Roof & Details (crowning the central tower at X=0, Y=0)
	Kit.AddRoofCorrugated(FVector(0.0f, 0.0f, 735.0f), 430.0f, 430.0f, 0.0f, 7.5f, 77, false);
	Kit.AddWaterTank(FVector(-85.0f, -85.0f, 735.0f), 0.0f, 901);
	Kit.AddWaterTank(FVector(85.0f, 85.0f, 735.0f), 90.0f, 902);

	// Corrugated tin awnings over the mid-level laje catwalks (Z = 365)
	Kit.AddRoofCorrugated(FVector(0.0f, -360.0f, 368.0f), 380.0f, 210.0f, 0.0f, -9.0f, 78, false);
	Kit.AddRoofCorrugated(FVector(0.0f, 360.0f, 368.0f), 380.0f, 210.0f, 0.0f, 9.0f, 79, false);

	// Blooming Bougainvillea flower vines cascading from central catwalk corners down the concrete pylons
	Kit.AddBougainvilleaCluster(FVector(-220.0f, -360.0f, 365.0f), FRotator(0.0f, 0.0f, 0.0f), FLinearColor(2.4f, 0.12f, 1.25f), 4);
	Kit.AddBougainvilleaCluster(FVector(220.0f, -360.0f, 365.0f), FRotator(0.0f, 0.0f, 0.0f), FLinearColor(2.2f, 0.75f, 0.15f), 4);
	Kit.AddBougainvilleaCluster(FVector(-220.0f, 360.0f, 365.0f), FRotator(0.0f, 180.0f, 0.0f), FLinearColor(1.6f, 0.10f, 1.95f), 4);
	Kit.AddBougainvilleaCluster(FVector(220.0f, 360.0f, 365.0f), FRotator(0.0f, 180.0f, 0.0f), FLinearColor(2.4f, 0.12f, 1.25f), 4);

	// Feira Livre Street Market Canopies (Vibrant Brazilian Yellow, Blue & Red Tarps)
	Kit.AddMarketCanopy(FVector(180.0f, -220.0f, 0.0f), FVector(360.0f, 220.0f, 220.0f), FRotator(0.0f, 15.0f, 0.0f), FLinearColor(0.98f, 0.82f, 0.05f), 910);
	Kit.AddMarketCanopy(FVector(-220.0f, 380.0f, 0.0f), FVector(320.0f, 200.0f, 210.0f), FRotator(0.0f, -20.0f, 0.0f), FLinearColor(0.08f, 0.45f, 0.95f), 911);
	Kit.AddMarketCanopy(FVector(520.0f, 150.0f, 0.0f), FVector(280.0f, 190.0f, 200.0f), FRotator(0.0f, 40.0f, 0.0f), FLinearColor(0.92f, 0.12f, 0.15f), 912);

	// Corrugated tin awnings & blue water tanks over West & East shanty blocks
	Kit.AddRoofCorrugated(FVector(-200.0f, -1200.0f, 272.0f), 610.0f, 260.0f, 0.0f, 8.0f, 81, false);
	Kit.AddWaterTank(FVector(-200.0f, -1200.0f, 262.0f), 0.0f, 920);
	Kit.AddBougainvilleaCluster(FVector(100.0f, -1200.0f, 260.0f), FRotator(0.0f, 90.0f, 0.0f), FLinearColor(2.4f, 0.12f, 1.25f), 4);
	Kit.AddBougainvilleaCluster(FVector(-500.0f, -1200.0f, 260.0f), FRotator(0.0f, -90.0f, 0.0f), FLinearColor(2.2f, 0.75f, 0.15f), 4);

	Kit.AddRoofCorrugated(FVector(200.0f, 1400.0f, 225.0f), 430.0f, 330.0f, 180.0f, 8.5f, 82, false);
	Kit.AddWaterTank(FVector(200.0f, 1400.0f, 215.0f), 0.0f, 925);
	Kit.AddBougainvilleaCluster(FVector(200.0f, 1550.0f, 210.0f), FRotator(0.0f, 0.0f, 0.0f), FLinearColor(1.6f, 0.10f, 1.95f), 5);
	Kit.AddBougainvilleaCluster(FVector(-600.0f, 1800.0f, 140.0f), FRotator(0.0f, 0.0f, 0.0f), FLinearColor(2.4f, 0.12f, 1.25f), 4);
	Kit.AddBougainvilleaCluster(FVector(600.0f, 1800.0f, 140.0f), FRotator(0.0f, 0.0f, 0.0f), FLinearColor(2.2f, 0.75f, 0.15f), 4);

	// 3. Multi-Story Favela Shanties flanking West Beco, East Beco, and Stepped North/South Hillsides
	// West Flank Shanties (facing +Y toward the central plaza)
	Kit.AddShanty(FVector(-1550.0f, -1550.0f, 0.0f), 460.0f, 480.0f, 78.0f, 2, 11, true, true, true);
	Kit.AddShanty(FVector(-100.0f, -1950.0f, 0.0f), 480.0f, 520.0f, 90.0f, 3, 12, true, true, true);
	Kit.AddShanty(FVector(1350.0f, -1550.0f, 0.0f), 460.0f, 480.0f, 102.0f, 2, 13, true, true, true);

	// East Flank Shanties (facing -Y toward the central plaza)
	Kit.AddShanty(FVector(-1450.0f, 1850.0f, 0.0f), 460.0f, 500.0f, -82.0f, 2, 21, true, true, true);
	Kit.AddShanty(FVector(0.0f, 2050.0f, 0.0f), 480.0f, 520.0f, -90.0f, 3, 22, true, true, true);
	Kit.AddShanty(FVector(1400.0f, 1850.0f, 0.0f), 460.0f, 500.0f, -98.0f, 2, 23, true, true, true);

	// Stepped Hillside Shanty Skyline along North Perimeter (X = +2650, directly in front of Alpha spawn view!)
	Kit.AddShanty(FVector(2680.0f, -2100.0f, 0.0f), 460.0f, 540.0f, 180.0f, 3, 31, true, true, true);
	Kit.AddShanty(FVector(2720.0f, -1350.0f, 0.0f), 440.0f, 500.0f, 175.0f, 2, 32, true, true, true);
	Kit.AddShanty(FVector(2720.0f, 1350.0f, 0.0f), 440.0f, 500.0f, -175.0f, 2, 33, true, true, true);
	Kit.AddShanty(FVector(2680.0f, 2100.0f, 0.0f), 460.0f, 540.0f, 180.0f, 3, 34, true, true, true);
	// Upper Hillside Tier behind North Wall (Z = +320cm hillside terrace)
	Kit.AddShanty(FVector(2880.0f, -650.0f, 280.0f), 440.0f, 540.0f, 180.0f, 2, 35, true, true, true);
	Kit.AddShanty(FVector(2880.0f, 0.0f, 340.0f), 460.0f, 560.0f, 180.0f, 3, 36, true, true, true);
	Kit.AddShanty(FVector(2880.0f, 650.0f, 280.0f), 440.0f, 540.0f, 180.0f, 2, 37, true, true, true);

	// Outer West & East Hillside Shanty Terraces
	Kit.AddShanty(FVector(-2200.0f, -2620.0f, 0.0f), 440.0f, 520.0f, 90.0f, 3, 41, true, true, true);
	Kit.AddShanty(FVector(-800.0f, -2680.0f, 0.0f), 440.0f, 520.0f, 90.0f, 2, 42, true, true, true);
	Kit.AddShanty(FVector(700.0f, -2680.0f, 0.0f), 440.0f, 520.0f, 90.0f, 3, 43, true, true, true);
	Kit.AddShanty(FVector(2050.0f, -2620.0f, 0.0f), 440.0f, 520.0f, 90.0f, 2, 44, true, true, true);

	Kit.AddShanty(FVector(-2200.0f, 2620.0f, 0.0f), 440.0f, 520.0f, -90.0f, 3, 51, true, true, true);
	Kit.AddShanty(FVector(-800.0f, 2680.0f, 0.0f), 440.0f, 520.0f, -90.0f, 2, 52, true, true, true);
	Kit.AddShanty(FVector(700.0f, 2680.0f, 0.0f), 440.0f, 520.0f, -90.0f, 3, 53, true, true, true);
	Kit.AddShanty(FVector(2050.0f, 2620.0f, 0.0f), 440.0f, 520.0f, -90.0f, 2, 54, true, true, true);

	// South Hillside Shanties (visible when looking South)
	Kit.AddShanty(FVector(-2720.0f, -1550.0f, 0.0f), 440.0f, 520.0f, 0.0f, 3, 61, true, true, true);
	Kit.AddShanty(FVector(-2720.0f, 1550.0f, 0.0f), 440.0f, 520.0f, 0.0f, 3, 62, true, true, true);
	Kit.AddShanty(FVector(-2880.0f, 0.0f, 290.0f), 440.0f, 560.0f, 0.0f, 2, 63, true, true, true);

	// 4. Tropical Urban Foliage: Banana Tree Clusters & Gutter Weeds
	Kit.AddBananaCluster(FVector(-1850.0f, -780.0f, 0.0f), 3, 201);
	Kit.AddBananaCluster(FVector(-1850.0f, 820.0f, 0.0f), 3, 202);
	Kit.AddBananaCluster(FVector(-620.0f, -920.0f, 0.0f), 3, 203);
	Kit.AddBananaCluster(FVector(-580.0f, 960.0f, 0.0f), 3, 204);
	Kit.AddBananaCluster(FVector(720.0f, -950.0f, 0.0f), 3, 205);
	Kit.AddBananaCluster(FVector(720.0f, 950.0f, 0.0f), 3, 206);
	Kit.AddBananaCluster(FVector(1950.0f, -850.0f, 0.0f), 4, 207);
	Kit.AddBananaCluster(FVector(1950.0f, 850.0f, 0.0f), 4, 208);

	// 5. Utility Poles, Transformers & Swaying Overhead Catenary Electrical Web ("Gatos")
	const FVector P_SW1 = Kit.AddUtilityPole(FVector(-1950.0f, -640.0f, 0.0f), 640.0f, 0.0f, true);
	const FVector P_SE1 = Kit.AddUtilityPole(FVector(-1950.0f, 680.0f, 0.0f), 640.0f, 0.0f, false);
	const FVector P_SW2 = Kit.AddUtilityPole(FVector(-920.0f, -680.0f, 0.0f), 660.0f, 8.0f, false);
	const FVector P_SE2 = Kit.AddUtilityPole(FVector(-920.0f, 720.0f, 0.0f), 660.0f, -8.0f, true);
	const FVector P_NW1 = Kit.AddUtilityPole(FVector(680.0f, -680.0f, 0.0f), 660.0f, -5.0f, true);
	const FVector P_NE1 = Kit.AddUtilityPole(FVector(680.0f, 720.0f, 0.0f), 660.0f, 5.0f, false);
	const FVector P_NW2 = Kit.AddUtilityPole(FVector(1850.0f, -640.0f, 0.0f), 640.0f, 0.0f, false);
	const FVector P_NE2 = Kit.AddUtilityPole(FVector(1850.0f, 680.0f, 0.0f), 640.0f, 0.0f, true);

	// Cross-street sagging catenary bundles (directly framing the sky in front of the player!)
	Kit.AddCableBundle(P_SW1, P_SE1, 5, 58.0f, 301);
	Kit.AddCableBundle(P_SW2, P_SE2, 6, 65.0f, 302);
	Kit.AddCableBundle(P_SW1, P_SE2, 4, 72.0f, 303);
	Kit.AddCableBundle(P_SE1, P_SW2, 4, 70.0f, 304);
	Kit.AddCableBundle(P_NW1, P_NE1, 5, 62.0f, 305);
	Kit.AddCableBundle(P_NW2, P_NE2, 5, 58.0f, 306);

	// Longitudinal trunk lines along both sides of the cobblestone street
	Kit.AddCableBundle(P_SW1, P_SW2, 4, 48.0f, 307);
	Kit.AddCableBundle(P_SE1, P_SE2, 4, 48.0f, 308);
	Kit.AddCableBundle(P_SW2, P_NW1, 4, 68.0f, 309);
	Kit.AddCableBundle(P_SE2, P_NE1, 4, 68.0f, 310);
	Kit.AddCableBundle(P_NW1, P_NW2, 4, 52.0f, 311);
	Kit.AddCableBundle(P_NE1, P_NE2, 4, 52.0f, 312);

	// Drop cables ("gatos") radiating from utility poles to the Central Laje Watchtower and flanking shanties
	Kit.AddCableBundle(P_SW2, FVector(-200.0f, -200.0f, 730.0f), 4, 52.0f, 313);
	Kit.AddCableBundle(P_SE2, FVector(-200.0f, 200.0f, 730.0f), 4, 52.0f, 314);
	Kit.AddCableBundle(P_NW1, FVector(200.0f, -200.0f, 730.0f), 4, 52.0f, 315);
	Kit.AddCableBundle(P_NE1, FVector(200.0f, 200.0f, 730.0f), 4, 52.0f, 316);
	Kit.AddCableBundle(P_SW1, FVector(-1400.0f, -1350.0f, 560.0f), 4, 45.0f, 317);
	Kit.AddCableBundle(P_SE1, FVector(-1400.0f, 1600.0f, 560.0f), 4, 45.0f, 318);
	Kit.AddCableBundle(P_SW2, FVector(-100.0f, -1700.0f, 840.0f), 4, 55.0f, 319);
	Kit.AddCableBundle(P_SE2, FVector(0.0f, 1800.0f, 840.0f), 4, 55.0f, 320);
}

// =========================================================================================
// THEATER 1: FAVELA // BAIA BRAVA (120m x 120m Coastal Hillside Favela & Waterfront Pier)
// =========================================================================================
void UFPSCoastalMaterialLibrary::PopulateBaiaBravaFavela(AActor* MapActor, USceneComponent* RootComp)
{
	if (!MapActor || !RootComp) return;

	FFPSFavelaEnvironmentKit Kit;
	Kit.Init(MapActor, RootComp);

	// 1. Cobblestone Waterfront Avenue, Cracked Asphalt Lanes, Drainage Sluices & Puddles (from Alpha Spawn Y=+4300 to Seawall Y=+100)
	Kit.AddBox(Kit.CobbleISM, FVector(0.0f, 2500.0f, 1.5f), FVector(1650.0f, 3800.0f, 3.0f));
	Kit.AddBox(Kit.AsphaltISM, FVector(0.0f, 2500.0f, 2.2f), FVector(620.0f, 3600.0f, 3.2f));
	Kit.AddBox(Kit.DirtISM, FVector(-1150.0f, 2500.0f, 1.2f), FVector(700.0f, 3600.0f, 2.4f));
	Kit.AddBox(Kit.DirtISM, FVector(1150.0f, 2500.0f, 1.2f), FVector(700.0f, 3600.0f, 2.4f));

	// Dual open concrete stormwater gutters running down the coastal avenue toward the seawall culvert
	Kit.AddDrainageGutter(FVector(-340.0f, 4350.0f, 0.0f), FVector(-340.0f, 350.0f, 0.0f), 0.0f, 401);
	Kit.AddDrainageGutter(FVector(340.0f, 4350.0f, 0.0f), FVector(340.0f, 350.0f, 0.0f), 0.0f, 402);

	// Rainwater puddles reflecting the coastal sky & shanties
	Kit.AddPuddle(FVector(120.0f, 3650.0f, 0.0f), 320.0f, 440.0f, 82.0f, 2.0f);
	Kit.AddPuddle(FVector(-140.0f, 2950.0f, 0.0f), 360.0f, 480.0f, 96.0f, 2.0f);
	Kit.AddPuddle(FVector(90.0f, 2100.0f, 0.0f), 340.0f, 420.0f, 88.0f, 2.0f);
	Kit.AddPuddle(FVector(-80.0f, 1250.0f, 0.0f), 380.0f, 460.0f, 92.0f, 2.0f);

	// 2. Coastal Favela Shanties flanking the Avenue (facing inward toward X=0 and toward the Alpha spawn camera view!)
	// East Side Shanty Row (X = +1050..+2400)
	Kit.AddShanty(FVector(1180.0f, 3350.0f, 0.0f), 460.0f, 520.0f, 165.0f, 2, 411, true, true, true);
	Kit.AddShanty(FVector(1220.0f, 2450.0f, 0.0f), 480.0f, 540.0f, 175.0f, 3, 412, true, true, true);
	Kit.AddShanty(FVector(1160.0f, 1550.0f, 0.0f), 460.0f, 520.0f, -172.0f, 2, 413, true, true, true);
	Kit.AddShanty(FVector(1200.0f, 700.0f, 0.0f), 480.0f, 540.0f, 180.0f, 3, 414, true, true, true);

	// West Side Shanty Row (X = -1050..-2400)
	Kit.AddShanty(FVector(-1180.0f, 3350.0f, 0.0f), 460.0f, 520.0f, 15.0f, 2, 421, true, true, true);
	Kit.AddShanty(FVector(-1220.0f, 2450.0f, 0.0f), 480.0f, 540.0f, 5.0f, 3, 422, true, true, true);
	Kit.AddShanty(FVector(-1160.0f, 1550.0f, 0.0f), 460.0f, 520.0f, -8.0f, 2, 423, true, true, true);
	Kit.AddShanty(FVector(-1200.0f, 700.0f, 0.0f), 480.0f, 540.0f, 0.0f, 3, 424, true, true, true);

	// Elevated Hillside Shanties on the Fore-Dune Crests (Z = +230cm)
	Kit.AddShanty(FVector(2150.0f, 2350.0f, 220.0f), 480.0f, 560.0f, 135.0f, 3, 431, true, true, true);
	Kit.AddShanty(FVector(-2150.0f, 2350.0f, 220.0f), 480.0f, 560.0f, 45.0f, 3, 432, true, true, true);
	Kit.AddShanty(FVector(2050.0f, 950.0f, 180.0f), 460.0f, 520.0f, 120.0f, 2, 433, true, true, true);
	Kit.AddShanty(FVector(-2050.0f, 950.0f, 180.0f), 460.0f, 520.0f, 60.0f, 2, 434, true, true, true);

	// Seawall Crest Shanties & Corrugated Bunker Roofs along the Dike Spine (Y = -500, Z = +420cm)
	Kit.AddRoofCorrugated(FVector(0.0f, -500.0f, 675.0f), 460.0f, 420.0f, 90.0f, 8.0f, 440, true);
	Kit.AddShanty(FVector(1150.0f, -500.0f, 415.0f), 420.0f, 480.0f, 90.0f, 2, 441, true, true, true);
	Kit.AddShanty(FVector(-1150.0f, -500.0f, 415.0f), 420.0f, 480.0f, 90.0f, 2, 442, true, true, true);
	Kit.AddShanty(FVector(2350.0f, -500.0f, 415.0f), 440.0f, 520.0f, 90.0f, 3, 443, true, true, true);
	Kit.AddShanty(FVector(-2350.0f, -500.0f, 415.0f), 440.0f, 520.0f, 90.0f, 3, 444, true, true, true);

	// Waterfront Pier Fisherman Shacks (overlooking the bay at Y = -2800..-4600, Z = +260cm)
	Kit.AddRoofCorrugated(FVector(-480.0f, -3100.0f, 510.0f), 380.0f, 320.0f, 0.0f, 9.0f, 445, true);
	Kit.AddRoofCorrugated(FVector(480.0f, -3800.0f, 510.0f), 380.0f, 320.0f, 180.0f, 9.0f, 446, false);

	// 3. Tropical Coastal Foliage: Banana Clusters & Weeds
	Kit.AddBananaCluster(FVector(680.0f, 3650.0f, 0.0f), 3, 501);
	Kit.AddBananaCluster(FVector(-680.0f, 3650.0f, 0.0f), 3, 502);
	Kit.AddBananaCluster(FVector(720.0f, 2750.0f, 0.0f), 4, 503);
	Kit.AddBananaCluster(FVector(-720.0f, 2750.0f, 0.0f), 4, 504);
	Kit.AddBananaCluster(FVector(700.0f, 1850.0f, 0.0f), 3, 505);
	Kit.AddBananaCluster(FVector(-700.0f, 1850.0f, 0.0f), 3, 506);
	Kit.AddBananaCluster(FVector(680.0f, 950.0f, 0.0f), 4, 507);
	Kit.AddBananaCluster(FVector(-680.0f, 950.0f, 0.0f), 4, 508);

	// Vibrant Feira Livre Market Canopies along the Coastal Avenue
	Kit.AddMarketCanopy(FVector(560.0f, 3200.0f, 0.0f), FVector(240.0f, 180.0f, 220.0f), FRotator(0.0f, -85.0f, 0.0f), FLinearColor(0.95f, 0.72f, 0.08f, 1.0f), 511);
	Kit.AddMarketCanopy(FVector(-560.0f, 2200.0f, 0.0f), FVector(260.0f, 190.0f, 220.0f), FRotator(0.0f, 95.0f, 0.0f), FLinearColor(0.08f, 0.42f, 0.88f, 1.0f), 512);
	Kit.AddMarketCanopy(FVector(580.0f, 1200.0f, 0.0f), FVector(250.0f, 185.0f, 220.0f), FRotator(0.0f, -90.0f, 0.0f), FLinearColor(0.88f, 0.16f, 0.18f, 1.0f), 513);

	// Cascading Magenta Bougainvillea Clusters framing shanty roofs & seawall retaining walls
	Kit.AddBougainvilleaCluster(FVector(960.0f, 3350.0f, 270.0f), FRotator(-15.0f, 180.0f, 0.0f), FLinearColor(2.4f, 0.12f, 1.25f, 1.0f), 28);
	Kit.AddBougainvilleaCluster(FVector(-960.0f, 2450.0f, 270.0f), FRotator(-15.0f, 0.0f, 0.0f), FLinearColor(1.6f, 0.10f, 1.95f, 1.0f), 32);
	Kit.AddBougainvilleaCluster(FVector(960.0f, 1550.0f, 270.0f), FRotator(-15.0f, 180.0f, 0.0f), FLinearColor(2.4f, 0.12f, 1.25f, 1.0f), 26);
	Kit.AddBougainvilleaCluster(FVector(720.0f, -480.0f, 430.0f), FRotator(-20.0f, 90.0f, 0.0f), FLinearColor(2.4f, 0.12f, 1.25f, 1.0f), 35);
	Kit.AddBougainvilleaCluster(FVector(-720.0f, -480.0f, 430.0f), FRotator(-20.0f, 90.0f, 0.0f), FLinearColor(1.6f, 0.10f, 1.95f, 1.0f), 35);

	// 4. Utility Poles & Swaying Overhead Catenary Electrical Web along the Coastal Avenue
	const FVector CP_E1 = Kit.AddUtilityPole(FVector(580.0f, 3750.0f, 0.0f), 640.0f, 90.0f, true);
	const FVector CP_W1 = Kit.AddUtilityPole(FVector(-580.0f, 3750.0f, 0.0f), 640.0f, 90.0f, false);
	const FVector CP_E2 = Kit.AddUtilityPole(FVector(620.0f, 2650.0f, 0.0f), 660.0f, 85.0f, false);
	const FVector CP_W2 = Kit.AddUtilityPole(FVector(-620.0f, 2650.0f, 0.0f), 660.0f, 95.0f, true);
	const FVector CP_E3 = Kit.AddUtilityPole(FVector(600.0f, 1450.0f, 0.0f), 660.0f, 90.0f, true);
	const FVector CP_W3 = Kit.AddUtilityPole(FVector(-600.0f, 1450.0f, 0.0f), 660.0f, 90.0f, false);
	const FVector CP_E4 = Kit.AddUtilityPole(FVector(580.0f, 250.0f, 0.0f), 680.0f, 90.0f, false);
	const FVector CP_W4 = Kit.AddUtilityPole(FVector(-580.0f, 250.0f, 0.0f), 680.0f, 90.0f, true);

	// Cross-avenue sagging catenary cable bundles (framing the player's view toward the seawall & bay!)
	Kit.AddCableBundle(CP_E1, CP_W1, 5, 56.0f, 601);
	Kit.AddCableBundle(CP_E2, CP_W2, 6, 64.0f, 602);
	Kit.AddCableBundle(CP_E1, CP_W2, 4, 70.0f, 603);
	Kit.AddCableBundle(CP_W1, CP_E2, 4, 70.0f, 604);
	Kit.AddCableBundle(CP_E3, CP_W3, 5, 62.0f, 605);
	Kit.AddCableBundle(CP_E4, CP_W4, 5, 60.0f, 606);

	// Longitudinal trunk lines & shanty rooftop service drops
	Kit.AddCableBundle(CP_E1, CP_E2, 4, 50.0f, 607);
	Kit.AddCableBundle(CP_W1, CP_W2, 4, 50.0f, 608);
	Kit.AddCableBundle(CP_E2, CP_E3, 4, 52.0f, 609);
	Kit.AddCableBundle(CP_W2, CP_W3, 4, 52.0f, 610);
	Kit.AddCableBundle(CP_E3, CP_E4, 4, 54.0f, 611);
	Kit.AddCableBundle(CP_W3, CP_W4, 4, 54.0f, 612);
	Kit.AddCableBundle(CP_E4, FVector(950.0f, -500.0f, 980.0f), 4, 58.0f, 613);
	Kit.AddCableBundle(CP_W4, FVector(-950.0f, -500.0f, 980.0f), 4, 58.0f, 614);
}

