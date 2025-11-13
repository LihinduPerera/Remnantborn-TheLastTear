// Copyright Epic Games, Inc. All Rights Reserved.

#include "ElectricDreamsSample.h"
#include "Modules/ModuleManager.h"
#include "RHI.h"

class FElectricDreamsSampleGameModule : public FDefaultGameModuleImpl
{
	virtual void StartupModule() override
	{
#if PLATFORM_WINDOWS
		if (RHIGetInterfaceType() == ERHIInterfaceType::D3D12)
		{
			GRHISupportsDynamicResolution = true;
		}
#endif
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FElectricDreamsSampleGameModule, ElectricDreamsSample, "Remnantborn");
