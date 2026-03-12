#include "Core.h"
#include "../Data/Data.h"
#include "../Hooks/Hooks.h"

namespace Core
{
	void F4SELoaded()
	{
		Hooks::InitializeOnLaunch();
	}

	void GameDataReady()
	{
		Hooks::Initialize();
	}

	void PreLoadGame()
	{

	}
}