#include "MultiplayerPlayerController.h"

void AMultiplayerPlayerController::BeginPlay()
{
	Super::BeginPlay();
}

void AMultiplayerPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (HasAuthority())
	{
		Client_SetupGameplayInput();
	}
}

void AMultiplayerPlayerController::Client_SetupGameplayInput_Implementation()
{
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

	// Ensure input is enabled
	bEnableMouseOverEvents = false;
	bEnableClickEvents = false;
}