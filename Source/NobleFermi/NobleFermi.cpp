#include "NobleFermi.h"
#include "Modules/ModuleManager.h"
#include "Lyra/AbilitySystem/FPSGameplayTags.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UObjectArray.h"
#include "UObject/Package.h"
#include "UObject/Class.h"
#include "Misc/CoreDelegates.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformTime.h"

#if PLATFORM_WINDOWS
#include <atomic>
#include <thread>
#include <mutex>
#include <vector>
#include <cmath>

#include "Windows/AllowWindowsPlatformTypes.h"
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "winmm.lib")
#include "Windows/HideWindowsPlatformTypes.h"

namespace NobleFermiSplashProgress
{
	static std::atomic<bool> GStopThread{ false };
	static std::atomic<bool> GWorldLoaded{ false };
	static std::atomic<int32> GPackagesLoaded{ 0 };
	static std::atomic<int32> GAssetsLoaded{ 0 };
	static double GStartTimeSeconds = 0.0;

	static std::mutex GTextMutex;
	static FString GLatestItemName = TEXT("Core Engine Subsystems");
	static FString GCurrentPhaseName = TEXT("INITIALIZING ENGINE SUBSYSTEMS");
	static std::thread GSplashThread;

	static void SetLatestLoadingItem(const FString& InItem, const FString& InPhase = FString())
	{
		std::lock_guard<std::mutex> Lock(GTextMutex);
		if (!InItem.IsEmpty())
		{
			GLatestItemName = InItem;
		}
		if (!InPhase.IsEmpty())
		{
			GCurrentPhaseName = InPhase;
		}
	}

	static void OnPackageCreated(UPackage* Pkg)
	{
		const int32 Count = ++GPackagesLoaded;
		if (Pkg)
		{
			FString PkgName = Pkg->GetName();
			if (PkgName.Len() > 48)
			{
				PkgName = TEXT("...") + PkgName.Right(45);
			}
			FString Phase = TEXT("STREAMING CONTENT PACKAGES");
			if (PkgName.Contains(TEXT("/Characters/")))
			{
				Phase = TEXT("LOADING OPERATOR SKELETAL RIG & PBR TEXTURES");
			}
			else if (PkgName.Contains(TEXT("/Weapons/")))
			{
				Phase = TEXT("LOADING TACTICAL ARSENAL & BALLISTICS DATA");
			}
			else if (PkgName.Contains(TEXT("/Environment/")) || PkgName.Contains(TEXT("/Maps/")))
			{
				Phase = TEXT("CONSTRUCTING 3D HANGAR DIORAMA & LIGHTING");
			}
			SetLatestLoadingItem(PkgName, Phase);
		}
	}

	static void OnAssetLoaded(UObject* AssetObj)
	{
		++GAssetsLoaded;
		if (AssetObj)
		{
			const FString ClassName = AssetObj->GetClass() ? AssetObj->GetClass()->GetName() : TEXT("Asset");
			FString ObjPath = AssetObj->GetPathName();
			int32 DotIdx = INDEX_NONE;
			if (ObjPath.FindLastChar(TEXT('.'), DotIdx))
			{
				ObjPath = ObjPath.Left(DotIdx);
			}
			if (ObjPath.Len() > 44)
			{
				ObjPath = TEXT("...") + ObjPath.Right(41);
			}
			SetLatestLoadingItem(FString::Printf(TEXT("%s: %s"), *ClassName, *ObjPath));
		}
	}

	static void OnPreLoadMap(const FString& MapName)
	{
		SetLatestLoadingItem(MapName, TEXT("CONSTRUCTING HANGAR DIORAMA & LIGHTING"));
	}

	static void OnPostLoadMap(UWorld* LoadedWorld)
	{
		GWorldLoaded = true;
		SetLatestLoadingItem(TEXT("MenuMap // Hangar Diorama Ready"), TEXT("DEPLOYMENT READY"));
	}

	struct FSplashEnumCtx
	{
		DWORD TargetPid;
		HWND FoundHwnd;
	};

	static BOOL CALLBACK EnumSplashWndProc(HWND hWnd, LPARAM lParam)
	{
		FSplashEnumCtx* Ctx = reinterpret_cast<FSplashEnumCtx*>(lParam);
		DWORD WndPid = 0;
		GetWindowThreadProcessId(hWnd, &WndPid);
		if (WndPid == Ctx->TargetPid)
		{
			wchar_t ClassName[64] = { 0 };
			if (GetClassNameW(hWnd, ClassName, 63) > 0)
			{
				if (wcscmp(ClassName, L"SplashScreenClass") == 0)
				{
					Ctx->FoundHwnd = hWnd;
					return 0; // stop enumeration
				}
			}
		}
		return 1; // continue enumeration
	}

	static HWND FindProcessSplashWindow()
	{
		FSplashEnumCtx Ctx{ GetCurrentProcessId(), NULL };
		EnumWindows(&EnumSplashWndProc, reinterpret_cast<LPARAM>(&Ctx));
		return Ctx.FoundHwnd;
	}

	/** Synthesize in-memory Web Audio track matching LaserCutLogo.tsx:
	 *  - 0.0s - 2.4s: Laser cut buzz (sawtooth pitch ramp 440Hz -> 880Hz -> 620Hz + spark crackle)
	 *  - 2.4s - 2.8s: Thermal flash snap pop (triangle 950Hz -> 120Hz) + thermal hiss dissipation
	 *  - 2.5s - 3.8s: Sub-bass surge (65Hz -> 140Hz -> 45Hz) + resonant noise whoosh + crystalline chime
	 */
	static std::vector<uint8> BuildTachyonSplashAudioBuffer()
	{
		constexpr int32 SampleRate = 44100;
		constexpr double Duration = 4.2;
		const int32 NumSamples = static_cast<int32>(SampleRate * Duration);
		const int32 DataSize = NumSamples * 2; // 16-bit mono

		std::vector<uint8> Wav;
		Wav.resize(44 + DataSize);

		// 44-byte standard RIFF header
		memcpy(&Wav[0], "RIFF", 4);
		uint32 ChunkSize = 36 + DataSize;
		memcpy(&Wav[4], &ChunkSize, 4);
		memcpy(&Wav[8], "WAVEfmt ", 8);
		uint32 Subchunk1Size = 16;
		memcpy(&Wav[16], &Subchunk1Size, 4);
		uint16 AudioFormat = 1; // PCM
		memcpy(&Wav[20], &AudioFormat, 2);
		uint16 NumChannels = 1; // Mono
		memcpy(&Wav[22], &NumChannels, 2);
		uint32 SR = SampleRate;
		memcpy(&Wav[24], &SR, 4);
		uint32 ByteRate = SampleRate * 2;
		memcpy(&Wav[28], &ByteRate, 4);
		uint16 BlockAlign = 2;
		memcpy(&Wav[32], &BlockAlign, 2);
		uint16 BitsPerSample = 16;
		memcpy(&Wav[34], &BitsPerSample, 2);
		memcpy(&Wav[36], "data", 4);
		uint32 Subchunk2Size = DataSize;
		memcpy(&Wav[40], &Subchunk2Size, 4);

		int16* Samples = reinterpret_cast<int16*>(&Wav[44]);

		double CutPhase = 0.0;
		double PopPhase = 0.0;
		double SubPhase = 0.0;
		double ChimePhase = 0.0;
		double FilterState = 0.0;
		double HissFilterState = 0.0;

		for (int32 i = 0; i < NumSamples; ++i)
		{
			const double t = static_cast<double>(i) / SampleRate;
			double Mixed = 0.0;

			// 1. Stage 1: Laser Cut Sawtooth Pitch Sweep (0.0s - 2.4s)
			if (t < 2.45)
			{
				double CutFreq = (t < 1.2) ? (440.0 + (t / 1.2) * 440.0) : (880.0 - ((t - 1.2) / 1.25) * 260.0);
				CutPhase += (2.0 * PI * CutFreq) / SampleRate;
				double Saw = 2.0 * ((CutPhase / (2.0 * PI)) - std::floor((CutPhase / (2.0 * PI)) + 0.5));
				double CutEnv = (t < 0.1) ? (t / 0.1) : ((t < 2.2) ? 1.0 : FMath::Max(0.0, 1.0 - (t - 2.2) / 0.25));
				double RawCrack = (static_cast<double>(rand()) / RAND_MAX) * 2.0 - 1.0;
				Mixed += (Saw * 0.16 + RawCrack * 0.02) * CutEnv;
			}

			// 2. Stage 2: Thermal Flash Snap Pop & Sizzle (2.4s - 2.8s)
			if (t >= 2.40 && t < 2.80)
			{
				const double pt = t - 2.40;
				double PopFreq = FMath::Max(100.0, 950.0 * std::exp(-pt / 0.045));
				PopPhase += (2.0 * PI * PopFreq) / SampleRate;
				double PopSaw = 2.0 * ((PopPhase / (2.0 * PI)) - std::floor((PopPhase / (2.0 * PI)) + 0.5));
				double PopTri = 2.0 * std::abs(PopSaw) - 1.0;
				double PopEnv = FMath::Max(0.0, 1.0 - pt / 0.20) * 0.40;

				double RawWhite = (static_cast<double>(rand()) / RAND_MAX) * 2.0 - 1.0;
				double HissAlpha = (2.0 * PI * 2800.0) / (SampleRate + 2.0 * PI * 2800.0);
				HissFilterState += HissAlpha * (RawWhite - HissFilterState);
				double HighPass = RawWhite - HissFilterState;
				double HissEnv = FMath::Max(0.0, 1.0 - pt / 0.38) * 0.14;

				Mixed += PopTri * PopEnv + HighPass * HissEnv;
			}

			// 3. Stage 3 & 4: Sub-Bass Surge, Resonant Whoosh & Crystal Sheen Chime (2.5s - 3.8s)
			if (t >= 2.50 && t < 3.80)
			{
				const double st = t - 2.50;

				// Sub-bass surge (65Hz -> 140Hz -> 45Hz)
				double SubFreq = (st < 0.35) ? (65.0 + (st / 0.35) * 75.0) : (140.0 - ((st - 0.35) / 0.65) * 95.0);
				SubFreq = FMath::Max(35.0, SubFreq);
				SubPhase += (2.0 * PI * SubFreq) / SampleRate;
				double SubEnv = (st < 0.35) ? (st / 0.35) : FMath::Max(0.0, 1.0 - (st - 0.35) / 0.65);
				double SubSample = std::sin(SubPhase) * SubEnv * 0.50;

				// Filtered resonant noise whoosh (220Hz -> 1200Hz -> 180Hz)
				double RawNoise = (static_cast<double>(rand()) / RAND_MAX) * 2.0 - 1.0;
				double FilterFreq = (st < 0.35) ? (220.0 + (st / 0.35) * 980.0) : (1200.0 - ((st - 0.35) / 0.65) * 1020.0);
				double Alpha = (2.0 * PI * FilterFreq) / (SampleRate + 2.0 * PI * FilterFreq);
				FilterState += Alpha * (RawNoise - FilterState);
				double NoiseEnv = (st < 0.35) ? (st / 0.35) : FMath::Max(0.0, 1.0 - (st - 0.35) / 0.60);
				double NoiseSample = FilterState * NoiseEnv * 0.32;

				// Crystalline flare chime (1650Hz -> 2400Hz -> 1200Hz)
				double ChimeSample = 0.0;
				if (st >= 0.28 && st <= 0.85)
				{
					const double ct = st - 0.28;
					double ChimeFreq = (ct < 0.15) ? (1650.0 + (ct / 0.15) * 750.0) : (2400.0 - ((ct - 0.15) / 0.42) * 1200.0);
					ChimePhase += (2.0 * PI * ChimeFreq) / SampleRate;
					double CSaw = 2.0 * ((ChimePhase / (2.0 * PI)) - std::floor((ChimePhase / (2.0 * PI)) + 0.5));
					double CTri = 2.0 * std::abs(CSaw) - 1.0;
					double ChimeEnv = (ct < 0.10) ? (ct / 0.10) : FMath::Max(0.0, 1.0 - (ct - 0.10) / 0.47);
					ChimeSample = CTri * ChimeEnv * 0.22;
				}

				Mixed += SubSample + NoiseSample + ChimeSample;
			}

			Mixed = FMath::Clamp(Mixed, -0.96, 0.96);
			Samples[i] = static_cast<int16>(Mixed * 32767.0);
		}

		return Wav;
	}

	static FString FindTachyonLogoBgraPath()
	{
		const TArray<FString> Candidates = {
			FPaths::ProjectContentDir() / TEXT("Splash/tachyon_logo_840x470.bgra"),
			FPaths::ProjectDir() / TEXT("Content/Splash/tachyon_logo_840x470.bgra"),
			FPaths::RootDir() / TEXT("Content/Splash/tachyon_logo_840x470.bgra"),
			FPaths::RootDir() / TEXT("NobleFermi/Content/Splash/tachyon_logo_840x470.bgra"),
			FPaths::LaunchDir() / TEXT("Content/Splash/tachyon_logo_840x470.bgra"),
			FPaths::LaunchDir() / TEXT("../../Content/Splash/tachyon_logo_840x470.bgra"),
			FPaths::LaunchDir() / TEXT("../../../Content/Splash/tachyon_logo_840x470.bgra"),
			TEXT("Content/Splash/tachyon_logo_840x470.bgra")
		};
		for (const FString& Path : Candidates)
		{
			if (FPaths::FileExists(Path))
			{
				return FPaths::ConvertRelativePathToFull(Path);
			}
		}
		return FString();
	}

	struct FDustMote
	{
		float X, Y;
		float DriftX, FallDist;
		float Size;
		float Alpha;
		float Duration;
		float Delay;
		uint8 R, G, B;
	};

	inline void BlendPixelAdd(uint32* Target, int32 W, int32 H, int32 X, int32 Y, uint8 R, uint8 G, uint8 B, uint8 A)
	{
		if (X < 0 || X >= W || Y < 0 || Y >= H || A == 0) return;
		int32 Idx = Y * W + X;
		uint32 Dst = Target[Idx];
		uint32 DstB = (Dst & 0xFF);
		uint32 DstG = ((Dst >> 8) & 0xFF);
		uint32 DstR = ((Dst >> 16) & 0xFF);
		uint32 DstA = ((Dst >> 24) & 0xFF);

		uint32 SrcPremR = (static_cast<uint32>(R) * A) / 255;
		uint32 SrcPremG = (static_cast<uint32>(G) * A) / 255;
		uint32 SrcPremB = (static_cast<uint32>(B) * A) / 255;

		uint32 OutR = FMath::Min(255u, DstR + SrcPremR);
		uint32 OutG = FMath::Min(255u, DstG + SrcPremG);
		uint32 OutB = FMath::Min(255u, DstB + SrcPremB);
		uint32 OutA = FMath::Min(255u, DstA + A - (DstA * A) / 255);

		// Guarantee premultiplied alpha safety: R, G, B <= A
		OutR = FMath::Min(OutR, OutA);
		OutG = FMath::Min(OutG, OutA);
		OutB = FMath::Min(OutB, OutA);

		Target[Idx] = (OutA << 24) | (OutR << 16) | (OutG << 8) | OutB;
	}

	inline void DrawGlowCircle(uint32* Target, int32 W, int32 H, float CX, float CY, float Radius, uint8 R, uint8 G, uint8 B, float MaxAlpha = 1.0f)
	{
		int32 MinX = FMath::Clamp(static_cast<int32>(CX - Radius), 0, W - 1);
		int32 MaxX = FMath::Clamp(static_cast<int32>(CX + Radius + 1), 0, W - 1);
		int32 MinY = FMath::Clamp(static_cast<int32>(CY - Radius), 0, H - 1);
		int32 MaxY = FMath::Clamp(static_cast<int32>(CY + Radius + 1), 0, H - 1);
		float RadSq = Radius * Radius;

		for (int32 Y = MinY; Y <= MaxY; ++Y)
		{
			for (int32 X = MinX; X <= MaxX; ++X)
			{
				float DistSq = (X - CX) * (X - CX) + (Y - CY) * (Y - CY);
				if (DistSq < RadSq)
				{
					float Dist = FMath::Sqrt(DistSq);
					float Falloff = 1.0f - (Dist / Radius);
					Falloff = Falloff * Falloff;
					uint8 A = static_cast<uint8>(FMath::Clamp(Falloff * MaxAlpha * 255.0f, 0.0f, 255.0f));
					if (A > 0)
					{
						BlendPixelAdd(Target, W, H, X, Y, R, G, B, A);
					}
				}
			}
		}
	}

	inline void DrawLineAA(uint32* Target, int32 W, int32 H, float X0, float Y0, float X1, float Y1, float Thickness, uint8 R, uint8 G, uint8 B, float Alpha = 1.0f)
	{
		float DX = X1 - X0;
		float DY = Y1 - Y0;
		float Len = FMath::Sqrt(DX * DX + DY * DY);
		if (Len < 0.001f) return;
		int32 Steps = static_cast<int32>(Len * 2.0f) + 1;
		float HalfThick = FMath::Max(0.75f, Thickness * 0.5f);

		for (int32 i = 0; i <= Steps; ++i)
		{
			float t = static_cast<float>(i) / static_cast<float>(Steps);
			float CurX = X0 + DX * t;
			float CurY = Y0 + DY * t;
			DrawGlowCircle(Target, W, H, CurX, CurY, HalfThick, R, G, B, Alpha);
		}
	}

	/** Plays the authentic Tachyon Studio hero logo laser cutting animation in a transparent borderless layered window */
	static void RunTachyonTransparentSplash(HWND hUnderlyingSplash)
	{
		if (FParse::Param(FCommandLine::Get(), TEXT("NoTachyon")))
		{
			return;
		}

		// Hide underlying UE splash during Tachyon brand animation
		if (hUnderlyingSplash && IsWindow(hUnderlyingSplash))
		{
			ShowWindow(hUnderlyingSplash, SW_HIDE);
		}

		// Register transparent window class
		WNDCLASSEXW Wc = { sizeof(WNDCLASSEXW) };
		Wc.lpfnWndProc = &DefWindowProcW;
		Wc.hInstance = GetModuleHandle(NULL);
		Wc.lpszClassName = L"TachyonStudioTransparentSplashClass";
		RegisterClassExW(&Wc);

		constexpr int32 WinW = 840;
		constexpr int32 WinH = 470;
		const int32 ScreenW = GetSystemMetrics(SM_CXSCREEN);
		const int32 ScreenH = GetSystemMetrics(SM_CYSCREEN);
		const int32 WinX = (ScreenW - WinW) / 2;
		const int32 WinY = (ScreenH - WinH) / 2;

		HWND hWnd = CreateWindowExW(
			WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
			L"TachyonStudioTransparentSplashClass",
			L"Tachyon Game Studio",
			WS_POPUP | WS_VISIBLE,
			WinX, WinY, WinW, WinH,
			NULL, NULL, GetModuleHandle(NULL), NULL);

		if (!hWnd)
		{
			return;
		}

		HDC hdcScreen = GetDC(NULL);
		HDC hdcMem = CreateCompatibleDC(hdcScreen);

		BITMAPINFO bmi = { 0 };
		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth = WinW;
		bmi.bmiHeader.biHeight = -WinH; // Top-down DIB
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 32;
		bmi.bmiHeader.biCompression = BI_RGB;

		void* pBits = nullptr;
		HBITMAP hBitmap = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
		HGDIOBJ hOldBmp = SelectObject(hdcMem, hBitmap);
		uint32* TargetPixels = reinterpret_cast<uint32*>(pBits);

		// Load pre-converted 32-bit premultiplied BGRA Tachyon branding logo
		TArray<uint8> LogoRawBytes;
		const FString LogoPath = FindTachyonLogoBgraPath();
		if (!LogoPath.IsEmpty())
		{
			FFileHelper::LoadFileToArray(LogoRawBytes, *LogoPath);
		}
		const uint32* LogoPixels = (LogoRawBytes.Num() >= WinW * WinH * 4) ? reinterpret_cast<const uint32*>(LogoRawBytes.GetData()) : nullptr;

		// Prepare unified soundtrack (Laser cut buzz + thermal snap/hiss + sub-bass/whoosh/chime)
		std::vector<uint8> SplashWav = BuildTachyonSplashAudioBuffer();
		if (SplashWav.size() > 44)
		{
			PlaySoundW(reinterpret_cast<LPCWSTR>(SplashWav.data()), NULL, SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
		}

		// Faceted border points (1000x560 scaled to WinW x WinH)
		const float ScaleX = static_cast<float>(WinW) / 1000.0f;
		const float ScaleY = static_cast<float>(WinH) / 560.0f;

		const FVector2f PolyPoints[] = {
			FVector2f(60.0f * ScaleX,  20.0f * ScaleY),
			FVector2f(940.0f * ScaleX, 20.0f * ScaleY),
			FVector2f(980.0f * ScaleX, 60.0f * ScaleY),
			FVector2f(980.0f * ScaleX, 500.0f * ScaleY),
			FVector2f(940.0f * ScaleX, 540.0f * ScaleY),
			FVector2f(60.0f * ScaleX,  540.0f * ScaleY),
			FVector2f(20.0f * ScaleX,  500.0f * ScaleY),
			FVector2f(20.0f * ScaleX,  60.0f * ScaleY),
			FVector2f(60.0f * ScaleX,  20.0f * ScaleY)
		};
		constexpr int32 NumPts = sizeof(PolyPoints) / sizeof(PolyPoints[0]);

		float SegLengths[NumPts - 1];
		float TotalPerimeter = 0.0f;
		for (int32 i = 0; i < NumPts - 1; ++i)
		{
			const float dx = PolyPoints[i + 1].X - PolyPoints[i].X;
			const float dy = PolyPoints[i + 1].Y - PolyPoints[i].Y;
			SegLengths[i] = std::sqrt(dx * dx + dy * dy);
			TotalPerimeter += SegLengths[i];
		}

		// Generate 36 atmospheric dust & debris particles (matching LaserCutLogo.tsx palette)
		TArray<FDustMote> DustMotes;
		const uint8 DustCols[][3] = {
			{ 0, 243, 255 },   // Cyan #00F3FF
			{ 255, 184, 0 },   // Amber #FFB800
			{ 255, 0, 255 },   // Magenta #FF00FF
			{ 255, 255, 255 }, // White #FFFFFF
			{ 255, 102, 0 },   // Orange #FF6600
			{ 0, 255, 178 }    // Mint #00FFB2
		};
		constexpr int32 NumDustColors = 6;
		constexpr int32 NumDustMotes = 36;
		DustMotes.Reserve(NumDustMotes);

		for (int32 i = 0; i < NumDustMotes; ++i)
		{
			FDustMote Mote;
			Mote.X = (0.06f + (static_cast<float>(rand() % 1000) / 1000.0f) * 0.88f) * WinW;
			Mote.Y = (0.15f + (static_cast<float>(rand() % 1000) / 1000.0f) * 0.65f) * WinH;
			Mote.Size = 1.2f + (static_cast<float>(rand() % 25) / 10.0f);
			Mote.DriftX = ((static_cast<float>(rand() % 1000) / 1000.0f) - 0.5f) * 60.0f;
			Mote.FallDist = 45.0f + (static_cast<float>(rand() % 1000) / 1000.0f) * 85.0f;
			Mote.Duration = 1.3f + (static_cast<float>(rand() % 1000) / 1000.0f) * 0.7f;
			Mote.Delay = (static_cast<float>(rand() % 1000) / 1000.0f) * 0.35f;
			Mote.Alpha = 0.5f + (static_cast<float>(rand() % 1000) / 1000.0f) * 0.45f;
			Mote.R = DustCols[i % NumDustColors][0];
			Mote.G = DustCols[i % NumDustColors][1];
			Mote.B = DustCols[i % NumDustColors][2];
			DustMotes.Add(Mote);
		}

		// Animation Loop: 4.15s total duration
		const double AnimStartTime = FPlatformTime::Seconds();
		constexpr double TotalDuration = 4.15;

		while (!GStopThread.load())
		{
			const double CurTime = FPlatformTime::Seconds();
			const double Elapsed = CurTime - AnimStartTime;
			if (Elapsed >= TotalDuration)
			{
				break;
			}

			// Clear framebuffer to 100% transparent black
			memset(pBits, 0, WinW * WinH * sizeof(uint32));

			// 1. Logo Artwork (with progressive top-to-bottom unveiling during cut phase)
			if (LogoPixels)
			{
				const float CutP = FMath::Clamp(static_cast<float>(Elapsed / 2.40), 0.0f, 1.0f);
				const float RevealFrac = (Elapsed < 2.40) ? FMath::Max(0.10f, CutP * 1.10f) : 1.0f;
				const int32 RevealH = FMath::Clamp(FMath::RoundToInt(WinH * RevealFrac), 1, WinH);

				const bool bIsFlash = (Elapsed >= 2.40 && Elapsed <= 2.65);
				const float FlashFrac = bIsFlash ? (1.0f - static_cast<float>((Elapsed - 2.40) / 0.25)) : 0.0f;

				for (int32 Y = 0; Y < RevealH; ++Y)
				{
					const int32 RowOffset = Y * WinW;
					for (int32 X = 0; X < WinW; ++X)
					{
						uint32 Pix = LogoPixels[RowOffset + X];
						if (Pix == 0) continue;

						if (bIsFlash)
						{
							uint32 B = (Pix & 0xFF);
							uint32 G = ((Pix >> 8) & 0xFF);
							uint32 R = ((Pix >> 16) & 0xFF);
							uint32 A = ((Pix >> 24) & 0xFF);

							if (A > 0)
							{
								const float FlashBoost = 1.0f + 0.85f * FlashFrac;
								const float WhiteAdd = A * 0.35f * FlashFrac;
								R = FMath::Clamp(static_cast<uint32>(R * FlashBoost + WhiteAdd), 0u, static_cast<uint32>(A));
								G = FMath::Clamp(static_cast<uint32>(G * FlashBoost + WhiteAdd), 0u, static_cast<uint32>(A));
								B = FMath::Clamp(static_cast<uint32>(B * FlashBoost + WhiteAdd * 1.15f), 0u, static_cast<uint32>(A));
								Pix = (A << 24) | (R << 16) | (G << 8) | B;
							}
						}
						TargetPixels[RowOffset + X] = Pix;
					}
				}
			}

			// 2. Stage 1: Laser Cut Border & Spatter (0.0s - 2.40s)
			if (Elapsed < 2.40)
			{
				const float CutP = FMath::Clamp(static_cast<float>(Elapsed / 2.40), 0.0f, 1.0f);
				const float TargetDist = CutP * TotalPerimeter;

				float AccDist = 0.0f;
				FVector2f HeadPt = PolyPoints[0];

				for (int32 i = 0; i < NumPts - 1; ++i)
				{
					const FVector2f& P1 = PolyPoints[i];
					const FVector2f& P2 = PolyPoints[i + 1];

					if (AccDist + SegLengths[i] <= TargetDist)
					{
						// Soft cyan flame glow (#laser-flame feGaussianBlur)
						DrawLineAA(TargetPixels, WinW, WinH, P1.X, P1.Y, P2.X, P2.Y, 9.0f, 0, 243, 255, 0.35f);
						// Sharp cyan seam line
						DrawLineAA(TargetPixels, WinW, WinH, P1.X, P1.Y, P2.X, P2.Y, 3.5f, 0, 243, 255, 0.85f);
						// White-hot core line
						DrawLineAA(TargetPixels, WinW, WinH, P1.X, P1.Y, P2.X, P2.Y, 1.5f, 255, 255, 255, 1.0f);
						HeadPt = P2;
					}
					else if (AccDist < TargetDist)
					{
						const float Frac = (TargetDist - AccDist) / SegLengths[i];
						const FVector2f CurPt(P1.X + (P2.X - P1.X) * Frac, P1.Y + (P2.Y - P1.Y) * Frac);
						DrawLineAA(TargetPixels, WinW, WinH, P1.X, P1.Y, CurPt.X, CurPt.Y, 9.0f, 0, 243, 255, 0.35f);
						DrawLineAA(TargetPixels, WinW, WinH, P1.X, P1.Y, CurPt.X, CurPt.Y, 3.5f, 0, 243, 255, 0.85f);
						DrawLineAA(TargetPixels, WinW, WinH, P1.X, P1.Y, CurPt.X, CurPt.Y, 1.5f, 255, 255, 255, 1.0f);
						HeadPt = CurPt;
						break;
					}
					AccDist += SegLengths[i];
				}

				// Laser cutting torch head
				DrawGlowCircle(TargetPixels, WinW, WinH, HeadPt.X, HeadPt.Y, 18.0f, 0, 243, 255, 0.45f);
				DrawGlowCircle(TargetPixels, WinW, WinH, HeadPt.X, HeadPt.Y, 8.0f, 255, 0, 255, 0.90f);
				DrawGlowCircle(TargetPixels, WinW, WinH, HeadPt.X, HeadPt.Y, 4.5f, 255, 255, 255, 1.0f);

				// Directional sparks matching SVG lines
				DrawLineAA(TargetPixels, WinW, WinH, HeadPt.X, HeadPt.Y, HeadPt.X - 14.0f, HeadPt.Y - 12.0f, 2.0f, 255, 184, 0, 0.90f);
				DrawLineAA(TargetPixels, WinW, WinH, HeadPt.X, HeadPt.Y, HeadPt.X + 16.0f, HeadPt.Y - 15.0f, 1.8f, 255, 255, 255, 0.95f);
				DrawLineAA(TargetPixels, WinW, WinH, HeadPt.X, HeadPt.Y, HeadPt.X - 8.0f,  HeadPt.Y + 18.0f, 2.2f, 0, 243, 255, 0.85f);
				DrawLineAA(TargetPixels, WinW, WinH, HeadPt.X, HeadPt.Y, HeadPt.X + 14.0f, HeadPt.Y + 14.0f, 1.8f, 255, 0, 127, 0.90f);
				DrawLineAA(TargetPixels, WinW, WinH, HeadPt.X, HeadPt.Y, HeadPt.X - 18.0f, HeadPt.Y + 4.0f,  1.5f, 255, 184, 0, 0.75f);

				// Dynamic sparks
				const uint8 SparkCols[][3] = {
					{ 255, 184, 0 },
					{ 255, 255, 255 },
					{ 0, 243, 255 },
					{ 255, 0, 127 }
				};
				for (int32 s = 0; s < 6; ++s)
				{
					const float Ang = (static_cast<float>(rand() % 360) * 3.14159265f) / 180.0f;
					const float Dist = 10.0f + static_cast<float>(rand() % 18);
					const float SparkX = HeadPt.X + std::cos(Ang) * Dist;
					const float SparkY = HeadPt.Y + std::sin(Ang) * Dist;
					DrawLineAA(TargetPixels, WinW, WinH, HeadPt.X, HeadPt.Y, SparkX, SparkY, 1.6f, SparkCols[s % 4][0], SparkCols[s % 4][1], SparkCols[s % 4][2], 0.9f);
				}
			}
			// 3. Stage 2 & 3: Flash, Thermal Cooldown Seam & Specular Sheen (2.40s - 3.75s)
			else
			{
				// Thermal Seam Cooldown (2.40s - 3.75s) matching @keyframes thermal-cool:
				// 0% (2.40s): White-Hot #FFFFFF with Amber glow
				// 25% (2.75s): Amber #FFB800 with Orange glow
				// 65% (3.30s): Cyan #00F3FF
				// 100% (3.75s): Cobalt #0055FF fading out
				const float CoolP = FMath::Clamp(static_cast<float>((Elapsed - 2.40) / 1.35), 0.0f, 1.0f);
				uint8 SeamR = 255, SeamG = 255, SeamB = 255;
				float SeamAlpha = 1.0f;
				float GlowAlpha = 0.0f;

				if (CoolP < 0.25f)
				{
					const float t = CoolP / 0.25f;
					SeamR = 255;
					SeamG = static_cast<uint8>(255 - t * (255 - 184));
					SeamB = static_cast<uint8>(255 - t * 255);
					SeamAlpha = 1.0f - t * 0.10f;
					GlowAlpha = (1.0f - t) * 0.5f;
				}
				else if (CoolP < 0.65f)
				{
					const float t = (CoolP - 0.25f) / 0.40f;
					SeamR = static_cast<uint8>(255 * (1.0f - t));
					SeamG = static_cast<uint8>(184 + t * (243 - 184));
					SeamB = static_cast<uint8>(t * 255);
					SeamAlpha = 0.90f - t * 0.45f;
				}
				else
				{
					const float t = (CoolP - 0.65f) / 0.35f;
					SeamR = 0;
					SeamG = static_cast<uint8>(243 - t * (243 - 85));
					SeamB = 255;
					SeamAlpha = 0.45f * (1.0f - t);
				}

				if (SeamAlpha > 0.02f)
				{
					for (int32 i = 0; i < NumPts - 1; ++i)
					{
						if (GlowAlpha > 0.02f)
						{
							DrawLineAA(TargetPixels, WinW, WinH, PolyPoints[i].X, PolyPoints[i].Y, PolyPoints[i + 1].X, PolyPoints[i + 1].Y, 7.0f, 255, 102, 0, GlowAlpha);
						}
						DrawLineAA(TargetPixels, WinW, WinH, PolyPoints[i].X, PolyPoints[i].Y, PolyPoints[i + 1].X, PolyPoints[i + 1].Y, 2.5f, SeamR, SeamG, SeamB, SeamAlpha);
					}
				}

				// Specular Sheen Beam Sweep & Starburst (2.60s - 3.75s)
				if (Elapsed >= 2.60 && Elapsed <= 3.75)
				{
					const float SheenP = static_cast<float>((Elapsed - 2.60) / 1.15);
					const float SheenCenterX = -250.0f + SheenP * (WinW + 500.0f);
					constexpr float SheenHalfW = 130.0f;

					for (int32 Y = 0; Y < WinH; ++Y)
					{
						const float SkewX = ((static_cast<float>(Y) / static_cast<float>(WinH)) - 0.5f) * 115.0f;
						const float RowCenterX = SheenCenterX + SkewX;
						const int32 MinX = FMath::Clamp(static_cast<int32>(RowCenterX - SheenHalfW), 0, WinW - 1);
						const int32 MaxX = FMath::Clamp(static_cast<int32>(RowCenterX + SheenHalfW), 0, WinW - 1);

						for (int32 X = MinX; X <= MaxX; ++X)
						{
							const float Offset = static_cast<float>(X) - RowCenterX;
							const float Dist = FMath::Abs(Offset);
							if (Dist < SheenHalfW)
							{
								float BeamFrac = 1.0f - (Dist / SheenHalfW);
								BeamFrac = BeamFrac * BeamFrac;

								// Leading edge is cyan, trailing edge is magenta (matching 115deg gradient)
								if (Offset < 0)
								{
									const uint8 CyanA = static_cast<uint8>(BeamFrac * 125.0f);
									if (CyanA > 0)
									{
										BlendPixelAdd(TargetPixels, WinW, WinH, X, Y, 0, 243, 255, CyanA);
									}
								}
								else
								{
									const uint8 MagA = static_cast<uint8>(BeamFrac * 105.0f);
									if (MagA > 0)
									{
										BlendPixelAdd(TargetPixels, WinW, WinH, X, Y, 255, 0, 255, MagA);
									}
								}

								// White specular core line
								if (Dist < SheenHalfW * 0.26f)
								{
									const float CoreFrac = 1.0f - (Dist / (SheenHalfW * 0.26f));
									const uint8 CoreA = static_cast<uint8>(CoreFrac * CoreFrac * 210.0f);
									if (CoreA > 0)
									{
										BlendPixelAdd(TargetPixels, WinW, WinH, X, Y, 255, 255, 255, CoreA);
									}
								}
							}
						}
					}

					// Concentric Shockwave expanding from center
					const float CenterX = WinW * 0.5f;
					const float CenterY = WinH * 0.5f;
					const float WaveRadius = 25.0f + SheenP * 290.0f;
					const float WaveAlpha = FMath::Max(0.0f, (1.0f - SheenP * SheenP) * 0.85f);
					if (WaveAlpha > 0.05f)
					{
						uint8 WaveR = 0, WaveG = 243, WaveB = 255;
						if (SheenP > 0.35f)
						{
							const float wt = FMath::Clamp((SheenP - 0.35f) / 0.45f, 0.0f, 1.0f);
							WaveR = static_cast<uint8>(wt * 255.0f);
							WaveG = static_cast<uint8>((1.0f - wt) * 243.0f);
							WaveB = 255;
						}
						constexpr int32 RingSteps = 56;
						for (int32 r = 0; r < RingSteps; ++r)
						{
							const float Ang1 = (static_cast<float>(r) / static_cast<float>(RingSteps)) * 6.2831853f;
							const float Ang2 = (static_cast<float>(r + 1) / static_cast<float>(RingSteps)) * 6.2831853f;
							const float X1 = CenterX + std::cos(Ang1) * WaveRadius;
							const float Y1 = CenterY + std::sin(Ang1) * WaveRadius * 0.65f;
							const float X2 = CenterX + std::cos(Ang2) * WaveRadius;
							const float Y2 = CenterY + std::sin(Ang2) * WaveRadius * 0.65f;
							DrawLineAA(TargetPixels, WinW, WinH, X1, Y1, X2, Y2, 2.5f, WaveR, WaveG, WaveB, WaveAlpha);
						}
					}

					// Central Rotating Diamond Starburst Flare
					const float FlareT = FMath::Clamp((SheenP - 0.18f) / 0.62f, 0.0f, 1.0f);
					if (FlareT > 0.0f && FlareT < 1.0f)
					{
						const float FlareAlpha = (FlareT < 0.30f) ? (FlareT / 0.30f) : (1.0f - (FlareT - 0.30f) / 0.70f);
						const float FlareScale = (FlareT < 0.30f) ? (0.2f + (FlareT / 0.30f) * 1.6f) : (1.8f - (FlareT - 0.30f) * 1.5f);
						const float Angle = (FlareT * 135.0f) * (3.14159265f / 180.0f);
						const float CosA = std::cos(Angle);
						const float SinA = std::sin(Angle);
						const float ArmLen = 70.0f * FlareScale;

						const float DX1 = CosA * ArmLen;
						const float DY1 = SinA * ArmLen;
						const float DX2 = -SinA * ArmLen;
						const float DY2 = CosA * ArmLen;

						DrawLineAA(TargetPixels, WinW, WinH, CenterX - DX1, CenterY - DY1, CenterX + DX1, CenterY + DY1, 2.8f, 255, 255, 255, FlareAlpha);
						DrawLineAA(TargetPixels, WinW, WinH, CenterX - DX2, CenterY - DY2, CenterX + DX2, CenterY + DY2, 2.8f, 0, 243, 255, FlareAlpha);

						const float DAng = Angle + 0.785398f;
						const float CD1 = std::cos(DAng) * ArmLen * 0.65f;
						const float SD1 = std::sin(DAng) * ArmLen * 0.65f;
						const float CD2 = -std::sin(DAng) * ArmLen * 0.65f;
						const float SD2 = std::cos(DAng) * ArmLen * 0.65f;
						DrawLineAA(TargetPixels, WinW, WinH, CenterX - CD1, CenterY - SD1, CenterX + CD1, CenterY + SD1, 1.8f, 255, 0, 255, FlareAlpha * 0.75f);
						DrawLineAA(TargetPixels, WinW, WinH, CenterX - CD2, CenterY - SD2, CenterX + CD2, CenterY + SD2, 1.8f, 255, 0, 255, FlareAlpha * 0.75f);

						DrawGlowCircle(TargetPixels, WinW, WinH, CenterX, CenterY, 15.0f * FlareScale, 255, 255, 255, FlareAlpha);
					}

					// 36 Atmospheric Dust Particles Settling
					for (const FDustMote& Mote : DustMotes)
					{
						const float ParticleT = FMath::Clamp(static_cast<float>((Elapsed - 2.65 - Mote.Delay) / Mote.Duration), 0.0f, 1.0f);
						if (ParticleT > 0.0f && ParticleT < 1.0f)
						{
							const float CurX = Mote.X + Mote.DriftX * ParticleT;
							const float CurY = Mote.Y + Mote.FallDist * (ParticleT * ParticleT);

							float MoteFade = 1.0f;
							if (ParticleT < 0.15f)
							{
								MoteFade = ParticleT / 0.15f;
							}
							else if (ParticleT > 0.65f)
							{
								MoteFade = 1.0f - (ParticleT - 0.65f) / 0.35f;
							}

							const float CurAlpha = Mote.Alpha * MoteFade * 0.90f;
							const float CurSize = Mote.Size * (1.15f - ParticleT * 0.55f);
							DrawGlowCircle(TargetPixels, WinW, WinH, CurX, CurY, CurSize, Mote.R, Mote.G, Mote.B, CurAlpha);
						}
					}
				}
			}

			// Global fade-out calculation (3.75s - 4.15s)
			BYTE GlobalAlpha = 255;
			if (Elapsed > 3.75)
			{
				const float FadeFrac = FMath::Clamp(static_cast<float>((Elapsed - 3.75) / 0.40), 0.0f, 1.0f);
				GlobalAlpha = static_cast<BYTE>((1.0f - FadeFrac) * 255.0f);
			}

			// Present transparent frame to desktop
			POINT ptSrc = { 0, 0 };
			POINT ptDst = { WinX, WinY };
			SIZE sizeWnd = { WinW, WinH };
			BLENDFUNCTION blend = { AC_SRC_OVER, 0, GlobalAlpha, AC_SRC_ALPHA };
			UpdateLayeredWindow(hWnd, hdcScreen, &ptDst, &sizeWnd, hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);

			Sleep(16); // ~60 FPS
		}

		PlaySoundW(NULL, NULL, 0);

		SelectObject(hdcMem, hOldBmp);
		DeleteObject(hBitmap);
		DeleteDC(hdcMem);
		ReleaseDC(NULL, hdcScreen);

		DestroyWindow(hWnd);
		UnregisterClassW(L"TachyonStudioTransparentSplashClass", GetModuleHandle(NULL));
	}

	static void RunNobleFermiRectangularSplash(HWND hUnderlyingNativeSplash)
	{
		if (hUnderlyingNativeSplash && IsWindow(hUnderlyingNativeSplash))
		{
			ShowWindow(hUnderlyingNativeSplash, SW_HIDE);
		}

		const FString CandidatePaths[] = {
			FPaths::ProjectContentDir() / TEXT("Splash/Splash.bmp"),
			FPaths::ProjectDir() / TEXT("Content/Splash/Splash.bmp"),
			FPaths::RootDir() / TEXT("Content/Splash/Splash.bmp"),
			FPaths::RootDir() / TEXT("NobleFermi/Content/Splash/Splash.bmp"),
			FPaths::LaunchDir() / TEXT("Content/Splash/Splash.bmp"),
			FPaths::LaunchDir() / TEXT("../../Content/Splash/Splash.bmp"),
			FPaths::LaunchDir() / TEXT("../../../Content/Splash/Splash.bmp"),
			TEXT("Content/Splash/Splash.bmp")
		};
		FString SplashPath;
		for (const FString& P : CandidatePaths)
		{
			if (FPaths::FileExists(P))
			{
				SplashPath = FPaths::ConvertRelativePathToFull(P);
				break;
			}
		}

		HBITMAP hBmpSplash = NULL;
		if (!SplashPath.IsEmpty())
		{
			hBmpSplash = (HBITMAP)LoadImageW(NULL, *SplashPath, IMAGE_BITMAP, 800, 450, LR_LOADFROMFILE);
		}

		WNDCLASSEXW Wc = { 0 };
		Wc.cbSize = sizeof(WNDCLASSEXW);
		Wc.style = CS_HREDRAW | CS_VREDRAW;
		Wc.lpfnWndProc = &DefWindowProcW;
		Wc.hInstance = GetModuleHandle(NULL);
		Wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		Wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		Wc.lpszClassName = L"NobleFermiActiveSplashClass";
		RegisterClassExW(&Wc);

		const int32 ScreenW = GetSystemMetrics(SM_CXSCREEN);
		const int32 ScreenH = GetSystemMetrics(SM_CYSCREEN);
		constexpr int32 WndW = 800;
		constexpr int32 WndH = 450;
		const int32 WndX = (ScreenW - WndW) / 2;
		const int32 WndY = (ScreenH - WndH) / 2;

		HWND hWnd = CreateWindowExW(
			WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
			L"NobleFermiActiveSplashClass",
			L"Noble Fermi Loading",
			WS_POPUP,
			WndX, WndY, WndW, WndH,
			NULL, NULL, GetModuleHandle(NULL), NULL
		);

		if (!hWnd)
		{
			if (hBmpSplash) DeleteObject(hBmpSplash);
			UnregisterClassW(L"NobleFermiActiveSplashClass", GetModuleHandle(NULL));
			return;
		}

		ShowWindow(hWnd, SW_SHOW);
		UpdateWindow(hWnd);
		SetWindowPos(hWnd, HWND_TOPMOST, WndX, WndY, WndW, WndH, SWP_SHOWWINDOW);

		constexpr int32 BarX = 140;
		constexpr int32 BarY = 386;
		constexpr int32 BarW = 520;
		constexpr int32 BarH = 18;

		HFONT hFontBar = CreateFontW(
			-11, 0, 0, 0, FW_BOLD, 0, 0, 0,
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
		if (!hFontBar) hFontBar = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

		HFONT hFontSub = CreateFontW(
			-11, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0,
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
		if (!hFontSub) hFontSub = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

		HBRUSH hBgBrush = CreateSolidBrush(RGB(3, 7, 18));
		HBRUSH hTroughBrush = CreateSolidBrush(RGB(8, 18, 32));
		HBRUSH hBorderBrush = CreateSolidBrush(RGB(0, 229, 255));
		HBRUSH hFillBrush = CreateSolidBrush(RGB(255, 179, 0));
		HBRUSH hHighlightBrush = CreateSolidBrush(RGB(255, 224, 102));
		HBRUSH hTipBrush = CreateSolidBrush(RGB(255, 250, 220));
		HPEN hGridPen = CreatePen(PS_SOLID, 1, RGB(10, 24, 42));

		HDC hdcWnd = GetDC(hWnd);
		HDC hdcMem = CreateCompatibleDC(hdcWnd);
		HBITMAP hbmMem = CreateCompatibleBitmap(hdcWnd, WndW, WndH);
		HGDIOBJ hOldMemBmp = SelectObject(hdcMem, hbmMem);

		HDC hdcSplashBmp = CreateCompatibleDC(hdcWnd);
		HGDIOBJ hOldSplashBmp = hBmpSplash ? SelectObject(hdcSplashBmp, hBmpSplash) : NULL;

		const double LoadingBarStartTime = FPlatformTime::Seconds();
		int32 MaxPctReached = 3;
		double FinishStartTime = -1.0;
		int32 FinishStartPct = 3;

		while (!GStopThread.load())
		{
			MSG Msg;
			while (PeekMessageW(&Msg, NULL, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&Msg);
				DispatchMessageW(&Msg);
			}

			const double BarElapsed = FPlatformTime::Seconds() - LoadingBarStartTime;
			const int32 CurObjects = GUObjectArray.GetObjectArrayNum();

			// Smooth, responsive progress progression model:
			// Starts at 3%, moves steadily across phases, and rapidly caps to 100% on GWorldLoaded
			float BaseFrac = 0.03f;
			FString PhaseStr = TEXT("INITIALIZING D3D11 RHI & SHADER PIPELINE");
			FString ItemStr = TEXT("Core Engine Subsystems & Vertex Layouts");

			if (BarElapsed < 0.6)
			{
				BaseFrac = 0.03f + static_cast<float>(BarElapsed / 0.6) * 0.28f;
				PhaseStr = TEXT("INITIALIZING D3D11 RHI & SHADER PIPELINE");
				ItemStr = FString::Printf(TEXT("PSO Bytecode & Pipeline State (%d UObjects)"), CurObjects);
			}
			else if (BarElapsed < 1.3)
			{
				BaseFrac = 0.31f + static_cast<float>((BarElapsed - 0.6) / 0.7) * 0.35f;
				PhaseStr = TEXT("STREAMING CONTENT PACKAGES & PBR MATERIALS");
				ItemStr = TEXT("Operator Skeletal Rig, Armor & Weapon Finishes");
			}
			else if (BarElapsed < 2.0)
			{
				BaseFrac = 0.66f + static_cast<float>((BarElapsed - 1.3) / 0.7) * 0.25f;
				PhaseStr = TEXT("CONSTRUCTING HANGAR DIORAMA & VOLUMETRIC LIGHTING");
				ItemStr = TEXT("Spawning Turntable Stage, Table Displays & Cold Fog");
			}
			else
			{
				BaseFrac = 0.91f + static_cast<float>(FMath::Min(0.06, (BarElapsed - 2.0) * 0.02));
				PhaseStr = TEXT("PREPARING COMMAND CENTER & SLATE UI");
				ItemStr = TEXT("Finalizing Hangar Render Targets & Post-Processing");
			}

			// Real-time custom item override from delegates if available
			{
				std::lock_guard<std::mutex> Lock(GTextMutex);
				if (!GLatestItemName.IsEmpty() && !GLatestItemName.Equals(TEXT("Core Engine Subsystems")))
				{
					ItemStr = GLatestItemName;
				}
				if (!GCurrentPhaseName.IsEmpty() && !GCurrentPhaseName.Equals(TEXT("INITIALIZING ENGINE SUBSYSTEMS")))
				{
					PhaseStr = GCurrentPhaseName;
				}
			}

			int32 TargetPct = FMath::Clamp(FMath::RoundToInt(BaseFrac * 100.0f), 3, 96);

			if (GWorldLoaded.load())
			{
				if (FinishStartTime < 0.0)
				{
					FinishStartTime = FPlatformTime::Seconds();
					FinishStartPct = FMath::Max(MaxPctReached, 65);
				}
				const double FinishElapsed = FPlatformTime::Seconds() - FinishStartTime;
				const float FinishFrac = FMath::Clamp(static_cast<float>(FinishElapsed / 0.45), 0.0f, 1.0f);
				TargetPct = FMath::RoundToInt(FMath::Lerp(static_cast<float>(FinishStartPct), 100.0f, FinishFrac));
				PhaseStr = TEXT("DEPLOYMENT READY // ENTERING COMMAND CENTER");
				ItemStr = TEXT("MenuMap Loaded [128-HZ AUTHORITATIVE ENGINE READY]");

				if (FinishFrac >= 1.0f && FinishElapsed > 0.60)
				{
					break; // Smooth completion achieved
				}
			}

			if (TargetPct > MaxPctReached)
			{
				MaxPctReached = TargetPct;
			}

			// 1. Draw base splash artwork
			if (hBmpSplash)
			{
				BitBlt(hdcMem, 0, 0, WndW, WndH, hdcSplashBmp, 0, 0, SRCCOPY);
			}
			else
			{
				RECT FullRect{ 0, 0, WndW, WndH };
				FillRect(hdcMem, &FullRect, hBgBrush);
			}

			// 2. Clear trough & draw outer cyan border
			RECT OuterBar{ BarX, BarY, BarX + BarW, BarY + BarH };
			FillRect(hdcMem, &OuterBar, hBorderBrush);

			RECT InnerTrough{ BarX + 1, BarY + 1, BarX + BarW - 1, BarY + BarH - 1 };
			FillRect(hdcMem, &InnerTrough, hTroughBrush);

			// 3. Draw live advancing yellow/gold progress bar fill
			const int32 MaxFillW = BarW - 4;
			const int32 FillW = FMath::Clamp((MaxFillW * MaxPctReached) / 100, 6, MaxFillW);
			RECT FillRectArea{ BarX + 2, BarY + 2, BarX + 2 + FillW, BarY + BarH - 2 };
			FillRect(hdcMem, &FillRectArea, hFillBrush);

			RECT BevelRect{ BarX + 2, BarY + 2, BarX + 2 + FillW, BarY + 5 };
			FillRect(hdcMem, &BevelRect, hHighlightBrush);

			if (FillW > 4)
			{
				RECT TipRect{ BarX + 2 + FillW - 3, BarY + 2, BarX + 2 + FillW, BarY + BarH - 2 };
				FillRect(hdcMem, &TipRect, hTipBrush);
			}

			// 4. Clear sub-text region and draw texts
			RECT SubBgRect{ 100, BarY + BarH + 4, WndW - 100, BarY + BarH + 34 };
			FillRect(hdcMem, &SubBgRect, hBgBrush);

			SetBkMode(hdcMem, TRANSPARENT);

			// Draw inside bar text with black outline
			FString BarInsideText = FString::Printf(TEXT("%s  —  %d%%"), *PhaseStr, MaxPctReached);
			HGDIOBJ hOldFont = SelectObject(hdcMem, hFontBar);
			RECT BarTextRect{ BarX + 6, BarY + 1, BarX + BarW - 6, BarY + BarH - 1 };
			SetTextColor(hdcMem, RGB(3, 7, 18));
			const int32 Offsets[8][2] = { {-1,0}, {1,0}, {0,-1}, {0,1}, {-1,-1}, {1,-1}, {-1,1}, {1,1} };
			for (int32 i = 0; i < 8; ++i)
			{
				RECT OutR{ BarTextRect.left + Offsets[i][0], BarTextRect.top + Offsets[i][1], BarTextRect.right + Offsets[i][0], BarTextRect.bottom + Offsets[i][1] };
				DrawTextW(hdcMem, *BarInsideText, BarInsideText.Len(), &OutR, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
			}
			SetTextColor(hdcMem, RGB(255, 255, 255));
			DrawTextW(hdcMem, *BarInsideText, BarInsideText.Len(), &BarTextRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

			// Draw sub-bar item info in tactical cyan
			FString SubBarText = FString::Printf(TEXT("LOADING: %s   [TACHYON STUDIO | %.1fs]"), *ItemStr, BarElapsed);
			SelectObject(hdcMem, hFontSub);
			RECT SubTextRect{ 40, BarY + BarH + 6, WndW - 40, BarY + BarH + 26 };
			SetTextColor(hdcMem, RGB(0, 229, 255));
			DrawTextW(hdcMem, *SubBarText, SubBarText.Len(), &SubTextRect, DT_CENTER | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);

			SelectObject(hdcMem, hOldFont);

			// Atomic BitBlt to window
			BitBlt(hdcWnd, 0, 0, WndW, WndH, hdcMem, 0, 0, SRCCOPY);

			Sleep(16); // ~60 FPS
		}

		if (hBmpSplash && hOldSplashBmp) SelectObject(hdcSplashBmp, hOldSplashBmp);
		DeleteDC(hdcSplashBmp);
		if (hBmpSplash) DeleteObject(hBmpSplash);

		SelectObject(hdcMem, hOldMemBmp);
		DeleteObject(hbmMem);
		DeleteDC(hdcMem);
		ReleaseDC(hWnd, hdcWnd);

		if (hFontBar && hFontBar != GetStockObject(DEFAULT_GUI_FONT)) DeleteObject(hFontBar);
		if (hFontSub && hFontSub != GetStockObject(DEFAULT_GUI_FONT)) DeleteObject(hFontSub);
		DeleteObject(hBgBrush);
		DeleteObject(hTroughBrush);
		DeleteObject(hBorderBrush);
		DeleteObject(hFillBrush);
		DeleteObject(hHighlightBrush);
		DeleteObject(hTipBrush);
		DeleteObject(hGridPen);

		DestroyWindow(hWnd);
		UnregisterClassW(L"NobleFermiActiveSplashClass", GetModuleHandle(NULL));
	}

	static void SplashWorkerLoop()
	{
		HWND hNativeSplash = NULL;
		for (int32 retry = 0; retry < 50; ++retry)
		{
			hNativeSplash = FindProcessSplashWindow();
			if (hNativeSplash && IsWindow(hNativeSplash))
			{
				ShowWindow(hNativeSplash, SW_HIDE);
				break;
			}
			Sleep(20);
		}

		// 1. Play Tachyon Studio Hero Logo Laser-Cut Splash Animation (Transparent Layered Window)
		RunTachyonTransparentSplash(hNativeSplash);

		// 2. Play Noble Fermi Rectangular Splash Screen with 100% Functioning Live Progress Bar
		RunNobleFermiRectangularSplash(hNativeSplash);
	}
}
#endif // PLATFORM_WINDOWS

class FNobleFermiModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		FDefaultGameModuleImpl::StartupModule();
		FFPSGameplayTags::InitializeNativeGameplayTags();

#if PLATFORM_WINDOWS
		NobleFermiSplashProgress::GStartTimeSeconds = FPlatformTime::Seconds();
		NobleFermiSplashProgress::GStopThread = false;
		NobleFermiSplashProgress::GWorldLoaded = false;

		FCoreUObjectDelegates::PackageCreatedForLoad.AddStatic(&NobleFermiSplashProgress::OnPackageCreated);
#if WITH_EDITOR
		FCoreUObjectDelegates::OnAssetLoaded.AddStatic(&NobleFermiSplashProgress::OnAssetLoaded);
#endif
		FCoreUObjectDelegates::PreLoadMap.AddStatic(&NobleFermiSplashProgress::OnPreLoadMap);
		FCoreUObjectDelegates::PostLoadMapWithWorld.AddStatic(&NobleFermiSplashProgress::OnPostLoadMap);

		NobleFermiSplashProgress::GSplashThread = std::thread(&NobleFermiSplashProgress::SplashWorkerLoop);
#endif
	}

	virtual void ShutdownModule() override
	{
#if PLATFORM_WINDOWS
		NobleFermiSplashProgress::GStopThread = true;
		if (NobleFermiSplashProgress::GSplashThread.joinable())
		{
			NobleFermiSplashProgress::GSplashThread.join();
		}
#endif
		FDefaultGameModuleImpl::ShutdownModule();
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FNobleFermiModule, NobleFermi, "NobleFermi");
