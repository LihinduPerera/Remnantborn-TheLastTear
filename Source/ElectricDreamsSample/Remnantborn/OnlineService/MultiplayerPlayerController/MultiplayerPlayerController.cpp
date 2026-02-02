#include "MultiplayerPlayerController.h"
#include "ElectricDreamsSample/Remnantborn/GameplayAbilitySystem/Characters/RemnantbornCharacterBase.h"
#include "ElectricDreamsSample/Remnantborn/CharacterSelection/CharacterPlayerState.h"
#include "ElectricDreamsSample/Remnantborn/OnlineService/MyOnlineGameInstance.h"
#include "ElectricDreamsSample/Remnantborn/CharacterSelection/CharacterSelectionSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"
#include "Blueprint/UserWidget.h"

void AMultiplayerPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	// Initialize HUD when PlayerState is replicated
	if (IsLocalPlayerController())
	{
		InitializeHUD();
	}
}

void AMultiplayerPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	
	// Initialize HUD and GAS when PlayerState replicates to clients
	if (IsLocalPlayerController())
	{
		InitializeHUD();
		
		// Setup GAS for the current pawn if we have one
		if (GetPawn())
		{
			SetupGASForPawn(GetPawn());
		}
		
		// Check if we need to restore character selection from GameInstance
		// This handles the case where PlayerState is cleared during seamless travel
		ACharacterPlayerState* CharPlayerState = GetPlayerState<ACharacterPlayerState>();
		if (CharPlayerState && !CharPlayerState->HasSelectedCharacter())
		{
			UMyOnlineGameInstance* GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance());
			if (GameInstance)
			{
				FName LocalCharacterSelection = GameInstance->GetLocalCharacterSelection();
				if (!LocalCharacterSelection.IsNone())
				{
					// Restore character selection from GameInstance
					CharPlayerState->Server_SetSelectedCharacterID(LocalCharacterSelection);
					UE_LOG(LogTemp, Log, TEXT("Restored character selection %s from GameInstance"), *LocalCharacterSelection.ToString());
				}
			}
		}
	}
}

void AMultiplayerPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// Initialize GAS for the possessed pawn on server
	if (HasAuthority())
	{
		SetupGASForPawn(InPawn);
		Client_SetupGameplayInput();
		Client_InitializeHUD();
	}
}

void AMultiplayerPlayerController::Client_SetupGameplayInput_Implementation()
{
	SetupInputMode();
}

void AMultiplayerPlayerController::Client_InitializeHUD_Implementation()
{
	InitializeHUD();
}

void AMultiplayerPlayerController::SetupInputMode()
{
	// Set game-only input mode
	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);

	// Hide mouse cursor
	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;

	// Force input enable
	SetIgnoreMoveInput(false);
	SetIgnoreLookInput(false);

	// Reset view rotation
	ResetIgnoreInputFlags();

	// Ensure input is enabled
	bEnableMouseOverEvents = false;
	bEnableClickEvents = false;
}

void AMultiplayerPlayerController::InitializeHUD()
{
	// Only proceed if we're the local player controller
	if (!IsLocalPlayerController())
	{
		return;
	}

	// Note: HUDClass should be set in the GameMode or PlayerController blueprint
	// This is a placeholder for HUD initialization logic
	// In your implementation, you would create your specific HUD widget here
	
	UE_LOG(LogTemp, Log, TEXT("HUD initialization called for local player"));
}

void AMultiplayerPlayerController::SetupGASForPawn(APawn* InPawn)
{
	if (!InPawn)
	{
		return;
	}

	ARemnantbornCharacterBase* RemnantbornCharacter = Cast<ARemnantbornCharacterBase>(InPawn);
	if (!RemnantbornCharacter || !RemnantbornCharacter->GetAbilitySystemComponent())
	{
		return;
	}

	UAbilitySystemComponent* ASC = RemnantbornCharacter->GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	// Initialize ASC ActorInfo on both server and client
	if (HasAuthority())
	{
		// Server initialization
		ASC->InitAbilityActorInfo(RemnantbornCharacter, RemnantbornCharacter);
		
		// Grant abilities from character data if available
		ACharacterPlayerState* CharPlayerState = GetPlayerState<ACharacterPlayerState>();
		if (CharPlayerState && CharPlayerState->IsCharacterDataReady())
		{
			UCharacterDataAsset* SelectedCharacter = CharPlayerState->GetSelectedCharacter();
			if (SelectedCharacter && SelectedCharacter->StartingAbilities.Num() > 0)
			{
				RemnantbornCharacter->GrantAbilities(SelectedCharacter->StartingAbilities);
			}
		}
	}
	else
	{
		// Client initialization - set up ASC for local prediction
		ASC->InitAbilityActorInfo(RemnantbornCharacter, RemnantbornCharacter);
	}
}