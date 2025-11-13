// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DeveloperSettings.h"
#include "UObject/SoftObjectPtr.h"

#include "EDSAudioSettings.generated.h"


/**
 *
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "EDSAudioSettings"))
class ELECTRICDREAMSSAMPLE_API UEDSAudioSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** The Default Base Control Bus Mix */
	UPROPERTY(config, EditAnywhere, Category = MixSettings, meta = (AllowedClasses = "/Script/AudioModulation.SoundControlBusMix"))
	FSoftObjectPath DefaultControlBusMix;

	/** The Live Control Bus Mix */
	UPROPERTY(config, EditAnywhere, Category = MixSettings, meta = (AllowedClasses = "/Script/AudioModulation.SoundControlBusMix"))
	FSoftObjectPath LiveControlBusMix;
};