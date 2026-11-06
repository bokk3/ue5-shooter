#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <mmsystem.h>
#include <shellapi.h>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>
#include <fstream>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "shell32.lib")

namespace Launcher
{
	struct Vec2
	{
		float X, Y;
		Vec2() : X(0.0f), Y(0.0f) {}
		Vec2(float inX, float inY) : X(inX), Y(inY) {}
	};

	struct DustMote
	{
		float X, Y;
		float Size;
		float DriftX, FallDist;
		float Duration, Delay, Alpha;
		uint8_t R, G, B;
	};

	inline void BlendPixelAdd(uint32_t* Target, int32_t W, int32_t H, int32_t X, int32_t Y, uint8_t R, uint8_t G, uint8_t B, uint8_t Alpha)
	{
		if (X < 0 || X >= W || Y < 0 || Y >= H || Alpha == 0) return;
		int32_t Idx = Y * W + X;
		uint32_t Dst = Target[Idx];

		uint32_t DstB = (Dst & 0xFF);
		uint32_t DstG = ((Dst >> 8) & 0xFF);
		uint32_t DstR = ((Dst >> 16) & 0xFF);
		uint32_t DstA = ((Dst >> 24) & 0xFF);

		float fA = static_cast<float>(Alpha) / 255.0f;
		uint32_t PreR = static_cast<uint32_t>(R * fA);
		uint32_t PreG = static_cast<uint32_t>(G * fA);
		uint32_t PreB = static_cast<uint32_t>(B * fA);

		uint32_t OutR = std::min(255u, DstR + PreR);
		uint32_t OutG = std::min(255u, DstG + PreG);
		uint32_t OutB = std::min(255u, DstB + PreB);
		uint32_t OutA = std::min(255u, DstA + Alpha);

		Target[Idx] = (OutA << 24) | (OutR << 16) | (OutG << 8) | OutB;
	}

	inline void DrawGlowCircle(uint32_t* Target, int32_t W, int32_t H, float CX, float CY, float Radius, uint8_t R, uint8_t G, uint8_t B, float MaxAlpha = 1.0f)
	{
		int32_t MinX = std::clamp(static_cast<int32_t>(CX - Radius), 0, W - 1);
		int32_t MaxX = std::clamp(static_cast<int32_t>(CX + Radius + 1), 0, W - 1);
		int32_t MinY = std::clamp(static_cast<int32_t>(CY - Radius), 0, H - 1);
		int32_t MaxY = std::clamp(static_cast<int32_t>(CY + Radius + 1), 0, H - 1);
		float RadSq = Radius * Radius;

		for (int32_t Y = MinY; Y <= MaxY; ++Y)
		{
			for (int32_t X = MinX; X <= MaxX; ++X)
			{
				float DistSq = (X - CX) * (X - CX) + (Y - CY) * (Y - CY);
				if (DistSq < RadSq)
				{
					float Dist = std::sqrt(DistSq);
					float Falloff = 1.0f - (Dist / Radius);
					Falloff = Falloff * Falloff;
					uint8_t A = static_cast<uint8_t>(std::clamp(Falloff * MaxAlpha * 255.0f, 0.0f, 255.0f));
					if (A > 0)
					{
						BlendPixelAdd(Target, W, H, X, Y, R, G, B, A);
					}
				}
			}
		}
	}

	inline void DrawLineAA(uint32_t* Target, int32_t W, int32_t H, float X0, float Y0, float X1, float Y1, float Thickness, uint8_t R, uint8_t G, uint8_t B, float Alpha = 1.0f)
	{
		float DX = X1 - X0;
		float DY = Y1 - Y0;
		float Len = std::sqrt(DX * DX + DY * DY);
		if (Len < 0.001f) return;
		int32_t Steps = static_cast<int32_t>(Len * 2.0f) + 1;
		float HalfThick = std::max(0.75f, Thickness * 0.5f);

		for (int32_t i = 0; i <= Steps; ++i)
		{
			float t = static_cast<float>(i) / static_cast<float>(Steps);
			float CurX = X0 + DX * t;
			float CurY = Y0 + DY * t;
			DrawGlowCircle(Target, W, H, CurX, CurY, HalfThick, R, G, B, Alpha);
		}
	}

	std::wstring GetExeDirectory()
	{
		wchar_t Buffer[MAX_PATH] = { 0 };
		GetModuleFileNameW(NULL, Buffer, MAX_PATH);
		std::wstring Path(Buffer);
		size_t Pos = Path.find_last_of(L"\\/");
		return (Pos != std::wstring::npos) ? Path.substr(0, Pos) : L"";
	}

	bool FileExists(const std::wstring& Path)
	{
		DWORD Attr = GetFileAttributesW(Path.c_str());
		return (Attr != INVALID_FILE_ATTRIBUTES && !(Attr & FILE_ATTRIBUTE_DIRECTORY));
	}

	std::wstring FindRelativeFile(const std::wstring& RelativeSubPath)
	{
		const std::wstring ExeDir = GetExeDirectory();
		const std::wstring Candidates[] = {
			ExeDir + L"\\" + RelativeSubPath,
			ExeDir + L"\\..\\" + RelativeSubPath,
			ExeDir + L"\\..\\..\\" + RelativeSubPath,
			ExeDir + L"\\..\\..\\..\\" + RelativeSubPath,
			ExeDir + L"\\NobleFermi\\" + RelativeSubPath,
			ExeDir + L"\\..\\NobleFermi\\" + RelativeSubPath,
			ExeDir + L"\\..\\..\\NobleFermi\\" + RelativeSubPath,
			ExeDir + L"\\..\\..\\..\\NobleFermi\\" + RelativeSubPath,
			RelativeSubPath
		};
		for (const auto& C : Candidates)
		{
			wchar_t FullPath[MAX_PATH] = { 0 };
			if (GetFullPathNameW(C.c_str(), MAX_PATH, FullPath, NULL))
			{
				if (FileExists(FullPath))
				{
					return std::wstring(FullPath);
				}
			}
		}
		return L"";
	}

	struct FChildWndCtx
	{
		DWORD TargetPid;
		HWND FoundGameHwnd;
	};

	static BOOL CALLBACK EnumChildWndProc(HWND hWnd, LPARAM lParam)
	{
		FChildWndCtx* Ctx = reinterpret_cast<FChildWndCtx*>(lParam);
		DWORD WndPid = 0;
		GetWindowThreadProcessId(hWnd, &WndPid);
		if (WndPid == Ctx->TargetPid)
		{
			wchar_t ClassName[128] = { 0 };
			if (GetClassNameW(hWnd, ClassName, 127) > 0)
			{
				if (wcscmp(ClassName, L"UnrealWindow") == 0)
				{
					Ctx->FoundGameHwnd = hWnd;
					return 0; // Found main Unreal game window!
				}
			}
		}
		return 1;
	}

	HWND FindChildGameWindow(DWORD TargetPid)
	{
		if (TargetPid == 0) return NULL;
		FChildWndCtx Ctx{ TargetPid, NULL };
		EnumWindows(&EnumChildWndProc, reinterpret_cast<LPARAM>(&Ctx));
		return Ctx.FoundGameHwnd;
	}

	static BOOL CALLBACK EnumSuppressWndProc(HWND hWnd, LPARAM lParam)
	{
		DWORD TargetPid = static_cast<DWORD>(lParam);
		DWORD WndPid = 0;
		GetWindowThreadProcessId(hWnd, &WndPid);
		if (WndPid == TargetPid && IsWindowVisible(hWnd))
		{
			wchar_t ClassName[128] = { 0 };
			if (GetClassNameW(hWnd, ClassName, 127) > 0)
			{
				if (wcscmp(ClassName, L"UnrealWindow") == 0 || wcscmp(ClassName, L"SplashScreenClass") == 0)
				{
					ShowWindow(hWnd, SW_HIDE);
				}
			}
		}
		return 1;
	}

	void SuppressAllChildWindows(DWORD TargetPid)
	{
		if (TargetPid == 0) return;
		EnumWindows(&EnumSuppressWndProc, static_cast<LPARAM>(TargetPid));
	}

	void SaveBitmapToFile(HDC hdc, HBITMAP hBitmap, int32_t w, int32_t h, const wchar_t* Filename)
	{
		BITMAPINFO bmi = { 0 };
		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth = w;
		bmi.bmiHeader.biHeight = h;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 32;
		bmi.bmiHeader.biCompression = BI_RGB;

		std::vector<uint8_t> Pixels(w * h * 4);
		GetDIBits(hdc, hBitmap, 0, h, Pixels.data(), &bmi, DIB_RGB_COLORS);

		BITMAPFILEHEADER bfh = { 0 };
		bfh.bfType = 0x4D42;
		bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
		bfh.bfSize = bfh.bfOffBits + static_cast<DWORD>(Pixels.size());

		HANDLE hFile = CreateFileW(Filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			DWORD Written = 0;
			WriteFile(hFile, &bfh, sizeof(bfh), &Written, NULL);
			WriteFile(hFile, &bmi.bmiHeader, sizeof(bmi.bmiHeader), &Written, NULL);
			WriteFile(hFile, Pixels.data(), static_cast<DWORD>(Pixels.size()), &Written, NULL);
			CloseHandle(hFile);
		}
	}
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	using namespace Launcher;

	// Ensure D3D11 & Engine DLL directory is in PATH for smooth Windows dynamic loading
	const wchar_t* UeBinaries = L"D:\\epuc\\UE_5.8\\Engine\\Binaries\\Win64";
	const wchar_t* DotNetRoot = L"D:\\epuc\\UE_5.8\\Engine\\Binaries\\ThirdParty\\DotNet\\10.0\\win-x64";
	
	wchar_t CurrentPathEnv[32768] = { 0 };
	GetEnvironmentVariableW(L"PATH", CurrentPathEnv, 32767);
	std::wstring NewPathEnv = std::wstring(UeBinaries) + L";" + DotNetRoot + L";" + CurrentPathEnv;
	SetEnvironmentVariableW(L"PATH", NewPathEnv.c_str());
	SetEnvironmentVariableW(L"DOTNET_ROOT", DotNetRoot);

	// Locate Game Executable
	std::wstring ExeCandidates[] = {
		FindRelativeFile(L"Packaged\\Windows\\NobleFermi\\Binaries\\Win64\\NobleFermi.exe"),
		FindRelativeFile(L"Binaries\\Win64\\NobleFermi.exe"),
		FindRelativeFile(L"Packaged\\Windows\\NobleFermi.exe"),
		FindRelativeFile(L"NobleFermi.exe")
	};
	std::wstring TargetExePath;
	for (const auto& C : ExeCandidates)
	{
		if (!C.empty() && FileExists(C))
		{
			TargetExePath = C;
			break;
		}
	}

	// =========================================================================
	// PHASE 1: Authentic Transparent Tachyon Studio Hero Splash Animation
	// =========================================================================
	WNDCLASSEXW WcTachyon = { sizeof(WNDCLASSEXW) };
	WcTachyon.lpfnWndProc = &DefWindowProcW;
	WcTachyon.hInstance = hInstance;
	WcTachyon.lpszClassName = L"TachyonStudioTransparentSplashClass";
	RegisterClassExW(&WcTachyon);

	constexpr int32_t WinW = 840;
	constexpr int32_t WinH = 470;
	const int32_t ScreenW = GetSystemMetrics(SM_CXSCREEN);
	const int32_t ScreenH = GetSystemMetrics(SM_CYSCREEN);
	const int32_t WinX = (ScreenW - WinW) / 2;
	const int32_t WinY = (ScreenH - WinH) / 2;

	HWND hWndTachyon = CreateWindowExW(
		WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
		L"TachyonStudioTransparentSplashClass",
		L"Tachyon Game Studio",
		WS_POPUP | WS_VISIBLE,
		WinX, WinY, WinW, WinH,
		NULL, NULL, hInstance, NULL);

	HDC hdcScreen = GetDC(NULL);
	HDC hdcMem = CreateCompatibleDC(hdcScreen);

	BITMAPINFO bmi = { 0 };
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = WinW;
	bmi.bmiHeader.biHeight = -WinH; // Top-down
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	void* pBits = nullptr;
	HBITMAP hBitmap = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
	HGDIOBJ hOldBmp = SelectObject(hdcMem, hBitmap);
	uint32_t* TargetPixels = reinterpret_cast<uint32_t*>(pBits);

	// Load Tachyon transparent logo
	std::wstring LogoPath = FindRelativeFile(L"Content\\Splash\\tachyon_logo_840x470.bgra");
	std::vector<uint8_t> LogoRawBytes;
	if (!LogoPath.empty())
	{
		std::ifstream F(LogoPath, std::ios::binary | std::ios::ate);
		if (F.is_open())
		{
			size_t Sz = F.tellg();
			F.seekg(0, std::ios::beg);
			LogoRawBytes.resize(Sz);
			F.read(reinterpret_cast<char*>(LogoRawBytes.data()), Sz);
		}
	}
	const uint32_t* LogoPixels = (LogoRawBytes.size() >= WinW * WinH * 4) ? reinterpret_cast<const uint32_t*>(LogoRawBytes.data()) : nullptr;

	// Sourced authentic mastered audio (sword scrapes, clash, supersonic bullet flybys, sub-drone)
	std::wstring WavPath = FindRelativeFile(L"Content\\Splash\\tachyon_splash.wav");
	if (!WavPath.empty())
	{
		PlaySoundW(WavPath.c_str(), NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
	}

	// Prepared child game process parameters
	PROCESS_INFORMATION pi = { 0 };
	STARTUPINFOW si = { sizeof(STARTUPINFOW) };
	DWORD ChildPid = 0;
	bool bLaunchedGame = false;

	// Faceted border points
	const float ScaleX = static_cast<float>(WinW) / 1000.0f;
	const float ScaleY = static_cast<float>(WinH) / 560.0f;
	const Vec2 PolyPoints[] = {
		Vec2(60.0f * ScaleX,  20.0f * ScaleY),
		Vec2(940.0f * ScaleX, 20.0f * ScaleY),
		Vec2(980.0f * ScaleX, 60.0f * ScaleY),
		Vec2(980.0f * ScaleX, 500.0f * ScaleY),
		Vec2(940.0f * ScaleX, 540.0f * ScaleY),
		Vec2(60.0f * ScaleX,  540.0f * ScaleY),
		Vec2(20.0f * ScaleX,  500.0f * ScaleY),
		Vec2(20.0f * ScaleX,  60.0f * ScaleY),
		Vec2(60.0f * ScaleX,  20.0f * ScaleY)
	};
	constexpr int32_t NumPts = sizeof(PolyPoints) / sizeof(PolyPoints[0]);

	float SegLengths[NumPts - 1];
	float TotalPerimeter = 0.0f;
	for (int32_t i = 0; i < NumPts - 1; ++i)
	{
		const float dx = PolyPoints[i + 1].X - PolyPoints[i].X;
		const float dy = PolyPoints[i + 1].Y - PolyPoints[i].Y;
		SegLengths[i] = std::sqrt(dx * dx + dy * dy);
		TotalPerimeter += SegLengths[i];
	}

	// 36 atmospheric dust & debris particles
	std::vector<DustMote> DustMotes;
	const uint8_t DustCols[][3] = {
		{ 0, 243, 255 },   // Cyan
		{ 255, 184, 0 },   // Amber
		{ 255, 0, 255 },   // Magenta
		{ 255, 255, 255 }, // White
		{ 255, 102, 0 },   // Orange
		{ 0, 255, 178 }    // Mint
	};
	DustMotes.reserve(36);
	srand(1337);
	for (int32_t i = 0; i < 36; ++i)
	{
		DustMote Mote;
		Mote.X = (0.06f + (static_cast<float>(rand() % 1000) / 1000.0f) * 0.88f) * WinW;
		Mote.Y = (0.15f + (static_cast<float>(rand() % 1000) / 1000.0f) * 0.65f) * WinH;
		Mote.Size = 1.2f + (static_cast<float>(rand() % 25) / 10.0f);
		Mote.DriftX = ((static_cast<float>(rand() % 1000) / 1000.0f) - 0.5f) * 60.0f;
		Mote.FallDist = 45.0f + (static_cast<float>(rand() % 1000) / 1000.0f) * 85.0f;
		Mote.Duration = 1.3f + (static_cast<float>(rand() % 1000) / 1000.0f) * 0.7f;
		Mote.Delay = (static_cast<float>(rand() % 1000) / 1000.0f) * 0.35f;
		Mote.Alpha = 0.5f + (static_cast<float>(rand() % 1000) / 1000.0f) * 0.45f;
		Mote.R = DustCols[i % 6][0];
		Mote.G = DustCols[i % 6][1];
		Mote.B = DustCols[i % 6][2];
		DustMotes.push_back(Mote);
	}

	auto StartTime = std::chrono::high_resolution_clock::now();
	constexpr double TachyonDuration = 4.15;

	while (true)
	{
		auto Now = std::chrono::high_resolution_clock::now();
		double Elapsed = std::chrono::duration<double>(Now - StartTime).count();
		if (Elapsed >= TachyonDuration) break;

		// Launch child game process at 3.0s into Tachyon cinematic (during sheen & dust settle)
		// so it initializes smoothly for Phase 2 without ever showing UnrealWindow during Phase 1!
		if (!bLaunchedGame && Elapsed >= 3.0)
		{
			bLaunchedGame = true;
			if (!TargetExePath.empty())
			{
				std::wstring CmdArgs = L"\"" + TargetExePath + L"\" -nosplash -d3d11 -windowed -ResX=1280 -ResY=720 -log -NoTachyon";
				if (wcslen(lpCmdLine) > 0)
				{
					CmdArgs += L" ";
					CmdArgs += lpCmdLine;
				}

				std::vector<wchar_t> CmdBuffer(CmdArgs.begin(), CmdArgs.end());
				CmdBuffer.push_back(0);

				std::wstring WorkDir = TargetExePath.substr(0, TargetExePath.find_last_of(L"\\/"));

				if (CreateProcessW(
					NULL,
					CmdBuffer.data(),
					NULL, NULL,
					FALSE,
					CREATE_NEW_PROCESS_GROUP,
					NULL,
					WorkDir.c_str(),
					&si, &pi))
				{
					ChildPid = pi.dwProcessId;
				}
			}
		}

		// Suppress any premature child windows during transparent desktop logo
		if (ChildPid != 0)
		{
			SuppressAllChildWindows(ChildPid);
		}

		MSG Msg;
		while (PeekMessageW(&Msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&Msg);
			DispatchMessageW(&Msg);
		}

		memset(pBits, 0, WinW * WinH * sizeof(uint32_t));

		// 1. Logo Artwork (progressive unveil)
		if (LogoPixels)
		{
			const float CutP = std::clamp(static_cast<float>(Elapsed / 2.40), 0.0f, 1.0f);
			const float RevealFrac = (Elapsed < 2.40) ? std::max(0.10f, CutP * 1.10f) : 1.0f;
			const int32_t RevealH = std::clamp(static_cast<int32_t>(std::round(WinH * RevealFrac)), 1, WinH);

			const bool bIsFlash = (Elapsed >= 2.40 && Elapsed <= 2.65);
			const float FlashFrac = bIsFlash ? (1.0f - static_cast<float>((Elapsed - 2.40) / 0.25)) : 0.0f;

			for (int32_t Y = 0; Y < RevealH; ++Y)
			{
				const int32_t RowOffset = Y * WinW;
				for (int32_t X = 0; X < WinW; ++X)
				{
					uint32_t Pix = LogoPixels[RowOffset + X];
					if (Pix == 0) continue;

					if (bIsFlash)
					{
						uint32_t B = (Pix & 0xFF);
						uint32_t G = ((Pix >> 8) & 0xFF);
						uint32_t R = ((Pix >> 16) & 0xFF);
						uint32_t A = ((Pix >> 24) & 0xFF);

						if (A > 0)
						{
							const float FlashBoost = 1.0f + 0.85f * FlashFrac;
							const float WhiteAdd = A * 0.35f * FlashFrac;
							R = std::clamp(static_cast<uint32_t>(R * FlashBoost + WhiteAdd), 0u, static_cast<uint32_t>(A));
							G = std::clamp(static_cast<uint32_t>(G * FlashBoost + WhiteAdd), 0u, static_cast<uint32_t>(A));
							B = std::clamp(static_cast<uint32_t>(B * FlashBoost + WhiteAdd * 1.15f), 0u, static_cast<uint32_t>(A));
							Pix = (A << 24) | (R << 16) | (G << 8) | B;
						}
					}
					TargetPixels[RowOffset + X] = Pix;
				}
			}
		}

		// 2. Laser Cut Border & Spatter (0.0s - 2.40s)
		if (Elapsed < 2.40)
		{
			const float CutP = std::clamp(static_cast<float>(Elapsed / 2.40), 0.0f, 1.0f);
			const float TargetDist = CutP * TotalPerimeter;

			float AccDist = 0.0f;
			Vec2 HeadPt = PolyPoints[0];

			for (int32_t i = 0; i < NumPts - 1; ++i)
			{
				const Vec2& P1 = PolyPoints[i];
				const Vec2& P2 = PolyPoints[i + 1];

				if (AccDist + SegLengths[i] <= TargetDist)
				{
					DrawLineAA(TargetPixels, WinW, WinH, P1.X, P1.Y, P2.X, P2.Y, 9.0f, 0, 243, 255, 0.35f);
					DrawLineAA(TargetPixels, WinW, WinH, P1.X, P1.Y, P2.X, P2.Y, 3.5f, 0, 243, 255, 0.85f);
					DrawLineAA(TargetPixels, WinW, WinH, P1.X, P1.Y, P2.X, P2.Y, 1.5f, 255, 255, 255, 1.0f);
					HeadPt = P2;
				}
				else if (AccDist < TargetDist)
				{
					const float Frac = (TargetDist - AccDist) / SegLengths[i];
					const Vec2 CurPt(P1.X + (P2.X - P1.X) * Frac, P1.Y + (P2.Y - P1.Y) * Frac);
					DrawLineAA(TargetPixels, WinW, WinH, P1.X, P1.Y, CurPt.X, CurPt.Y, 9.0f, 0, 243, 255, 0.35f);
					DrawLineAA(TargetPixels, WinW, WinH, P1.X, P1.Y, CurPt.X, CurPt.Y, 3.5f, 0, 243, 255, 0.85f);
					DrawLineAA(TargetPixels, WinW, WinH, P1.X, P1.Y, CurPt.X, CurPt.Y, 1.5f, 255, 255, 255, 1.0f);
					HeadPt = CurPt;
					break;
				}
				AccDist += SegLengths[i];
			}

			// Torch head
			DrawGlowCircle(TargetPixels, WinW, WinH, HeadPt.X, HeadPt.Y, 18.0f, 0, 243, 255, 0.45f);
			DrawGlowCircle(TargetPixels, WinW, WinH, HeadPt.X, HeadPt.Y, 8.0f, 255, 0, 255, 0.90f);
			DrawGlowCircle(TargetPixels, WinW, WinH, HeadPt.X, HeadPt.Y, 4.5f, 255, 255, 255, 1.0f);

			// Directional sparks
			DrawLineAA(TargetPixels, WinW, WinH, HeadPt.X, HeadPt.Y, HeadPt.X - 14.0f, HeadPt.Y - 12.0f, 2.0f, 255, 184, 0, 0.90f);
			DrawLineAA(TargetPixels, WinW, WinH, HeadPt.X, HeadPt.Y, HeadPt.X + 16.0f, HeadPt.Y - 15.0f, 1.8f, 255, 255, 255, 0.95f);
			DrawLineAA(TargetPixels, WinW, WinH, HeadPt.X, HeadPt.Y, HeadPt.X - 8.0f,  HeadPt.Y + 18.0f, 2.2f, 0, 243, 255, 0.85f);
			DrawLineAA(TargetPixels, WinW, WinH, HeadPt.X, HeadPt.Y, HeadPt.X + 14.0f, HeadPt.Y + 14.0f, 1.8f, 255, 0, 127, 0.90f);
			DrawLineAA(TargetPixels, WinW, WinH, HeadPt.X, HeadPt.Y, HeadPt.X - 18.0f, HeadPt.Y + 4.0f,  1.5f, 255, 184, 0, 0.75f);
		}
		// 3. Thermal Cooldown Seam & Specular Sheen (2.40s - 3.75s)
		else
		{
			const float CoolP = std::clamp(static_cast<float>((Elapsed - 2.40) / 1.35), 0.0f, 1.0f);
			uint8_t SeamR = 255, SeamG = 255, SeamB = 255;
			float SeamAlpha = 1.0f;
			float GlowAlpha = 0.0f;

			if (CoolP < 0.25f)
			{
				const float t = CoolP / 0.25f;
				SeamR = 255;
				SeamG = static_cast<uint8_t>(255 - t * (255 - 184));
				SeamB = static_cast<uint8_t>(255 - t * 255);
				SeamAlpha = 1.0f - t * 0.10f;
				GlowAlpha = (1.0f - t) * 0.5f;
			}
			else if (CoolP < 0.65f)
			{
				const float t = (CoolP - 0.25f) / 0.40f;
				SeamR = static_cast<uint8_t>(255 * (1.0f - t));
				SeamG = static_cast<uint8_t>(184 + t * (243 - 184));
				SeamB = static_cast<uint8_t>(t * 255);
				SeamAlpha = 0.90f - t * 0.45f;
			}
			else
			{
				const float t = (CoolP - 0.65f) / 0.35f;
				SeamR = 0;
				SeamG = static_cast<uint8_t>(243 - t * (243 - 85));
				SeamB = 255;
				SeamAlpha = 0.45f * (1.0f - t);
			}

			if (SeamAlpha > 0.02f)
			{
				for (int32_t i = 0; i < NumPts - 1; ++i)
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

				for (int32_t Y = 0; Y < WinH; ++Y)
				{
					const float SkewX = ((static_cast<float>(Y) / static_cast<float>(WinH)) - 0.5f) * 115.0f;
					const float RowCenterX = SheenCenterX + SkewX;
					const int32_t MinX = std::clamp(static_cast<int32_t>(RowCenterX - SheenHalfW), 0, WinW - 1);
					const int32_t MaxX = std::clamp(static_cast<int32_t>(RowCenterX + SheenHalfW), 0, WinW - 1);

					for (int32_t X = MinX; X <= MaxX; ++X)
					{
						const float Offset = static_cast<float>(X) - RowCenterX;
						const float Dist = std::abs(Offset);
						if (Dist < SheenHalfW)
						{
							float BeamFrac = 1.0f - (Dist / SheenHalfW);
							BeamFrac = BeamFrac * BeamFrac;

							if (Offset < 0)
							{
								const uint8_t CyanA = static_cast<uint8_t>(BeamFrac * 125.0f);
								if (CyanA > 0) BlendPixelAdd(TargetPixels, WinW, WinH, X, Y, 0, 243, 255, CyanA);
							}
							else
							{
								const uint8_t MagA = static_cast<uint8_t>(BeamFrac * 105.0f);
								if (MagA > 0) BlendPixelAdd(TargetPixels, WinW, WinH, X, Y, 255, 0, 255, MagA);
							}

							if (Dist < SheenHalfW * 0.26f)
							{
								const float CoreFrac = 1.0f - (Dist / (SheenHalfW * 0.26f));
								const uint8_t CoreA = static_cast<uint8_t>(CoreFrac * CoreFrac * 210.0f);
								if (CoreA > 0) BlendPixelAdd(TargetPixels, WinW, WinH, X, Y, 255, 255, 255, CoreA);
							}
						}
					}
				}

				// Concentric Shockwave
				const float CenterX = WinW * 0.5f;
				const float CenterY = WinH * 0.5f;
				const float WaveRadius = 25.0f + SheenP * 290.0f;
				const float WaveAlpha = std::max(0.0f, (1.0f - SheenP * SheenP) * 0.85f);
				if (WaveAlpha > 0.05f)
				{
					uint8_t WaveR = 0, WaveG = 243, WaveB = 255;
					if (SheenP > 0.35f)
					{
						const float wt = std::clamp((SheenP - 0.35f) / 0.45f, 0.0f, 1.0f);
						WaveR = static_cast<uint8_t>(wt * 255.0f);
						WaveG = static_cast<uint8_t>((1.0f - wt) * 243.0f);
						WaveB = 255;
					}
					constexpr int32_t RingSteps = 56;
					for (int32_t r = 0; r < RingSteps; ++r)
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

				// Central Rotating Diamond Starburst
				const float FlareT = std::clamp((SheenP - 0.18f) / 0.62f, 0.0f, 1.0f);
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
					DrawGlowCircle(TargetPixels, WinW, WinH, CenterX, CenterY, 15.0f * FlareScale, 255, 255, 255, FlareAlpha);
				}

				// 36 Atmospheric Dust Particles Settling
				for (const auto& Mote : DustMotes)
				{
					const float ParticleT = std::clamp(static_cast<float>((Elapsed - 2.65 - Mote.Delay) / Mote.Duration), 0.0f, 1.0f);
					if (ParticleT > 0.0f && ParticleT < 1.0f)
					{
						const float CurX = Mote.X + Mote.DriftX * ParticleT;
						const float CurY = Mote.Y + Mote.FallDist * (ParticleT * ParticleT);

						float MoteFade = (ParticleT < 0.15f) ? (ParticleT / 0.15f) : ((ParticleT > 0.65f) ? (1.0f - (ParticleT - 0.65f) / 0.35f) : 1.0f);
						const float CurAlpha = Mote.Alpha * MoteFade * 0.90f;
						const float CurSize = Mote.Size * (1.15f - ParticleT * 0.55f);
						DrawGlowCircle(TargetPixels, WinW, WinH, CurX, CurY, CurSize, Mote.R, Mote.G, Mote.B, CurAlpha);
					}
				}
			}
		}

		// Fade out to black (3.75s - 4.15s)
		BYTE GlobalAlpha = 255;
		if (Elapsed > 3.75)
		{
			const float FadeFrac = std::clamp(static_cast<float>((Elapsed - 3.75) / 0.40), 0.0f, 1.0f);
			GlobalAlpha = static_cast<BYTE>((1.0f - FadeFrac) * 255.0f);
		}

		POINT ptSrc = { 0, 0 };
		POINT ptDst = { WinX, WinY };
		SIZE sizeWnd = { WinW, WinH };
		BLENDFUNCTION blend = { AC_SRC_OVER, 0, GlobalAlpha, AC_SRC_ALPHA };
		UpdateLayeredWindow(hWndTachyon, hdcScreen, &ptDst, &sizeWnd, hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);

		Sleep(16);
	}

	PlaySoundW(NULL, NULL, 0);

	SelectObject(hdcMem, hOldBmp);
	DeleteObject(hBitmap);
	DeleteDC(hdcMem);
	ReleaseDC(NULL, hdcScreen);
	DestroyWindow(hWndTachyon);
	UnregisterClassW(L"TachyonStudioTransparentSplashClass", hInstance);

	// =========================================================================
	// PHASE 2: Rectangular Noble Fermi Loading Splash & Live Loading Bar (800x450)
	// =========================================================================
	WNDCLASSEXW WcRect = { 0 };
	WcRect.cbSize = sizeof(WNDCLASSEXW);
	WcRect.style = CS_HREDRAW | CS_VREDRAW;
	WcRect.lpfnWndProc = &DefWindowProcW;
	WcRect.hInstance = hInstance;
	WcRect.hCursor = LoadCursor(NULL, IDC_ARROW);
	WcRect.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	WcRect.lpszClassName = L"NobleFermiActiveSplashClass";
	RegisterClassExW(&WcRect);

	constexpr int32_t WndW = 800;
	constexpr int32_t WndH = 450;
	const int32_t WndX = (ScreenW - WndW) / 2;
	const int32_t WndY = (ScreenH - WndH) / 2;

	HWND hWndRect = CreateWindowExW(
		WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
		L"NobleFermiActiveSplashClass",
		L"Noble Fermi Loading",
		WS_POPUP,
		WndX, WndY, WndW, WndH,
		NULL, NULL, hInstance, NULL);

	std::wstring SplashBmpPath = FindRelativeFile(L"Content\\Splash\\Splash.bmp");
	HBITMAP hBmpSplash = (!SplashBmpPath.empty()) ? (HBITMAP)LoadImageW(NULL, SplashBmpPath.c_str(), IMAGE_BITMAP, WndW, WndH, LR_LOADFROMFILE) : NULL;

	ShowWindow(hWndRect, SW_SHOW);
	UpdateWindow(hWndRect);
	SetWindowPos(hWndRect, HWND_TOPMOST, WndX, WndY, WndW, WndH, SWP_SHOWWINDOW);

	constexpr int32_t BarX = 140;
	constexpr int32_t BarY = 386;
	constexpr int32_t BarW = 520;
	constexpr int32_t BarH = 18;

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

	HDC hdcWnd = GetDC(hWndRect);
	HDC hdcMemRect = CreateCompatibleDC(hdcWnd);
	HBITMAP hbmMemRect = CreateCompatibleBitmap(hdcWnd, WndW, WndH);
	HGDIOBJ hOldMemBmp = SelectObject(hdcMemRect, hbmMemRect);

	HDC hdcSplashBmp = CreateCompatibleDC(hdcWnd);
	HGDIOBJ hOldSplashBmp = hBmpSplash ? SelectObject(hdcSplashBmp, hBmpSplash) : NULL;

	auto Phase2StartTime = std::chrono::high_resolution_clock::now();
	int32_t MaxPctReached = 12;
	HWND FoundGameWindow = NULL;
	double FinishStartTime = -1.0;
	int32_t FinishStartPct = 82;
	bool bSavedPhase2Snapshot = false;

	while (true)
	{
		// Suppress any premature game window during loading
		if (ChildPid != 0)
		{
			SuppressAllChildWindows(ChildPid);
			if (!FoundGameWindow)
			{
				FoundGameWindow = FindChildGameWindow(ChildPid);
			}
		}

		MSG Msg;
		while (PeekMessageW(&Msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&Msg);
			DispatchMessageW(&Msg);
		}

		auto CurT = std::chrono::high_resolution_clock::now();
		double Elapsed = std::chrono::duration<double>(CurT - Phase2StartTime).count();

		float BaseFrac = 0.12f;
		std::wstring PhaseStr = L"INITIALIZING D3D11 RHI & SHADER PIPELINE";
		std::wstring ItemStr = L"Core Engine Subsystems & Vertex Layouts";

		if (Elapsed < 0.8)
		{
			BaseFrac = 0.12f + static_cast<float>(Elapsed / 0.8) * 0.28f;
			PhaseStr = L"INITIALIZING D3D11 RHI & SHADER PIPELINE";
			ItemStr = L"PSO Bytecode & Pipeline State Subsystems";
		}
		else if (Elapsed < 1.8)
		{
			BaseFrac = 0.40f + static_cast<float>((Elapsed - 0.8) / 1.0) * 0.30f;
			PhaseStr = L"STREAMING CONTENT PACKAGES & PBR MATERIALS";
			ItemStr = L"Operator Skeletal Rig, Armor & Weapon Finishes";
		}
		else if (Elapsed < 2.8)
		{
			BaseFrac = 0.70f + static_cast<float>((Elapsed - 1.8) / 1.0) * 0.20f;
			PhaseStr = L"CONSTRUCTING HANGAR DIORAMA & VOLUMETRIC LIGHTING";
			ItemStr = L"Spawning Turntable Stage, Table Displays & Cold Fog";
		}
		else
		{
			BaseFrac = 0.90f + static_cast<float>(std::min(0.06, (Elapsed - 2.8) * 0.02));
			PhaseStr = L"PREPARING COMMAND CENTER & SLATE UI";
			ItemStr = L"Finalizing Hangar Render Targets & Post-Processing";
		}

		int32_t TargetPct = std::clamp(static_cast<int32_t>(std::round(BaseFrac * 100.0f)), 12, 96);

		// If child game window has been created by Slate, trigger rapid, smooth completion to 100%
		if (FoundGameWindow != NULL)
		{
			if (FinishStartTime < 0.0)
			{
				FinishStartTime = Elapsed;
				FinishStartPct = std::max(MaxPctReached, 82);
			}
			double FinishElapsed = Elapsed - FinishStartTime;
			float FinishFrac = std::clamp(static_cast<float>(FinishElapsed / 0.55), 0.0f, 1.0f);
			TargetPct = std::clamp(static_cast<int32_t>(std::round(FinishStartPct + (100 - FinishStartPct) * FinishFrac)), FinishStartPct, 100);
			PhaseStr = L"DEPLOYMENT READY // ENTERING COMMAND CENTER";
			ItemStr = L"MenuMap Loaded [128-HZ AUTHORITATIVE ENGINE READY]";

			if (FinishFrac >= 1.0f && FinishElapsed > 0.65)
			{
				break; // Smooth completion achieved
			}
		}

		if (TargetPct > MaxPctReached)
		{
			MaxPctReached = TargetPct;
		}

		// Draw splash artwork
		if (hBmpSplash)
		{
			BitBlt(hdcMemRect, 0, 0, WndW, WndH, hdcSplashBmp, 0, 0, SRCCOPY);
		}
		else
		{
			RECT FullRect{ 0, 0, WndW, WndH };
			FillRect(hdcMemRect, &FullRect, hBgBrush);
		}

		// Progress bar trough & border
		RECT OuterBar{ BarX, BarY, BarX + BarW, BarY + BarH };
		FillRect(hdcMemRect, &OuterBar, hBorderBrush);

		RECT InnerTrough{ BarX + 1, BarY + 1, BarX + BarW - 1, BarY + BarH - 1 };
		FillRect(hdcMemRect, &InnerTrough, hTroughBrush);

		// Gold fill
		const int32_t MaxFillW = BarW - 4;
		const int32_t FillW = std::clamp((MaxFillW * MaxPctReached) / 100, 6, MaxFillW);
		RECT FillRectArea{ BarX + 2, BarY + 2, BarX + 2 + FillW, BarY + BarH - 2 };
		FillRect(hdcMemRect, &FillRectArea, hFillBrush);

		RECT BevelRect{ BarX + 2, BarY + 2, BarX + 2 + FillW, BarY + 5 };
		FillRect(hdcMemRect, &BevelRect, hHighlightBrush);

		if (FillW > 4)
		{
			RECT TipRect{ BarX + 2 + FillW - 3, BarY + 2, BarX + 2 + FillW, BarY + BarH - 2 };
			FillRect(hdcMemRect, &TipRect, hTipBrush);
		}

		// Subtext & label
		RECT SubBgRect{ 100, BarY + BarH + 4, WndW - 100, BarY + BarH + 34 };
		FillRect(hdcMemRect, &SubBgRect, hBgBrush);

		SetBkMode(hdcMemRect, TRANSPARENT);

		// Draw inside bar text
		wchar_t BarTextBuf[128] = { 0 };
		swprintf_s(BarTextBuf, L"%s  —  %d%%", PhaseStr.c_str(), MaxPctReached);

		HGDIOBJ hOldFont = SelectObject(hdcMemRect, hFontBar);
		RECT BarTextRect{ BarX + 6, BarY + 1, BarX + BarW - 6, BarY + BarH - 1 };
		SetTextColor(hdcMemRect, RGB(3, 7, 18));
		const int32_t Offsets[8][2] = { {-1,0}, {1,0}, {0,-1}, {0,1}, {-1,-1}, {1,-1}, {-1,1}, {1,1} };
		for (int32_t i = 0; i < 8; ++i)
		{
			RECT OutR{ BarTextRect.left + Offsets[i][0], BarTextRect.top + Offsets[i][1], BarTextRect.right + Offsets[i][0], BarTextRect.bottom + Offsets[i][1] };
			DrawTextW(hdcMemRect, BarTextBuf, -1, &OutR, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		}
		SetTextColor(hdcMemRect, RGB(255, 255, 255));
		DrawTextW(hdcMemRect, BarTextBuf, -1, &BarTextRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

		// Sub item text
		SelectObject(hdcMemRect, hFontSub);
		SetTextColor(hdcMemRect, RGB(0, 229, 255));
		RECT SubItemRect{ 100, BarY + BarH + 5, WndW - 100, BarY + BarH + 28 };
		DrawTextW(hdcMemRect, ItemStr.c_str(), -1, &SubItemRect, DT_CENTER | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);
		SelectObject(hdcMemRect, hOldFont);

		BitBlt(hdcWnd, 0, 0, WndW, WndH, hdcMemRect, 0, 0, SRCCOPY);
		SetWindowPos(hWndRect, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

		if (!bSavedPhase2Snapshot && MaxPctReached >= 45)
		{
			SaveBitmapToFile(hdcMemRect, hbmMemRect, WndW, WndH, L"C:\\Users\\Boris\\.gemini\\antigravity\\brain\\c135ce77-4bef-4380-b69a-99d73fc7b5a7\\phase2_noble_fermi_loading_bar.bmp");
			bSavedPhase2Snapshot = true;
		}

		Sleep(16);
	}

	// =========================================================================
	// PHASE 3: Seamless Reveal of Main Game Window into Hangar Command Center
	// =========================================================================
	SelectObject(hdcMemRect, hOldMemBmp);
	DeleteObject(hbmMemRect);
	DeleteDC(hdcMemRect);

	if (hBmpSplash)
	{
		SelectObject(hdcSplashBmp, hOldSplashBmp);
		DeleteObject(hBmpSplash);
	}
	DeleteDC(hdcSplashBmp);
	ReleaseDC(hWndRect, hdcWnd);

	if (hFontBar && hFontBar != GetStockObject(DEFAULT_GUI_FONT)) DeleteObject(hFontBar);
	if (hFontSub && hFontSub != GetStockObject(DEFAULT_GUI_FONT)) DeleteObject(hFontSub);
	DeleteObject(hBgBrush);
	DeleteObject(hTroughBrush);
	DeleteObject(hBorderBrush);
	DeleteObject(hFillBrush);
	DeleteObject(hHighlightBrush);
	DeleteObject(hTipBrush);
	DeleteObject(hGridPen);

	DestroyWindow(hWndRect);
	UnregisterClassW(L"NobleFermiActiveSplashClass", hInstance);

	// Reveal child game window
	if (ChildPid != 0)
	{
		HWND hGame = FoundGameWindow ? FoundGameWindow : FindChildGameWindow(ChildPid);
		for (int32_t retry = 0; retry < 60 && !hGame; ++retry)
		{
			hGame = FindChildGameWindow(ChildPid);
			if (hGame) break;
			Sleep(50);
		}

		if (hGame && IsWindow(hGame))
		{
			ShowWindow(hGame, SW_SHOWNORMAL);
			UpdateWindow(hGame);
			SetForegroundWindow(hGame);
		}
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}

	return 0;
}
