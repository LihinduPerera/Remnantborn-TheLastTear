// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Subsystems/WorldSubsystem.h"

#include "EDSMixManagerSubsystem.generated.h"

class USoundControlBus;
class USoundControlBusMix;

/**
 *
 */
UCLASS()
class ELECTRICDREAMSSAMPLE_API UEDSMixManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	// USubsystem implementation Begin
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	// USubsystem implementation End

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	/** Called once all UWorldSubsystems have been initialized */
	virtual void PostInitialize() override;

	/** Called when world is ready to start gameplay before the game mode transitions to the correct state and call BeginPlay on all actors */
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

protected:
	// Called when determining whether to create this Subsystem
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	// Default Sound Control Bus Mix retrieved from the EDS Audio Settings
	UPROPERTY(Transient)
	TObjectPtr<USoundControlBusMix> DefaultBaseMix = nullptr;

	// Live Sound Control Bus Mix retrieved from the EDS Audio Settings
	UPROPERTY(Transient)
	TObjectPtr<USoundControlBusMix> LiveMix = nullptr;

};