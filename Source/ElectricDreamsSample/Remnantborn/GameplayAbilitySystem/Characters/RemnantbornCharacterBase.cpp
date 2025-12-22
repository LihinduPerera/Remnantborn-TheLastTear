// Fill out your copyright notice in the Description page of Project Settings.

#include "RemnantbornCharacterBase.h"

// Sets default values
ARemnantbornCharacterBase::ARemnantbornCharacterBase()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(AscReplicationMode);
}

// Called when the game starts or when spawned
void ARemnantbornCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ARemnantbornCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ARemnantbornCharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void ARemnantbornCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this,this);
	}
}

void ARemnantbornCharacterBase::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this,this);
	}
}

UAbilitySystemComponent* ARemnantbornCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}