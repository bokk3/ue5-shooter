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

UMaterialInterface* UFPSCoastalMaterialLibrary::GetOrCreateCoastalMaterial(UObject* WorldContext, ECoastalSurfaceType SurfaceType, FVector2D WindDirection)
{
	const uint32 Key = MakeCacheKey(SurfaceType, WindDirection);
	if (TObjectPtr<UMaterialInterface>* Found = GCoastalMaterialCache.Find(Key))
	{
		if (Found->Get()) return Found->Get();
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

	// Universal vector parameters
	MID->SetVectorParameterValue(TEXT("Color"), Albedo);
	MID->SetVectorParameterValue(TEXT("BaseColor"), Albedo);
	MID->SetVectorParameterValue(TEXT("Albedo"), Albedo);
	MID->SetVectorParameterValue(TEXT("DiffuseColor"), Albedo);

	// Universal scalar parameters
	MID->SetScalarParameterValue(TEXT("Roughness"), Roughness);
	MID->SetScalarParameterValue(TEXT("RoughnessBase"), Roughness);
	MID->SetScalarParameterValue(TEXT("Metallic"), Metallic);
	MID->SetScalarParameterValue(TEXT("Specular"), Specular);

	// Texture parameters
	if (BaseColorTex)
	{
		MID->SetTextureParameterValue(TEXT("Texture"), BaseColorTex);
		MID->SetTextureParameterValue(TEXT("BaseColorTexture"), BaseColorTex);
		MID->SetTextureParameterValue(TEXT("BaseColorMap"), BaseColorTex);
		MID->SetTextureParameterValue(TEXT("Diffuse"), BaseColorTex);
	}
	if (NormalTex)
	{
		MID->SetTextureParameterValue(TEXT("NormalMap"), NormalTex);
		MID->SetTextureParameterValue(TEXT("Normal"), NormalTex);
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
		PhysTag = FName(TEXT("Sand"));
		break;
	case ECoastalSurfaceType::WeatheredPierTimber:
		PhysTag = FName(TEXT("Wood"));
		break;
	case ECoastalSurfaceType::MarineCastConcrete:
	case ECoastalSurfaceType::WeatheredAsphalt:
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
