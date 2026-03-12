#pragma once

#include <Windows.h>

namespace Utilities
{
	RE::TESForm* GetFormFromMod(std::string modname, uint32_t formid);
	uintptr_t GetFallout4BaseAddress();

	template <class FunctionPointer>
	FunctionPointer SafeWrite64Function(uintptr_t addr, FunctionPointer data)
	{
		DWORD oldProtect;
		void* _d[2];
		memcpy(_d, &data, sizeof(data));
		size_t len = sizeof(_d[0]);

		VirtualProtect((void*)addr, len, PAGE_EXECUTE_READWRITE, &oldProtect);
		FunctionPointer olddata;
		memset(&olddata, 0, sizeof(FunctionPointer));
		memcpy(&olddata, (void*)addr, len);
		memcpy((void*)addr, &_d[0], len);
		VirtualProtect((void*)addr, len, oldProtect, &oldProtect);
		return olddata;
	}

	inline bool RollChanceFast(int chance)
	{
		if (chance <= 0)
			return false;
		if (chance >= 100)
			return true;

		thread_local uint32_t seed =
			static_cast<uint32_t>(std::chrono::high_resolution_clock::now()
				.time_since_epoch().count());

		seed = 1664525u * seed + 1013904223u;
		uint32_t roll = seed % 100;

		return roll < static_cast<uint32_t>(chance);
	}

	inline int RollIntRangeFast(int min, int max)
	{
		if (min >= max)
			return min;

		thread_local uint32_t seed =
			static_cast<uint32_t>(std::chrono::high_resolution_clock::now()
				.time_since_epoch().count());

		seed = 1664525u * seed + 1013904223u;
		return min + (seed % (max - min + 1));
	}

	template <typename T>
	T Min(T a, T b)
	{
		return (a < b) ? a : b;
	}

	template <typename T>
	T Max(T a, T b)
	{
		return (a > b) ? a : b;
	}
}