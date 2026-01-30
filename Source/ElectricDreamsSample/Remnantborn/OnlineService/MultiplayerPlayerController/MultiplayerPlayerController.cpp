#include "MultiplayerPlayerController.h"

void AMultiplayerPlayerController::BeginPlay()
{
	Super::BeginPlay();
	SetupInputMode();
}

void AMultiplayerPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	SetupInputMode();
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
}