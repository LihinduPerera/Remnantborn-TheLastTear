#include "RemnantbornCharacterBase.h"

#include "Components/CapsuleComponent.h"
#include "ElectricDreamsSample/Remnantborn/GameplayAbilitySystem/AttributeSets/BasicAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "ElectricDreamsSample/Remnantborn/CharacterSelection/CharacterPlayerState.h"
#include "ElectricDreamsSample/Remnantborn/GameplayAbilitySystem/RemnantbornAbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
ARemnantbornCharacterBase::ARemnantbornCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// Ability System Component
	AbilitySystemComponent =
		CreateDefaultSubobject<URemnantbornAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(AscReplicationMode);

	// Capsule
	GetCapsuleComponent()->InitCapsuleSize(35.0f, 90.0f);

	// Rotation settings
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Movement values
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Attribute Set
	BasicAttributeSet =
		CreateDefaultSubobject<UBasicAttributeSet>(TEXT("BasicAttributeSet"));
}

// Called when the game starts or when spawned
void ARemnantbornCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	// Bind GameplayTag delegate AFTER everything is initialized
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent
			->RegisterGameplayTagEvent(
				FGameplayTag::RequestGameplayTag(FName("State.Dead")),
				EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &ARemnantbornCharacterBase::OnDeadTagChanged);
	}
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

	// Initialize GAS on server
	if (HasAuthority() && AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		
		// Only grant starting abilities if no specific character data is available
		// Character-specific abilities will be granted by GameMode
		ACharacterPlayerState* CharPlayerState = GetPlayerState<ACharacterPlayerState>();
		if (!CharPlayerState || !CharPlayerState->IsCharacterDataReady())
		{
			// Only grant abilities if there are any to grant
			if (StartingAbilities.Num() > 0)
			{
				GrantAbilities(StartingAbilities);
			}
		}
	}
}

void ARemnantbornCharacterBase::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// Initialize GAS on clients when PlayerState replicates
	if (AbilitySystemComponent && GetLocalRole() != ROLE_Authority)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

UAbilitySystemComponent* ARemnantbornCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

TArray<FGameplayAbilitySpecHandle>
ARemnantbornCharacterBase::GrantAbilities(
	TArray<TSubclassOf<UGameplayAbility>> AbilitiesToGrant)
{
	if (!AbilitySystemComponent || !HasAuthority())
	{
		return {};
	}

	TArray<FGameplayAbilitySpecHandle> AbilityHandles;

	for (TSubclassOf<UGameplayAbility> Ability : AbilitiesToGrant)
	{
		FGameplayAbilitySpecHandle Handle =
			AbilitySystemComponent->GiveAbility(
				FGameplayAbilitySpec(Ability, 1, INDEX_NONE, this));

		AbilityHandles.Add(Handle);
	}

	SendAbilitiesChangedEvent();
	return AbilityHandles;
}

void ARemnantbornCharacterBase::RemoveAbilities(
	TArray<FGameplayAbilitySpecHandle> AbilityHandlesToRemove)
{
	if (!AbilitySystemComponent || !HasAuthority())
	{
		return;
	}

	for (FGameplayAbilitySpecHandle Handle : AbilityHandlesToRemove)
	{
		AbilitySystemComponent->ClearAbility(Handle);
	}

	SendAbilitiesChangedEvent();
}

void ARemnantbornCharacterBase::SendAbilitiesChangedEvent()
{
	FGameplayEventData EventData;
	EventData.EventTag =
		FGameplayTag::RequestGameplayTag(FName("Event.Abilities.Changed"));
	EventData.Instigator = this;
	EventData.Target = this;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		this, EventData.EventTag, EventData);
}

void ARemnantbornCharacterBase::ServerSendGameplayEventToSelf_Implementation(
	FGameplayEventData EventData)
{
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		this, EventData.EventTag, EventData);
}

void ARemnantbornCharacterBase::HandleDeath_Implementation()
{
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	GetCharacterMovement()->DisableMovement();

	FVector Impulse = GetActorForwardVector() * -20000.f;
	Impulse.Z = 15000.f;

	GetMesh()->AddImpulseAtLocation(Impulse, GetActorLocation());
}

void ARemnantbornCharacterBase::OnDeadTagChanged(
	const FGameplayTag CallbackTag, int32 NewCount)
{
	if (NewCount > 0)
	{
		HandleDeath();
	}
}
