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

	// Ensure the player controller is of the correct type
	AMultiplayerPlayerController* MultiplayerPC = Cast<AMultiplayerPlayerController>(NewPlayer);
	if (!MultiplayerPC)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerController is not of type MultiplayerPlayerController"));
		return;
	}

	// Use retry mechanism to wait for PlayerState replication and initialization
	// This handles cases where PlayerState hasn't replicated yet or character data isn't ready
	FTimerHandle SpawnTimer;
	FTimerDelegate SpawnDelegate;
	SpawnDelegate.BindUFunction(this, "RetrySpawnPlayerWithCharacter", NewPlayer, 0);
	GetWorld()->GetTimerManager().SetTimer(SpawnTimer, SpawnDelegate, 0.2f, false);
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
        UE_LOG(LogTemp, Error, TEXT("SpawnPlayerWithCharacter: Invalid parameters"));
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
        UE_LOG(LogTemp, Warning, TEXT("PlayerState not found for player, using default character"));
        // Use default spawning for safety
        RestartPlayer(PlayerController);
        return;
    }

    // Log character selection state
    FName SelectedID = PlayerState->GetSelectedCharacterID();
    UE_LOG(LogTemp, Log, TEXT("Attempting to spawn character %s for player %s"), 
        *SelectedID.ToString(), *PlayerState->GetPlayerName());

    // Check if character selection is replicated
    if (!PlayerState->HasSelectedCharacter())
    {
        UE_LOG(LogTemp, Warning, TEXT("No character selected for player %s, using default character"), *PlayerState->GetPlayerName());

        // Spawn default character using standard GameMode method
        if (DefaultPawnClass)
        {
            RestartPlayer(PlayerController);
        }
        return;
    }

    UCharacterDataAsset* SelectedCharacter = PlayerState->GetSelectedCharacter();
    if (!SelectedCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("Character data not cached for player %s, retrying..."), *PlayerState->GetPlayerName());
        
        // Force retry of character data caching
        if (PlayerState->IsCharacterDataReady())
        {
            SelectedCharacter = PlayerState->GetSelectedCharacter();
        }
        
        if (!SelectedCharacter)
        {
            UE_LOG(LogTemp, Warning, TEXT("Still unable to get character data for player %s, using default character"), *PlayerState->GetPlayerName());
            // Fallback to default spawning
            if (DefaultPawnClass)
            {
                RestartPlayer(PlayerController);
            }
            return;
        }
    }

    if (!SelectedCharacter->CharacterClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid character class for %s, using default character"), *SelectedCharacter->CharacterName.ToString());
        
        // Fallback to default spawning
        if (DefaultPawnClass)
        {
            RestartPlayer(PlayerController);
        }
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Spawning character class %s for player %s"), 
        *SelectedCharacter->CharacterClass->GetName(), *PlayerState->GetPlayerName());

    // Override the DefaultPawnClass for this player temporarily
    TSubclassOf<APawn> OriginalPawnClass = DefaultPawnClass;
    DefaultPawnClass = SelectedCharacter->CharacterClass;

    // Use standard RestartPlayer to ensure proper initialization
    RestartPlayer(PlayerController);

    // Restore original default pawn class
    DefaultPawnClass = OriginalPawnClass;

    // Get the spawned pawn and set up GAS and abilities
    if (APawn* SpawnedPawn = PlayerController->GetPawn())
    {
        ARemnantbornCharacterBase* CharacterBase = Cast<ARemnantbornCharacterBase>(SpawnedPawn);
        if (CharacterBase)
        {
            // Ensure GAS is properly initialized for the character
            if (CharacterBase->GetAbilitySystemComponent())
            {
                CharacterBase->GetAbilitySystemComponent()->InitAbilityActorInfo(CharacterBase, CharacterBase);
                
                // Grant starting abilities if available
                if (SelectedCharacter->StartingAbilities.Num() > 0)
                {
                    CharacterBase->GrantAbilities(SelectedCharacter->StartingAbilities);
                }
            }
            
            UE_LOG(LogTemp, Log, TEXT("Successfully spawned character %s for player %s"), 
                *SelectedCharacter->CharacterName.ToString(), *PlayerState->GetPlayerName());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Spawned pawn is not a RemnantbornCharacterBase for player %s"), *PlayerState->GetPlayerName());
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to spawn character for player %s"), *PlayerState->GetPlayerName());
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
    if (RetryCount >= 10)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to spawn player after 10 retries, using default character"));
        if (DefaultPawnClass)
        {
            RestartPlayer(PlayerController);
        }
        return;
    }

    ACharacterPlayerState* PlayerState = PlayerController->GetPlayerState<ACharacterPlayerState>();
    if (!PlayerState)
    {
        UE_LOG(LogTemp, Warning, TEXT("PlayerState not available yet (retry %d/10)"), RetryCount + 1);
        // Retry with delay
        FTimerHandle RetryTimer;
        FTimerDelegate RetryDelegate;
        RetryDelegate.BindUFunction(this, "RetrySpawnPlayerWithCharacter", PlayerController, RetryCount + 1);
        GetWorld()->GetTimerManager().SetTimer(RetryTimer, RetryDelegate, 0.5f, false);
        return;
    }

    if (!PlayerState->HasSelectedCharacter())
    {
        UE_LOG(LogTemp, Warning, TEXT("Character selection not available yet (retry %d/10)"), RetryCount + 1);
        // Retry with delay
        FTimerHandle RetryTimer;
        FTimerDelegate RetryDelegate;
        RetryDelegate.BindUFunction(this, "RetrySpawnPlayerWithCharacter", PlayerController, RetryCount + 1);
        GetWorld()->GetTimerManager().SetTimer(RetryTimer, RetryDelegate, 0.5f, false);
        return;
    }

    if (!PlayerState->IsCharacterDataReady())
    {
        UE_LOG(LogTemp, Warning, TEXT("Character data not ready yet (retry %d/10)"), RetryCount + 1);
        // Retry with delay
        FTimerHandle RetryTimer;
        FTimerDelegate RetryDelegate;
        RetryDelegate.BindUFunction(this, "RetrySpawnPlayerWithCharacter", PlayerController, RetryCount + 1);
        GetWorld()->GetTimerManager().SetTimer(RetryTimer, RetryDelegate, 0.5f, false);
        return;
    }

    // All checks passed, spawn the character
    UE_LOG(LogTemp, Log, TEXT("Spawning character for player (attempt %d)"), RetryCount + 1);
    SpawnPlayerWithCharacter(PlayerController);
}