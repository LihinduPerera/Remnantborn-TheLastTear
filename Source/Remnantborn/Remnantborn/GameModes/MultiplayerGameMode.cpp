#include "MultiplayerGameMode.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Misc/DateTime.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "Remnantborn/Remnantborn/CharacterSelection/CharacterPlayerState.h"
#include "Remnantborn/Remnantborn/GameplayAbilitySystem/Characters/RemnantbornCharacterBase.h"
#include "Remnantborn/Remnantborn/GameplayAbilitySystem/AttributeSets/BasicAttributeSet.h"
#include "Remnantborn/Remnantborn/CharacterSelection/CharacterSelectionSubsystem.h"
#include "Remnantborn/Remnantborn/CharacterSelection/CharacterDataAsset.h"
#include "Remnantborn/Remnantborn/OnlineService/MyOnlineGameInstance.h"
#include "Remnantborn/Remnantborn/OnlineService/MultiplayerPlayerController/MultiplayerPlayerController.h"

AMultiplayerGameMode::AMultiplayerGameMode()
{
	// Set player state class
    PlayerStateClass = ACharacterPlayerState::StaticClass();
    
    // Set game session class - this persists through seamless travel
    GameSessionClass = ARemnantbornGameSession::StaticClass();
    
    // Set game state class for match tracking
    GameStateClass = AMultiplayerMatchGameState::StaticClass();
    
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

	// Initialize match tracking after a short delay to ensure all players are loaded
	FTimerHandle InitTimer;
	FTimerDelegate InitDelegate;
	InitDelegate.BindUFunction(this, "InitializeMatchTracking");
	GetWorld()->GetTimerManager().SetTimer(InitTimer, InitDelegate, 3.0f, false); // Increased to 3 seconds
	
	UE_LOG(LogTemp, Log, TEXT("MultiplayerGameMode: BeginPlay - Match tracking will initialize in 3 seconds"));
}

void AMultiplayerGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Check for player deaths periodically
	CheckForPlayerDeaths();
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

	UE_LOG(LogTemp, Log, TEXT("PostLogin: Player joined - %s"), *NewPlayer->GetName());

	// Check if player controller is the expected type (optional RPC features)
	AMultiplayerPlayerController* MultiplayerPC = Cast<AMultiplayerPlayerController>(NewPlayer);
	if (!MultiplayerPC)
	{
		UE_LOG(LogTemp, Log, TEXT("PostLogin: PlayerController is not MultiplayerPlayerController type, but will still spawn with character selection"));
	}

	// Try to apply character selection from GameSession (persisted from lobby)
	// This must happen BEFORE spawning the player
	if (ApplyCharacterSelectionFromGameSession(NewPlayer))
	{
		UE_LOG(LogTemp, Log, TEXT("PostLogin: Applied character selection from GameSession for player"));
	}

	// Use retry mechanism to wait for PlayerState replication and initialization
	// This handles cases where PlayerState hasn't replicated yet or character data isn't ready
	FTimerHandle SpawnTimer;
	FTimerDelegate SpawnDelegate;
	SpawnDelegate.BindUFunction(this, "RetrySpawnPlayerWithCharacter", NewPlayer, 0);
	GetWorld()->GetTimerManager().SetTimer(SpawnTimer, SpawnDelegate, 0.2f, false);
	
	// Re-initialize match tracking to include the new player
	// This ensures the PlayerResults array includes all players
	FTimerHandle ReinitTimer;
	GetWorld()->GetTimerManager().SetTimer(ReinitTimer, [this]()
	{
		InitializeMatchTracking();
	}, 1.0f, false);
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

    // Find a player start for this player
    AActor* StartSpot = FindPlayerStart(PlayerController);
    if (!StartSpot)
    {
        UE_LOG(LogTemp, Error, TEXT("No player start found for player %s"), *PlayerState->GetPlayerName());
        return;
    }
    
    FTransform SpawnTransform = StartSpot->GetActorTransform();
    
    // Spawn the character directly - this avoids race conditions with DefaultPawnClass
    FActorSpawnParameters SpawnInfo;
    SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    SpawnInfo.Owner = PlayerController;
    SpawnInfo.Instigator = nullptr;
    
    APawn* SpawnedPawn = GetWorld()->SpawnActor<APawn>(SelectedCharacter->CharacterClass, SpawnTransform.GetLocation(), SpawnTransform.GetRotation().Rotator(), SpawnInfo);
    
    if (!SpawnedPawn)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to spawn character %s for player %s"), 
            *SelectedCharacter->CharacterName.ToString(), *PlayerState->GetPlayerName());
        return;
    }
    
    // Possess the newly spawned pawn
    PlayerController->Possess(SpawnedPawn);
    
    UE_LOG(LogTemp, Log, TEXT("Successfully spawned and possessed character %s for player %s"), 
        *SelectedCharacter->CharacterName.ToString(), *PlayerState->GetPlayerName());

    // Set up GAS and abilities for the spawned pawn
        ARemnantbornCharacterBase* CharacterBase = Cast<ARemnantbornCharacterBase>(SpawnedPawn);
        if (CharacterBase)
        {
            UE_LOG(LogTemp, Log, TEXT("SpawnPlayerWithCharacter: Setting up GAS for character %s"), 
                *SelectedCharacter->CharacterName.ToString());
            
            // Grant starting abilities if available
            // Note: ASC is initialized in PossessedBy() on the character
            if (UAbilitySystemComponent* ASC = CharacterBase->GetAbilitySystemComponent())
            {
                int32 AbilityCount = SelectedCharacter->StartingAbilities.Num();
                UE_LOG(LogTemp, Log, TEXT("SpawnPlayerWithCharacter: Character %s has %d starting abilities"), 
                    *SelectedCharacter->CharacterName.ToString(), AbilityCount);
                
                if (AbilityCount > 0)
                {
                    for (int32 i = 0; i < AbilityCount; ++i)
                    {
                        if (SelectedCharacter->StartingAbilities[i])
                        {
                            UE_LOG(LogTemp, Log, TEXT("SpawnPlayerWithCharacter: Ability %d: %s"), 
                                i, *SelectedCharacter->StartingAbilities[i]->GetName());
                        }
                    }
                    
                    TArray<FGameplayAbilitySpecHandle> GrantedHandles = CharacterBase->GrantAbilities(SelectedCharacter->StartingAbilities);
                    UE_LOG(LogTemp, Log, TEXT("SpawnPlayerWithCharacter: Granted %d abilities for character %s"), 
                        GrantedHandles.Num(), *SelectedCharacter->CharacterName.ToString());
                    
                    // Notify character that abilities are ready for UI setup
                    CharacterBase->OnAbilitiesGrantedAndReady();
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("SpawnPlayerWithCharacter: No starting abilities to grant for character %s"), 
                        *SelectedCharacter->CharacterName.ToString());
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("SpawnPlayerWithCharacter: Character %s has no ASC!"), 
                    *SelectedCharacter->CharacterName.ToString());
            }
            
            UE_LOG(LogTemp, Log, TEXT("Successfully spawned character %s for player %s"),
                *SelectedCharacter->CharacterName.ToString(), *PlayerState->GetPlayerName());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Spawned pawn is not a RemnantbornCharacterBase for player %s"), *PlayerState->GetPlayerName());
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

    // Max retry limit - apply default character if we can't get a selection
    if (RetryCount >= 15)
    {
        UE_LOG(LogTemp, Warning, TEXT("RetrySpawn: Max retries reached for player, attempting to apply default character"));
        
        // Try to apply default character
        if (ApplyDefaultCharacterIfNeeded(PlayerController))
        {
            UE_LOG(LogTemp, Log, TEXT("RetrySpawn: Applied default character after max retries"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("RetrySpawn: Failed to apply default character, spawning with default pawn class"));
            if (DefaultPawnClass)
            {
                RestartPlayer(PlayerController);
            }
        }
        return;
    }

    ACharacterPlayerState* PlayerState = PlayerController->GetPlayerState<ACharacterPlayerState>();
    if (!PlayerState)
    {
        UE_LOG(LogTemp, Warning, TEXT("RetrySpawn: PlayerState not available yet (retry %d/15)"), RetryCount + 1);
        // Retry with delay
        FTimerHandle RetryTimer;
        FTimerDelegate RetryDelegate;
        RetryDelegate.BindUFunction(this, "RetrySpawnPlayerWithCharacter", PlayerController, RetryCount + 1);
        GetWorld()->GetTimerManager().SetTimer(RetryTimer, RetryDelegate, 0.3f, false);
        return;
    }

    // Try to apply character selection from GameSession if not already done
    if (!PlayerState->HasSelectedCharacter())
    {
        if (ApplyCharacterSelectionFromGameSession(PlayerController))
        {
            UE_LOG(LogTemp, Log, TEXT("RetrySpawn: Applied character selection from GameSession (retry %d/15)"), RetryCount + 1);
        }
    }

    if (!PlayerState->HasSelectedCharacter())
    {
        UE_LOG(LogTemp, Warning, TEXT("RetrySpawn: Character selection not available yet (retry %d/15)"), RetryCount + 1);
        
        // Try to restore from GameInstance as backup (for listen server host)
        if (UMyOnlineGameInstance* GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance()))
        {
            FName LocalCharacterSelection = GameInstance->GetLocalCharacterSelection();
            if (!LocalCharacterSelection.IsNone())
            {
                UE_LOG(LogTemp, Log, TEXT("RetrySpawn: Restoring character selection from GameInstance: %s"), *LocalCharacterSelection.ToString());
                PlayerState->Server_SetSelectedCharacterID(LocalCharacterSelection);
            }
        }
        
        // For remote clients, give them more time to send their selection via RPC
        // Don't apply default until we've given them enough chances
        if (RetryCount < 10)
        {
            // Retry with delay
            FTimerHandle RetryTimer;
            FTimerDelegate RetryDelegate;
            RetryDelegate.BindUFunction(this, "RetrySpawnPlayerWithCharacter", PlayerController, RetryCount + 1);
            GetWorld()->GetTimerManager().SetTimer(RetryTimer, RetryDelegate, 0.3f, false);
            return;
        }
        else
        {
            // We've waited long enough, apply default character
            UE_LOG(LogTemp, Warning, TEXT("RetrySpawn: Waited %d retries, applying default character"), RetryCount);
            if (ApplyDefaultCharacterIfNeeded(PlayerController))
            {
                UE_LOG(LogTemp, Log, TEXT("RetrySpawn: Applied default character"));
            }
        }
    }

    if (!PlayerState->IsCharacterDataReady())
    {
        UE_LOG(LogTemp, Warning, TEXT("RetrySpawn: Character data not ready yet (retry %d/15)"), RetryCount + 1);
        // Retry with delay
        FTimerHandle RetryTimer;
        FTimerDelegate RetryDelegate;
        RetryDelegate.BindUFunction(this, "RetrySpawnPlayerWithCharacter", PlayerController, RetryCount + 1);
        GetWorld()->GetTimerManager().SetTimer(RetryTimer, RetryDelegate, 0.3f, false);
        return;
    }

    // All checks passed, spawn the character
    UE_LOG(LogTemp, Log, TEXT("RetrySpawn: Spawning character for player %s (attempt %d)"), 
        PlayerState ? *PlayerState->GetPlayerName() : TEXT("Unknown"), RetryCount + 1);
    SpawnPlayerWithCharacter(PlayerController);
}

bool AMultiplayerGameMode::ApplyCharacterSelectionFromGameSession(APlayerController* PlayerController)
{
    if (!PlayerController)
    {
        return false;
    }

    ACharacterPlayerState* PlayerState = PlayerController->GetPlayerState<ACharacterPlayerState>();
    if (!PlayerState)
    {
        return false;
    }

    // Check if we already have a selection
    if (PlayerState->HasSelectedCharacter())
    {
        return true;
    }

    // Try to get selection from GameInstance (persists through seamless travel)
    UMyOnlineGameInstance* GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance());
    if (!GameInstance)
    {
        UE_LOG(LogTemp, Warning, TEXT("ApplyCharacterSelectionFromGameSession: GameInstance is not UMyOnlineGameInstance (actual class: %s)"),
            GetGameInstance() ? *GetGameInstance()->GetClass()->GetName() : TEXT("NULL"));
        return false;
    }

    // Use player name instead of PlayerId because PlayerId can change during seamless travel
    FString PlayerName = PlayerState->GetPlayerName();

    // Debug: Log the GameInstance and all stored selections
    UE_LOG(LogTemp, Log, TEXT("ApplyCharacterSelectionFromGameSession: Looking for player '%s' in GameInstance %p"),
        *PlayerName, (void*)GameInstance);

    TMap<FString, FName> AllSelections = GameInstance->GetAllCharacterSelections();
    UE_LOG(LogTemp, Log, TEXT("ApplyCharacterSelectionFromGameSession: GameInstance has %d stored selections:"), AllSelections.Num());
    for (const auto& Pair : AllSelections)
    {
        UE_LOG(LogTemp, Log, TEXT("  - Player: '%s' -> Character: '%s'"), *Pair.Key, *Pair.Value.ToString());
    }

    FName CharacterID = GameInstance->GetPlayerCharacterSelection(PlayerName);

    if (CharacterID.IsNone())
    {
        UE_LOG(LogTemp, Warning, TEXT("ApplyCharacterSelectionFromGameSession: No character selection stored for player '%s'"), *PlayerName);
        return false;
    }

    // Apply the character selection
    PlayerState->Server_SetSelectedCharacterID(CharacterID);

    UE_LOG(LogTemp, Log, TEXT("ApplyCharacterSelectionFromGameSession: Applied character %s for player %s"),
        *CharacterID.ToString(), *PlayerName);

    return true;
}

UCharacterDataAsset* AMultiplayerGameMode::GetDefaultCharacter() const
{
    if (UCharacterSelectionSubsystem* CharacterSubsystem = GetGameInstance()->GetSubsystem<UCharacterSelectionSubsystem>())
    {
        TArray<UCharacterDataAsset*> AvailableCharacters = CharacterSubsystem->GetAvailableCharacters();
        if (AvailableCharacters.Num() > 0)
        {
            // Return the first character as default
            return AvailableCharacters[0];
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("GetDefaultCharacter: No available characters found in subsystem"));
    return nullptr;
}

bool AMultiplayerGameMode::ApplyDefaultCharacterIfNeeded(APlayerController* PlayerController)
{
    if (!PlayerController)
    {
        return false;
    }

    ACharacterPlayerState* PlayerState = PlayerController->GetPlayerState<ACharacterPlayerState>();
    if (!PlayerState)
    {
        return false;
    }

    // Check if we already have a selection
    if (PlayerState->HasSelectedCharacter())
    {
        return true;
    }

    // Get default character
    UCharacterDataAsset* DefaultCharacter = GetDefaultCharacter();
    if (!DefaultCharacter)
    {
        UE_LOG(LogTemp, Error, TEXT("ApplyDefaultCharacterIfNeeded: Could not get default character"));
        return false;
    }

// Apply the default character
    FName DefaultCharacterID = DefaultCharacter->CharacterID;
    PlayerState->Server_SetSelectedCharacterID(DefaultCharacterID);
    
    UE_LOG(LogTemp, Log, TEXT("ApplyDefaultCharacterIfNeeded: Applied default character %s for player %s"), 
        *DefaultCharacterID.ToString(), *PlayerState->GetPlayerName());
    
    return true;
}

void AMultiplayerGameMode::InitializeMatchTracking()
{
    AMultiplayerMatchGameState* MatchGameState = GetGameState<AMultiplayerMatchGameState>();
    if (MatchGameState)
    {
        bRewardsDispatched = false;
        MatchGameState->InitializePlayerResults();
        UE_LOG(LogTemp, Log, TEXT("MultiplayerGameMode: Match tracking initialized for %d players"), 
            MatchGameState->PlayerResults.Num());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("MultiplayerGameMode: Failed to get MultiplayerMatchGameState"));
    }
}

void AMultiplayerGameMode::OnPlayerDied(APlayerController* PlayerController)
{
    NotifyPlayerDied(PlayerController);
}

void AMultiplayerGameMode::NotifyPlayerDied(APlayerController* PlayerController)
{
    if (!PlayerController)
    {
        return;
    }

    AMultiplayerMatchGameState* MatchGameState = GetGameState<AMultiplayerMatchGameState>();
    if (!MatchGameState)
    {
        UE_LOG(LogTemp, Warning, TEXT("MultiplayerGameMode: No match game state available for player death tracking"));
        return;
    }

    ACharacterPlayerState* PlayerState = PlayerController->GetPlayerState<ACharacterPlayerState>();
    if (!PlayerState)
    {
        UE_LOG(LogTemp, Warning, TEXT("MultiplayerGameMode: No player state for dead player"));
        return;
    }

    // Check if match is already finished - don't process further deaths
    if (MatchGameState->IsMatchFinished())
    {
        UE_LOG(LogTemp, Warning, TEXT("MultiplayerGameMode: Match already finished, ignoring death of %s"), *PlayerState->GetPlayerName());
        return;
    }

    FString PlayerName = PlayerState->GetPlayerName();
    int32 PlayerId = PlayerState->GetPlayerId();

    UE_LOG(LogTemp, Log, TEXT("MultiplayerGameMode: Player %s died, notifying match state"), *PlayerName);
    
    // Notify the dead player immediately (for death cam/effects)
    if (AMultiplayerPlayerController* MPC = Cast<AMultiplayerPlayerController>(PlayerController))
    {
        MPC->Client_NotifyPlayerDeath(TEXT("")); // Empty string for now, can be extended to pass killer info
    }
    
    // Notify the game state about the player death
    MatchGameState->OnPlayerDeath(PlayerName, PlayerId);

    // Check if match is finished and show results to all players
    if (MatchGameState->IsMatchFinished())
    {
        if (bRewardsDispatched)
        {
            return;
        }

        bRewardsDispatched = true;
        UE_LOG(LogTemp, Log, TEXT("MultiplayerGameMode: Match finished, showing results to all players in 2 seconds..."));
        
        // Use a timer to show results after a short delay (allows for death animations)
        FTimerHandle ShowResultsTimer;
        GetWorld()->GetTimerManager().SetTimer(ShowResultsTimer, [this]()
        {
            AMultiplayerMatchGameState* LocalMatchGameState = GetGameState<AMultiplayerMatchGameState>();
            const float MatchDuration = LocalMatchGameState ? LocalMatchGameState->GetMatchDuration() : 0.0f;
            const FString MatchId = FString::Printf(TEXT("%s-%lld"),
                *UGameplayStatics::GetCurrentLevelName(this),
                FDateTime::UtcNow().ToUnixTimestamp());

            // Show match results to all players
            for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
            {
                if (AMultiplayerPlayerController* PC = Cast<AMultiplayerPlayerController>(It->Get()))
                {
                    bool bIsWinner = false;
                    int32 EliminationOrder = 0;

                    if (LocalMatchGameState)
                    {
                        if (ACharacterPlayerState* CharacterPlayerState = PC->GetPlayerState<ACharacterPlayerState>())
                        {
                            const FPlayerMatchResult Result = LocalMatchGameState->GetPlayerResult(CharacterPlayerState->GetPlayerName());
                            bIsWinner = Result.bIsWinner;
                            EliminationOrder = Result.EliminationOrder;
                        }
                    }

                    PC->Client_SubmitMatchReward(bIsWinner, MatchDuration, EliminationOrder, MatchId);
                    PC->Client_ShowMatchResults();
                }
            }
        }, 2.0f, false);
    }
}

void AMultiplayerGameMode::CheckForPlayerDeaths()
{
    if (!HasAuthority())
    {
        return;
    }

    AMultiplayerMatchGameState* MatchGameState = GetGameState<AMultiplayerMatchGameState>();
    if (!MatchGameState)
    {
        UE_LOG(LogTemp, Error, TEXT("CheckForPlayerDeaths: No MatchGameState found!"));
        return;
    }

    if (MatchGameState->IsMatchFinished())
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    // Debug: Log how many controllers we're checking
    int32 ControllerCount = 0;
    int32 DeadCount = 0;

    // Check all player controllers for dead characters
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        ControllerCount++;
        APlayerController* PC = It->Get();
        if (!PC)
        {
            continue;
        }

        // Get the character pawn
        APawn* Pawn = PC->GetPawn();
        if (!Pawn)
        {
            continue;
        }

        // Check if this is a Remnantborn character
        ARemnantbornCharacterBase* Character = Cast<ARemnantbornCharacterBase>(Pawn);
        if (!Character)
        {
            continue;
        }

        // Check if character is dead (either by tag OR by health <= 0)
        if (UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent())
        {
            static FGameplayTag DeathTag = FGameplayTag::RequestGameplayTag("State.Dead");
            bool bHasDeathTag = ASC->HasMatchingGameplayTag(DeathTag);
            
            // Also check health directly as a backup
            bool bIsHealthZero = false;
            if (const UBasicAttributeSet* BasicAttrSet = Cast<UBasicAttributeSet>(ASC->GetAttributeSet(UBasicAttributeSet::StaticClass())))
            {
                bIsHealthZero = BasicAttrSet->GetHealth() <= 0.0f;
            }
            
            bool bIsDead = bHasDeathTag || bIsHealthZero;
            
            if (bIsDead)
            {
                DeadCount++;
                
                // Check if we've already recorded this death
                ACharacterPlayerState* PlayerState = PC->GetPlayerState<ACharacterPlayerState>();
                if (PlayerState)
                {
                    FString PlayerName = PlayerState->GetPlayerName();
                    FPlayerMatchResult ExistingResult = MatchGameState->GetPlayerResult(PlayerName);
                    
                    UE_LOG(LogTemp, Log, TEXT("CheckForPlayerDeaths: Found dead player %s - bIsAlive=%s (DeathTag=%s, HealthZero=%s)"), 
                        *PlayerName, 
                        ExistingResult.bIsAlive ? TEXT("true") : TEXT("false"),
                        bHasDeathTag ? TEXT("YES") : TEXT("NO"),
                        bIsHealthZero ? TEXT("YES") : TEXT("NO"));
                    
                    if (ExistingResult.bIsAlive) // Still marked as alive, so this is a new death
                    {
                        UE_LOG(LogTemp, Warning, TEXT("CheckForPlayerDeaths: Processing NEW death for %s"), *PlayerName);
                        
                        // Ensure the death tag is applied for consistency
                        if (!bHasDeathTag && bIsHealthZero)
                        {
                            UE_LOG(LogTemp, Warning, TEXT("CheckForPlayerDeaths: Player %s has 0 health but no death tag - applying tag now"), *PlayerName);
                            ASC->AddLooseGameplayTag(DeathTag);
                        }
                        
                        NotifyPlayerDied(PC);
                    }
                    else
                    {
                        UE_LOG(LogTemp, Verbose, TEXT("CheckForPlayerDeaths: Player %s already recorded as dead"), *PlayerName);
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("CheckForPlayerDeaths: Dead character has no PlayerState!"));
                }
            }
        }
    }

    // Debug log every few seconds
    static float LastDebugTime = 0.0f;
    float CurrentTime = World->GetTimeSeconds();
    if (CurrentTime - LastDebugTime > 5.0f)
    {
        UE_LOG(LogTemp, Log, TEXT("CheckForPlayerDeaths: Checking %d controllers, found %d dead. AlivePlayerCount=%d"), 
            ControllerCount, DeadCount, MatchGameState->AlivePlayerCount);
        LastDebugTime = CurrentTime;
    }
}