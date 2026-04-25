#include "RemnantbornCharacterBase.h"

#include "Components/CapsuleComponent.h"
#include "Blueprint/UserWidget.h"
#include "Remnantborn/Remnantborn/GameplayAbilitySystem/AttributeSets/BasicAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Remnantborn/Remnantborn/CharacterSelection/CharacterPlayerState.h"
#include "Remnantborn/Remnantborn/GameplayAbilitySystem/RemnantbornAbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameModeBase.h"
#include "Remnantborn/Remnantborn/GameModes/MultiplayerGameMode.h"
#include "Sound/SoundBase.h"

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
		
		// Note: Character-specific abilities are granted by MultiplayerGameMode::SpawnPlayerWithCharacter
		// We don't grant abilities here to avoid conflicts with the GameMode's ability setup
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

	// Notify GameMode about player death - ONLY on server
	if (HasAuthority())
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			if (AMultiplayerGameMode* GameMode = Cast<AMultiplayerGameMode>(GetWorld()->GetAuthGameMode()))
			{
				UE_LOG(LogTemp, Warning, TEXT("HandleDeath: Notifying GameMode about death of %s"), *PC->GetName());
				GameMode->NotifyPlayerDied(PC);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("HandleDeath: GameMode is not AMultiplayerGameMode!"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("HandleDeath: No PlayerController found!"));
		}
	}
}

void ARemnantbornCharacterBase::OnDeadTagChanged(
	const FGameplayTag CallbackTag, int32 NewCount)
{
	if (NewCount > 0)
	{
		HandleDeath();
	}
}

void ARemnantbornCharacterBase::OnAbilitiesGrantedAndReady()
{
	// Call Blueprint event
	OnCharacterReadyForUI();
	
	// Notify all clients that their character is ready for UI
	if (GetLocalRole() == ROLE_Authority)
	{
		Client_OnCharacterReadyForUI();
	}
	else
	{
		// For clients, directly set up the UI
		SetupPlayerHUD();
	}
}

void ARemnantbornCharacterBase::Client_OnCharacterReadyForUI_Implementation()
{
	SetupPlayerHUD();
}

void ARemnantbornCharacterBase::SetupPlayerHUD()
{
	// Only create HUD on locally controlled characters
	if (IsLocallyControlled() && PlayerVitalsWidgetClass && !PlayerVitalsWidget)
	{
		APlayerController* PC = Cast<APlayerController>(GetController());
		if (PC)
		{
			PlayerVitalsWidget = CreateWidget<UUserWidget>(PC, PlayerVitalsWidgetClass);
			if (PlayerVitalsWidget)
			{
				PlayerVitalsWidget->AddToViewport();
			}
		}
	}
}

bool ARemnantbornCharacterBase::HasDamageBurstGruntOverrides() const
{
	for (USoundBase* Sound : DamageBurstGruntOverrideSounds)
	{
		if (IsValid(Sound))
		{
			return true;
		}
	}

	return false;
}

USoundBase* ARemnantbornCharacterBase::GetRandomDamageBurstGruntOverride() const
{
	return PickRandomValidSound(DamageBurstGruntOverrideSounds);
}

USoundBase* ARemnantbornCharacterBase::SelectDamageBurstGruntSound(
	const TArray<USoundBase*>& MaleFallbackSounds,
	const TArray<USoundBase*>& FemaleFallbackSounds) const
{
	if (USoundBase* OverrideSound = GetRandomDamageBurstGruntOverride())
	{
		return OverrideSound;
	}

	const TArray<USoundBase*>& GenderFallbackSounds =
		CharacterGender == ERemnantbornCharacterGender::Female
			? FemaleFallbackSounds
			: MaleFallbackSounds;

	return PickRandomValidSound(GenderFallbackSounds);
}

USoundBase* ARemnantbornCharacterBase::PickRandomValidSound(const TArray<USoundBase*>& Sounds)
{
	TArray<USoundBase*> ValidSounds;
	ValidSounds.Reserve(Sounds.Num());

	for (USoundBase* Sound : Sounds)
	{
		if (IsValid(Sound))
		{
			ValidSounds.Add(Sound);
		}
	}

	if (ValidSounds.Num() == 0)
	{
		return nullptr;
	}

	const int32 RandomIndex = FMath::RandRange(0, ValidSounds.Num() - 1);
	return ValidSounds[RandomIndex];
}
