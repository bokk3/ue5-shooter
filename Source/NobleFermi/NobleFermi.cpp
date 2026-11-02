#include "NobleFermi.h"
#include "Modules/ModuleManager.h"
#include "Lyra/AbilitySystem/FPSGameplayTags.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UObjectArray.h"
#include "UObject/Package.h"
#include "UObject/Class.h"
#include "Misc/CoreDelegates.h"
#include "HAL/PlatformTime.h"

#if PLATFORM_WINDOWS
#include <atomic>
#include <thread>
#include <mutex>
#include "Windows/AllowWindowsPlatformTypes.h"
#include <windows.h>
#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "User32.lib")

namespace NobleFermiSplashProgress
{
	static std::atomic<bool> GStopThread{ false };
	static std::atomic<bool> GWorldLoaded{ false };
	static std::atomic<int32> GPackagesLoaded{ 0 };
	static std::atomic<int32> GAssetsLoaded{ 0 };
	static double GStartTimeSeconds = 0.0;
	static int32 GBaselineObjectCount = 0;

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
			else if (Count < 80)
			{
				Phase = TEXT("INITIALIZING D3D11 RHI & SHADER PIPELINE");
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
		SetLatestLoadingItem(MapName, TEXT("LOADING WORLD & SPAWNING HANGAR STAGE"));
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
					return FALSE;
				}
			}
		}
		return TRUE;
	}

	static HWND FindProcessSplashWindow()
	{
		FSplashEnumCtx Ctx{ GetCurrentProcessId(), NULL };
		EnumWindows(&EnumSplashWndProc, reinterpret_cast<LPARAM>(&Ctx));
		return Ctx.FoundHwnd;
	}

	static void SplashWorkerLoop()
	{
		// Offscreen region covers x=[60..740], y=[380..442] on the 800x450 splash window
		constexpr int32 RegionX = 60;
		constexpr int32 RegionY = 380;
		constexpr int32 RegionW = 680;
		constexpr int32 RegionH = 62;

		constexpr int32 BarX = 140 - RegionX; // 80 in local offscreen coords
		constexpr int32 BarY = 386 - RegionY; // 6 in local offscreen coords
		constexpr int32 BarW = 520;
		constexpr int32 BarH = 18;

		HFONT hFontBar = CreateFontW(
			-11, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
		if (!hFontBar)
		{
			hFontBar = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
		}

		HFONT hFontSub = CreateFontW(
			-11, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
		if (!hFontSub)
		{
			hFontSub = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
		}

		HBRUSH hBgBrush = CreateSolidBrush(RGB(3, 7, 18));
		HBRUSH hTroughBrush = CreateSolidBrush(RGB(8, 18, 32));
		HBRUSH hBorderBrush = CreateSolidBrush(RGB(0, 229, 255));
		HBRUSH hFillBrush = CreateSolidBrush(RGB(255, 179, 0));
		HBRUSH hHighlightBrush = CreateSolidBrush(RGB(255, 224, 102));
		HBRUSH hTipBrush = CreateSolidBrush(RGB(255, 250, 220));
		HPEN hGridPen = CreatePen(PS_SOLID, 1, RGB(10, 24, 42));

		int32 MaxPctReached = 4;

		while (!GStopThread.load())
		{
			HWND hSplash = FindProcessSplashWindow();
			if (hSplash && IsWindow(hSplash) && IsWindowVisible(hSplash))
			{
				const double Elapsed = FPlatformTime::Seconds() - GStartTimeSeconds;
				const int32 CurObjects = GUObjectArray.GetObjectArrayNum();
				const int32 ObjDelta = FMath::Max(0, CurObjects - GBaselineObjectCount);
				const int32 Pkgs = GPackagesLoaded.load();
				const int32 Assets = GAssetsLoaded.load();

				// Calculate real combined loading progress from UObject creation, package streaming, and world state
				const float ObjFrac = FMath::Clamp(static_cast<float>(ObjDelta) / 38000.0f, 0.0f, 0.55f);
				const float PkgFrac = FMath::Clamp(static_cast<float>(Pkgs) / 320.0f, 0.0f, 0.30f);
				const float TimeFrac = FMath::Clamp(static_cast<float>(Elapsed) / 45.0f, 0.0f, 0.12f);

				int32 TargetPct = FMath::Clamp(4 + FMath::RoundToInt((ObjFrac + PkgFrac + TimeFrac) * 95.0f), 4, 98);
				if (GWorldLoaded.load())
				{
					TargetPct = 100;
				}
				if (TargetPct > MaxPctReached)
				{
					MaxPctReached = TargetPct;
				}

				FString ItemCopy;
				FString PhaseCopy;
				{
					std::lock_guard<std::mutex> Lock(GTextMutex);
					ItemCopy = GLatestItemName;
					PhaseCopy = GCurrentPhaseName;
				}

				// Update Phase from object count if no packages fired yet
				if (Pkgs == 0 && CurObjects > 0)
				{
					if (CurObjects < 25000)
					{
						PhaseCopy = TEXT("INITIALIZING CORE UOBJECTS & REFLECTION");
					}
					else if (CurObjects < 40000)
					{
						PhaseCopy = TEXT("LOADING ENGINE & GAMEPLAY ABILITY MODULES");
					}
					else
					{
						PhaseCopy = TEXT("PREPARING D3D11 SHADERS & HANGAR ASSETS");
					}
					ItemCopy = FString::Printf(TEXT("UObject Subsystem (%d active objects)"), CurObjects);
				}

				FString BarInsideText = FString::Printf(TEXT("%s  —  %d%%  (%d OBJECTS)"), *PhaseCopy, MaxPctReached, CurObjects);
				FString SubBarText = FString::Printf(TEXT("LOADING: %s   [PKGS: %d | ASSETS: %d | %.1fs]"), *ItemCopy, Pkgs, Assets, Elapsed);

				HDC hdcWnd = GetDC(hSplash);
				if (hdcWnd)
				{
					HDC hdcMem = CreateCompatibleDC(hdcWnd);
					HBITMAP hbmMem = CreateCompatibleBitmap(hdcWnd, RegionW, RegionH);
					HGDIOBJ hOldBmp = SelectObject(hdcMem, hbmMem);

					// 1. Clear background & draw matching 40px tactical grid lines
					RECT FullRect{ 0, 0, RegionW, RegionH };
					FillRect(hdcMem, &FullRect, hBgBrush);

					HGDIOBJ hOldPen = SelectObject(hdcMem, hGridPen);
					for (int32 gx = 80 - RegionX; gx < RegionW; gx += 40)
					{
						if (gx >= 0)
						{
							MoveToEx(hdcMem, gx, 0, NULL);
							LineTo(hdcMem, gx, RegionH);
						}
					}
					for (int32 gy = 400 - RegionY; gy < RegionH; gy += 40)
					{
						if (gy >= 0)
						{
							MoveToEx(hdcMem, 0, gy, NULL);
							LineTo(hdcMem, RegionW, gy);
						}
					}
					SelectObject(hdcMem, hOldPen);

					// 2. Draw progress bar outer cyan border & dark trough
					RECT OuterBar{ BarX, BarY, BarX + BarW, BarY + BarH };
					FillRect(hdcMem, &OuterBar, hBorderBrush);

					RECT InnerTrough{ BarX + 1, BarY + 1, BarX + BarW - 1, BarY + BarH - 1 };
					FillRect(hdcMem, &InnerTrough, hTroughBrush);

					// 3. Draw live advancing yellow/amber progress bar fill
					const int32 MaxFillW = BarW - 4;
					const int32 FillW = FMath::Clamp((MaxFillW * MaxPctReached) / 100, 6, MaxFillW);
					RECT FillRectArea{ BarX + 2, BarY + 2, BarX + 2 + FillW, BarY + BarH - 2 };
					FillRect(hdcMem, &FillRectArea, hFillBrush);

					// Top specular bevel on yellow bar
					RECT BevelRect{ BarX + 2, BarY + 2, BarX + 2 + FillW, BarY + 5 };
					FillRect(hdcMem, &BevelRect, hHighlightBrush);

					// Leading edge bright cursor
					if (FillW > 4)
					{
						RECT TipRect{ BarX + 2 + FillW - 3, BarY + 2, BarX + 2 + FillW, BarY + BarH - 2 };
						FillRect(hdcMem, &TipRect, hTipBrush);
					}

					SetBkMode(hdcMem, TRANSPARENT);

					// 4. Draw text INSIDE the progress bar (with 1px dark outline for 100% legibility over yellow & dark)
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

					// 5. Draw live package/asset loading info BENEATH the progress bar
					SelectObject(hdcMem, hFontSub);
					RECT SubTextRect{ 4, BarY + BarH + 6, RegionW - 4, RegionH - 2 };
					SetTextColor(hdcMem, RGB(0, 229, 255));
					DrawTextW(hdcMem, *SubBarText, SubBarText.Len(), &SubTextRect, DT_CENTER | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);

					SelectObject(hdcMem, hOldFont);

					// Atomic blit to splash window
					BitBlt(hdcWnd, RegionX, RegionY, RegionW, RegionH, hdcMem, 0, 0, SRCCOPY);

					SelectObject(hdcMem, hOldBmp);
					DeleteObject(hbmMem);
					DeleteDC(hdcMem);
					ReleaseDC(hSplash, hdcWnd);
				}
			}
			else if (GWorldLoaded.load())
			{
				// Splash window has closed and world is loaded; exit worker thread cleanly
				break;
			}

			Sleep(35);
		}

		if (hFontBar && hFontBar != GetStockObject(DEFAULT_GUI_FONT)) DeleteObject(hFontBar);
		if (hFontSub && hFontSub != GetStockObject(DEFAULT_GUI_FONT)) DeleteObject(hFontSub);
		DeleteObject(hBgBrush);
		DeleteObject(hTroughBrush);
		DeleteObject(hBorderBrush);
		DeleteObject(hFillBrush);
		DeleteObject(hHighlightBrush);
		DeleteObject(hTipBrush);
		DeleteObject(hGridPen);
	}
}
#include "Windows/HideWindowsPlatformTypes.h"
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
		NobleFermiSplashProgress::GBaselineObjectCount = GUObjectArray.GetObjectArrayNum();
		NobleFermiSplashProgress::GStopThread = false;
		NobleFermiSplashProgress::GWorldLoaded = false;

		FCoreUObjectDelegates::PackageCreatedForLoad.AddStatic(&NobleFermiSplashProgress::OnPackageCreated);
		FCoreUObjectDelegates::OnAssetLoaded.AddStatic(&NobleFermiSplashProgress::OnAssetLoaded);
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
