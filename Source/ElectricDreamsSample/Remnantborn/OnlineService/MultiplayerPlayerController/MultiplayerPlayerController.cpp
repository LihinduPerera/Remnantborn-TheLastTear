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
		
		// Send character selection to server if needed
		// This handles the case where PlayerState is cleared during seamless travel
		SendCharacterSelectionToServer();
	}
}

void AMultiplayerPlayerController::SendCharacterSelectionToServer()
{
	ACharacterPlayerState* CharPlayerState = GetPlayerState<ACharacterPlayerState>();
	if (!CharPlayerState)
	{
		return;
	}
	
	// Only send if we don't already have a selection (server might have applied it from GameSession)
	if (CharPlayerState->HasSelectedCharacter())
	{
		return;
	}
	
	// Get character selection from GameInstance
	UMyOnlineGameInstance* GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance());
	if (!GameInstance)
	{
		return;
	}
	
	FName LocalCharacterSelection = GameInstance->GetLocalCharacterSelection();
	if (LocalCharacterSelection.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("SendCharacterSelectionToServer: No character selection in GameInstance"));
		return;
	}
	
	// Send to server via RPC
	Server_NotifyCharacterSelection(LocalCharacterSelection);
	UE_LOG(LogTemp, Log, TEXT("SendCharacterSelectionToServer: Notified server of character selection %s"), *LocalCharacterSelection.ToString());
}

bool AMultiplayerPlayerController::Server_NotifyCharacterSelection_Validate(FName CharacterID)
{
	return !CharacterID.IsNone();
}

void AMultiplayerPlayerController::Server_NotifyCharacterSelection_Implementation(FName CharacterID)
{
	if (CharacterID.IsNone())
	{
		return;
	}
	
	// Apply the character selection to our PlayerState
	ACharacterPlayerState* CharPlayerState = GetPlayerState<ACharacterPlayerState>();
	if (CharPlayerState)
	{
		// Only apply if we don't already have a selection
		if (!CharPlayerState->HasSelectedCharacter())
		{
			CharPlayerState->Server_SetSelectedCharacterID(CharacterID);
			UE_LOG(LogTemp, Log, TEXT("Server_NotifyCharacterSelection: Applied character %s for player %s"), 
				*CharacterID.ToString(), *CharPlayerState->GetPlayerName());
		}
		else
		{
			UE_LOG(LogTemp, Verbose, TEXT("Server_NotifyCharacterSelection: Player %s already has character %s selected"), 
				*CharPlayerState->GetPlayerName(), *CharPlayerState->GetSelectedCharacterID().ToString());
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
	// Note: Abilities are granted by MultiplayerGameMode::SpawnPlayerWithCharacter
	// to ensure they match the selected character
	ASC->InitAbilityActorInfo(RemnantbornCharacter, RemnantbornCharacter);
}