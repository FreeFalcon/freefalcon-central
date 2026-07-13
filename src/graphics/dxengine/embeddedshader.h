// Artscout - 2026: FFEmu.hlsl source loader shared by the D3D11 and D3D12 renderers.
//
// The shader used to be compiled straight from an external file (D3DCompileFromFile), so the game required
// shaders\FFEmu.hlsl to be shipped next to the exe. Now the source is BAKED INTO THE EXE as the RCDATA
// resource "FFEMU_HLSL" (see src\Falclib\Include\falcon4.rc) and compiled from memory.
//
// Load order (LoadFFEmuShaderSource):
//   1. EXTERNAL  shaderDir\FFEmu.hlsl  -- if it exists. Keeps the runtime-tunable workflow: drop an edited
//      FFEmu.hlsl next to the exe (or in the shaders dir) and relaunch, no rebuild.
//   2. EMBEDDED  RCDATA "FFEMU_HLSL"   -- fallback, so a clean install runs with no external file at all.
#pragma once

#include <windows.h>
#include <string>

// Returns the FFEmu.hlsl source text (empty string on total failure). outPath is filled with the external
// path probed; fromFile is true when the returned text came from that file (false = embedded resource).
inline std::string LoadFFEmuShaderSource(const char* shaderDir, std::wstring& outPath, bool& fromFile)
{
	std::wstring dir;
	{
		std::string s = shaderDir ? shaderDir : ".";
		dir.assign(s.begin(), s.end());
		if (!dir.empty() && dir.back() != L'\\' && dir.back() != L'/') dir += L'\\';
	}
	outPath = dir + L"FFEmu.hlsl";
	fromFile = false;

	// 1) External file (runtime-tunable) -- read the whole thing into memory.
	HANDLE h = CreateFileW(outPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
	                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER sz;
		if (GetFileSizeEx(h, &sz) && sz.QuadPart > 0 && sz.QuadPart < (1 << 24))   // sane cap: 16 MB
		{
			std::string out((size_t)sz.QuadPart, '\0');
			DWORD rd = 0;
			if (ReadFile(h, &out[0], (DWORD)sz.QuadPart, &rd, NULL) && rd == (DWORD)sz.QuadPart)
			{
				CloseHandle(h);
				fromFile = true;
				return out;
			}
		}
		CloseHandle(h);
	}

	// 2) Embedded RCDATA fallback (baked into the exe by falcon4.rc).
	HRSRC res = FindResourceA(NULL, "FFEMU_HLSL", (LPCSTR)RT_RCDATA);
	if (res)
	{
		HGLOBAL g = LoadResource(NULL, res);
		DWORD len = SizeofResource(NULL, res);
		const void* p = g ? LockResource(g) : NULL;
		if (p && len) return std::string((const char*)p, len);
	}
	return std::string();
}
