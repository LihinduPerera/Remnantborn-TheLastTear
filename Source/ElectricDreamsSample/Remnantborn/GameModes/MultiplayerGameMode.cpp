#include "MultiplayerGameMode.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "ElectricDreamsSample/Remnantborn/CharacterSelection/CharacterPlayerState.h"
#include "ElectricDreamsSample/Remnantborn/GameplayAbilitySystem/Characters/RemnantbornCharacterBase.h"
#include "ElectricDreamsSample/Remnantborn/CharacterSelection/CharacterSelectionSubsystem.h"
#include "ElectricDreamsSample/Remnantborn/CharacterSelection/CharacterDataAsset.h"
#include "ElectricDreamsSample/Remnantborn/OnlineService/MultiplayerPlayerController/MultiplayerPlayerController.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"

AMultiplayerGameMode::AMultiplayerGameMode()
{
	// Set player state class
    PlayerStateClass = ACharacterPlayerState::StaticClass();
    
	// Default pawn class - will be overridden by character selection
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/Remnantborn/Blueprints/GameplayAbilitySystem/Characters/BP_RemnantbornCharacterBase"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

void AMultiplayerGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Don't spawn characters here - let PostLogin handle it properly
	// This prevents double spawning and GAS initialization issues
}

void AMultiplayerGameMode::PostSeamlessTravel()
{
	Super::PostSeamlessTravel();

	// Set up input for all existing players after travel
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AMultiplayerPlayerController* PlayerController = Cast<AMultiplayerPlayerController>(It->Get()))
		{
			PlayerController->Client_SetupGameplayInput();
		}
	}
}

void AMultiplayerGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!NewPlayer)
	{
		return;
	}

	// Use retry mechanism to wait for PlayerState replication
	// This handles cases where PlayerState hasn't replicated yet
	FTimerHandle SpawnTimer;
	FTimerDelegate SpawnDelegate;
	SpawnDelegate.BindUFunction(this, "RetrySpawnPlayerWithCharacter", NewPlayer, 0);
	GetWorld()->GetTimerManager().SetTimer(SpawnTimer, SpawnDelegate, 0.1f, false);
}

void AMultiplayerGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	if (APlayerController* PC = Cast<APlayerController>(Exiting))
	{
		if (PC->GetPawn())
		{
			PC->GetPawn()->Destroy();
		}
	}
}

void AMultiplayerGameMode::SetupPlayerInput(APlayerController* PlayerController)
{
	if (PlayerController)
	{
		// Reset to game input mode
		FInputModeGameOnly InputMode;
		PlayerController->SetInputMode(InputMode);
		PlayerController->SetShowMouseCursor(false);

		// Make sure the player controller can process input
		PlayerController->bShowMouseCursor = false;
		PlayerController->bEnableClickEvents = false;
		PlayerController->bEnableMouseOverEvents = false;
	}
}

void AMultiplayerGameMode::SpawnPlayerWithCharacter(APlayerController* PlayerController)
{
    if (!PlayerController || !GetWorld())
    {
        return;
    }

    // Destroy existing pawn if any
    if (PlayerController->GetPawn())
    {
        PlayerController->GetPawn()->Destroy();
    }

    // Get player's selected character
    ACharacterPlayerState* PlayerState = PlayerController->GetPlayerState<ACharacterPlayerState>();
    if (!PlayerState)
    {
        UE_LOG(LogTemp, Warning, TEXT("PlayerState not found for player"));
        // Use default spawning for safety
        RestartPlayer(PlayerController);
        return;
    }

    // Check if character selection is replicated
    if (!PlayerState->HasSelectedCharacter())
    {
        UE_LOG(LogTemp, Warning, TEXT("No character selected for player, using default character"));

        // Spawn default character using standard GameMode method
        if (DefaultPawnClass)
        {
            RestartPlayer(PlayerController);
        }
        return;
    }

    UCharacterDataAsset* SelectedCharacter = PlayerState->GetSelectedCharacter();
    if (!SelectedCharacter || !SelectedCharacter->CharacterClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid character data, using default character"));
        
        // Fallback to default spawning
        if (DefaultPawnClass)
        {
            RestartPlayer(PlayerController);
        }
        return;
    }

    // Temporarily override the DefaultPawnClass for this player
    TSubclassOf<APawn> OriginalPawnClass = DefaultPawnClass;
    DefaultPawnClass = SelectedCharacter->CharacterClass;

    // Use standard RestartPlayer to ensure proper initialization
    RestartPlayer(PlayerController);

    // Restore original default pawn class
    DefaultPawnClass = OriginalPawnClass;

    // Get the spawned pawn and grant abilities if it's our character
    if (APawn* SpawnedPawn = PlayerController->GetPawn())
    {
        ARemnantbornCharacterBase* CharacterBase = Cast<ARemnantbornCharacterBase>(SpawnedPawn);
        if (CharacterBase && SelectedCharacter->StartingAbilities.Num() > 0)
        {
            // Grant starting abilities (server only)
            if (HasAuthority())
            {
                CharacterBase->GrantAbilities(SelectedCharacter->StartingAbilities);
            }
        }

        UE_LOG(LogTemp, Log, TEXT("Successfully spawned character %s for player"), *SelectedCharacter->CharacterName.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to spawn character for player"));
    }

    // Set up input for the player
    SetupPlayerInput(PlayerController);
}

void AMultiplayerGameMode::RetrySpawnPlayerWithCharacter(APlayerController* PlayerController, int32 RetryCount)
{
    if (!PlayerController || !GetWorld())
    {
        return;
    }

    // Max retry limit
    if (RetryCount >= 5)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to spawn player after 5 retries, using default character"));
        if (DefaultPawnClass)
        {
            RestartPlayer(PlayerController);
        }
        return;
    }

    ACharacterPlayerState* PlayerState = PlayerController->GetPlayerState<ACharacterPlayerState>();
    if (!PlayerState || !PlayerState->HasSelectedCharacter())
    {
        // Retry with delay
        FTimerHandle RetryTimer;
        FTimerDelegate RetryDelegate;
        RetryDelegate.BindUFunction(this, "RetrySpawnPlayerWithCharacter", PlayerController, RetryCount + 1);
        GetWorld()->GetTimerManager().SetTimer(RetryTimer, RetryDelegate, 0.5f, false);
        return;
    }

    // PlayerState is ready, spawn the character
    SpawnPlayerWithCharacter(PlayerController);
}