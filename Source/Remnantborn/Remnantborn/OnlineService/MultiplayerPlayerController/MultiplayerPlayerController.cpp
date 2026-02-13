#include "MultiplayerPlayerController.h"
#include "Remnantborn/Remnantborn/GameplayAbilitySystem/Characters/RemnantbornCharacterBase.h"
#include "Remnantborn/Remnantborn/CharacterSelection/CharacterPlayerState.h"
#include "Remnantborn/Remnantborn/OnlineService/MyOnlineGameInstance.h"
#include "Remnantborn/Remnantborn/CharacterSelection/CharacterSelectionSubsystem.h"
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

	// Create match results widget if class is set and not already created
	if (MatchResultsWidgetClass && !MatchResultsWidget)
	{
		MatchResultsWidget = CreateWidget<UMatchResultsWidget>(this, MatchResultsWidgetClass);
		if (MatchResultsWidget)
		{
			MatchResultsWidget->AddToViewport(100); // High Z-order to ensure it's on top
			MatchResultsWidget->SetVisibility(ESlateVisibility::Hidden); // Start hidden
			MatchResultsWidget->InitializeMatchResults(); // Initialize with game state
			
			UE_LOG(LogTemp, Log, TEXT("MultiplayerPlayerController: Match results widget created, added to viewport (Z-Order 100), and initialized"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("MultiplayerPlayerController: Failed to create MatchResultsWidget from class %s"), 
				*MatchResultsWidgetClass->GetName());
		}
	}
	else if (!MatchResultsWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("MultiplayerPlayerController: MatchResultsWidgetClass is not set! Please assign it in the PlayerController Blueprint."));
	}
	
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

void AMultiplayerPlayerController::Client_ShowMatchResults_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("Client_ShowMatchResults CALLED for %s (LocalPlayer=%s, HasWidget=%s)"),
		*GetName(),
		IsLocalPlayerController() ? TEXT("YES") : TEXT("NO"),
		MatchResultsWidget ? TEXT("YES") : TEXT("NO"));

	if (MatchResultsWidget)
	{
		// Refresh the results display before showing
		MatchResultsWidget->UpdateResultsDisplay();
		
		// Show the widget
		MatchResultsWidget->SetVisibility(ESlateVisibility::Visible);
		
		// Set UI input mode for interaction
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(MatchResultsWidget->TakeWidget());
		SetInputMode(InputMode);
		bShowMouseCursor = true;
		
		// Disable player input while showing results
		SetIgnoreMoveInput(true);
		SetIgnoreLookInput(true);
		
		UE_LOG(LogTemp, Log, TEXT("MultiplayerPlayerController: Match results shown for %s"), 
			*GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MultiplayerPlayerController: Cannot show match results - widget not created! WidgetClass=%s"),
			MatchResultsWidgetClass ? *MatchResultsWidgetClass->GetName() : TEXT("NULL"));
		
		// Try to reinitialize HUD and show again
		InitializeHUD();
		
		if (MatchResultsWidget)
		{
			UE_LOG(LogTemp, Log, TEXT("MultiplayerPlayerController: Widget created on retry, showing results..."));
			MatchResultsWidget->UpdateResultsDisplay();
			MatchResultsWidget->SetVisibility(ESlateVisibility::Visible);
			
			FInputModeUIOnly InputMode;
			SetInputMode(InputMode);
			bShowMouseCursor = true;
			SetIgnoreMoveInput(true);
			SetIgnoreLookInput(true);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("MultiplayerPlayerController: FAILED to create widget even after retry!"));
		}
	}
}

void AMultiplayerPlayerController::Client_HideMatchResults_Implementation()
{
	if (MatchResultsWidget)
	{
		MatchResultsWidget->SetVisibility(ESlateVisibility::Hidden);
		
		// Return to game input mode
		SetupInputMode();
		
		// Re-enable player input
		SetIgnoreMoveInput(false);
		SetIgnoreLookInput(false);
		
		UE_LOG(LogTemp, Log, TEXT("MultiplayerPlayerController: Match results hidden"));
	}
}

void AMultiplayerPlayerController::Client_NotifyPlayerDeath_Implementation(const FString& KillerName)
{
	// Show death notification to the player who died
	// This can be extended to show a kill feed or death cam
	UE_LOG(LogTemp, Log, TEXT("Player died! Killer: %s"), *KillerName);
	
	// Optional: Show a death overlay before match ends
	// This is useful for showing "YOU DIED" message immediately
}