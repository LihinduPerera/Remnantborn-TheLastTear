// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "RemnantbornGameplayAbility.generated.h"

/**
 * 
 */
UCLASS()
class ELECTRICDREAMSSAMPLE_API URemnantbornGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="UI")
	bool ShouldShowInAbilitiesBar = false;
};
