// Copyright Epic Games, Inc. All Rights Reserved.

#include "EDSMixManagerSubsystem.h"
#include "EDSAudioSettings.h"
#include "Engine/World.h"
#include "UObject/Object.h"
#include "SoundControlBus.h"
#include "SoundControlBusMix.h"
#include "AudioModulationStatics.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(EDSMixManagerSubsystem)

class FSubsystemCollectionBase;

void UEDSMixManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UEDSMixManagerSubsystem::Deinitialize()
{

	Super::Deinitialize();
}

bool UEDSMixManagerSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	bool bShouldCreateSubsystem = Super::ShouldCreateSubsystem(Outer);

	if (Outer)
	{
		if (UWorld* World = Outer->GetWorld())
		{
			bShouldCreateSubsystem = DoesSupportWorldType(World->WorldType) && bShouldCreateSubsystem;
		}
	}

	return bShouldCreateSubsystem;
}

void UEDSMixManagerSubsystem::PostInitialize()
{
	if (const UEDSAudioSettings* EDSAudioSettings = GetDefault<UEDSAudioSettings>())
	{
		if (UObject* ObjPath = EDSAudioSettings->DefaultControlBusMix.TryLoad())
		{
			if (USoundControlBusMix* SoundControlBusMix = Cast<USoundControlBusMix>(ObjPath))
			{
				DefaultBaseMix = SoundControlBusMix;
			}
			else
			{
				ensureMsgf(SoundControlBusMix, TEXT("Default Control Bus Mix reference missing from EDS Audio Settings."));
			}
		}

		if (UObject* ObjPath = EDSAudioSettings->LiveControlBusMix.TryLoad())
		{
			if (USoundControlBusMix* SoundControlBusMix = Cast<USoundControlBusMix>(ObjPath))
			{
				LiveMix = SoundControlBusMix;
			}
			else
			{
				ensureMsgf(SoundControlBusMix, TEXT("Live Control Bus Mix reference missing from EDS Audio Settings."));
			}
		}
	}
}

void UEDSMixManagerSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	if (const UWorld* World = InWorld.GetWorld())
	{
		// Activate the default base mix
		if (DefaultBaseMix)
		{
			UAudioModulationStatics::ActivateBusMix(World, DefaultBaseMix);
		}

		// Activate the live mix
		if (LiveMix)
		{
			UAudioModulationStatics::ActivateBusMix(World, LiveMix);
		}
	}
}

bool UEDSMixManagerSubsystem::DoesSupportWorldType(const EWorldType::Type World) const
{
	// We only need this subsystem on Game worlds (PIE included)
	return (World == EWorldType::Game || World == EWorldType::PIE);
}