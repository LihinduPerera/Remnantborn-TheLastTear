#include "MultiplayerPlayerController.h"
#include "ElectricDreamsSample/Remnantborn/GameplayAbilitySystem/Characters/RemnantbornCharacterBase.h"

void AMultiplayerPlayerController::BeginPlay()
{
	Super::BeginPlay();
}

void AMultiplayerPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// Initialize GAS for the possessed pawn on server
	if (HasAuthority())
	{
		ARemnantbornCharacterBase* RemnantbornCharacter = Cast<ARemnantbornCharacterBase>(InPawn);
		if (RemnantbornCharacter && RemnantbornCharacter->GetAbilitySystemComponent())
		{
			// Ensure Ability System Component is properly initialized
			RemnantbornCharacter->GetAbilitySystemComponent()->InitAbilityActorInfo(RemnantbornCharacter, RemnantbornCharacter);
		}
		
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