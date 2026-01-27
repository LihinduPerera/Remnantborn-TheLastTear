// Fill out your copyright notice in the Description page of Project Settings.


#include "RemnantbornGameplayAbility.h"

URemnantbornGameplayAbility::URemnantbornGameplayAbility()
{
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("GameplayAbility.Active")));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Dead")));
}
