#include "MultiplayerGameMode.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "ElectricDreamsSample/Remnantborn/CharacterSelection/CharacterPlayerState.h"
#include "ElectricDreamsSample/Remnantborn/GameplayAbilitySystem/Characters/RemnantbornCharacterBase.h"
#include "ElectricDreamsSample/Remnantborn/CharacterSelection/CharacterSelectionSubsystem.h"

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
    
	// Set up input for all existing players
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PlayerController = It->Get())
		{
			SetupPlayerInput(PlayerController);
            SpawnPlayerWithCharacter(PlayerController);
		}
	}
}

void AMultiplayerGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
    
	// Set up input for newly joined players
	SetupPlayerInput(NewPlayer);
    SpawnPlayerWithCharacter(NewPlayer);
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
    if (!PlayerController)
    {
        return;
    }
    
    // Get player's selected character
    ACharacterPlayerState* PlayerState = PlayerController->GetPlayerState<ACharacterPlayerState>();
    if (!PlayerState)
    {
        return;
    }
    
    UCharacterDataAsset* SelectedCharacter = PlayerState->GetSelectedCharacter();
    if (!SelectedCharacter)
    {
        // Try to get from subsystem as fallback
        UCharacterSelectionSubsystem* CharacterSubsystem = GetGameInstance()->GetSubsystem<UCharacterSelectionSubsystem>();
        if (CharacterSubsystem)
        {
            SelectedCharacter = CharacterSubsystem->GetSelectedCharacter(PlayerController);
        }
    }
    
    if (!SelectedCharacter || !SelectedCharacter->CharacterClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("No character selected or invalid character class for player"));
        return;
    }
    
    // Find a player start
    AActor* PlayerStart = FindPlayerStart(PlayerController);
    if (!PlayerStart)
    {
        UE_LOG(LogTemp, Warning, TEXT("No player start found"));
        return;
    }
    
    // Spawn the character
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = PlayerController;
    SpawnParams.Instigator = nullptr;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    
    FVector SpawnLocation = PlayerStart->GetActorLocation();
    FRotator SpawnRotation = PlayerStart->GetActorRotation();
    
    ARemnantbornCharacterBase* SpawnedCharacter = GetWorld()->SpawnActor<ARemnantbornCharacterBase>(
        SelectedCharacter->CharacterClass,
        SpawnLocation,
        SpawnRotation,
        SpawnParams
    );
    
    if (SpawnedCharacter)
    {
        // Possess the character
        PlayerController->Possess(SpawnedCharacter);
        
        // Grant starting abilities
        if (SelectedCharacter->StartingAbilities.Num() > 0)
        {
            SpawnedCharacter->GrantAbilities(SelectedCharacter->StartingAbilities);
        }
        
        UE_LOG(LogTemp, Log, TEXT("Spawned character %s for player"), *SelectedCharacter->CharacterName.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to spawn character for player"));
    }
}