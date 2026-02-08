#include "MultiplayerMatchGameState.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerState.h"
#include "Engine/World.h"

AMultiplayerMatchGameState::AMultiplayerMatchGameState()
{
    bReplicates = true;
    SetReplicates(true);
    
    // Initialize default values
    CurrentMatchState = EMatchState::InProgress;
    AlivePlayerCount = 0;
    NextEliminationOrder = 1;
}

void AMultiplayerMatchGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AMultiplayerMatchGameState, CurrentMatchState);
    DOREPLIFETIME(AMultiplayerMatchGameState, PlayerResults);
    DOREPLIFETIME(AMultiplayerMatchGameState, AlivePlayerCount);
    DOREPLIFETIME(AMultiplayerMatchGameState, WinnerName);
    DOREPLIFETIME(AMultiplayerMatchGameState, MatchStartTime);
}

void AMultiplayerMatchGameState::BeginPlay()
{
    Super::BeginPlay();
    
    // Record match start time on server
    if (HasAuthority())
    {
        MatchStartTime = GetWorld()->GetTimeSeconds();
    }
}

void AMultiplayerMatchGameState::InitializePlayerResults()
{
    if (!HasAuthority())
    {
        return;
    }

    PlayerResults.Empty();
    AlivePlayerCount = 0;
    NextEliminationOrder = 1;
    WinnerName = TEXT("");

    // Initialize results for all connected players
    for (APlayerState* PlayerState : PlayerArray)
    {
        if (PlayerState)
        {
            FPlayerMatchResult NewResult;
            NewResult.PlayerName = PlayerState->GetPlayerName();
            NewResult.PlayerId = PlayerState->GetPlayerId();
            NewResult.bIsAlive = true;
            NewResult.bIsWinner = false;
            NewResult.SurvivalTime = 0.0f;
            NewResult.EliminationOrder = 0;

            PlayerResults.Add(NewResult);
            AlivePlayerCount++;
        }
    }

    UE_LOG(LogTemp, Log, TEXT("MultiplayerMatchGameState: Initialized %d player results"), AlivePlayerCount);
}

void AMultiplayerMatchGameState::OnPlayerDeath(const FString& PlayerName, int32 PlayerId)
{
    if (!HasAuthority())
    {
        return;
    }

    if (CurrentMatchState != EMatchState::InProgress)
    {
        return;
    }

    // Find the player result
    FPlayerMatchResult* PlayerResult = nullptr;
    for (FPlayerMatchResult& Result : PlayerResults)
    {
        if (Result.PlayerName == PlayerName && Result.PlayerId == PlayerId)
        {
            PlayerResult = &Result;
            break;
        }
    }

    if (!PlayerResult)
    {
        UE_LOG(LogTemp, Warning, TEXT("MultiplayerMatchGameState: Could not find player result for %s"), *PlayerName);
        return;
    }

    if (!PlayerResult->bIsAlive)
    {
        UE_LOG(LogTemp, Warning, TEXT("MultiplayerMatchGameState: Player %s is already dead"), *PlayerName);
        return;
    }

    // Mark player as eliminated
    PlayerResult->bIsAlive = false;
    PlayerResult->SurvivalTime = GetWorld()->GetTimeSeconds() - MatchStartTime;
    PlayerResult->EliminationOrder = NextEliminationOrder++;
    AlivePlayerCount--;

    UE_LOG(LogTemp, Log, TEXT("MultiplayerMatchGameState: Player %s eliminated (order: %d, survival time: %.2f)"), 
        *PlayerName, PlayerResult->EliminationOrder, PlayerResult->SurvivalTime);

    // Fire elimination event
    OnPlayerEliminated.Broadcast(PlayerName, PlayerResult->EliminationOrder);

    // Check for match end
    CheckForMatchEnd();
}

void AMultiplayerMatchGameState::SetMatchState(EMatchState NewState)
{
    if (!HasAuthority())
    {
        return;
    }

    if (CurrentMatchState == NewState)
    {
        return;
    }

    CurrentMatchState = NewState;
    OnMatchStateChanged.Broadcast(NewState);

    UE_LOG(LogTemp, Log, TEXT("MultiplayerMatchGameState: Match state changed to %d"), (int32)NewState);
}

void AMultiplayerMatchGameState::SetWinner(const FString& WinnerPlayerName)
{
    if (!HasAuthority())
    {
        return;
    }

    WinnerName = WinnerPlayerName;

    // Update winner status in player results
    for (FPlayerMatchResult& Result : PlayerResults)
    {
        if (Result.PlayerName == WinnerPlayerName)
        {
            Result.bIsWinner = true;
            Result.bIsAlive = true; // Winner is considered alive
            Result.SurvivalTime = GetWorld()->GetTimeSeconds() - MatchStartTime;
            break;
        }
    }

    UE_LOG(LogTemp, Log, TEXT("MultiplayerMatchGameState: Winner set to %s"), *WinnerPlayerName);
}

FPlayerMatchResult AMultiplayerMatchGameState::GetPlayerResult(const FString& PlayerName) const
{
    for (const FPlayerMatchResult& Result : PlayerResults)
    {
        if (Result.PlayerName == PlayerName)
        {
            return Result;
        }
    }
    return FPlayerMatchResult(); // Return empty result if not found
}

TArray<FPlayerMatchResult> AMultiplayerMatchGameState::GetRankedResults() const
{
    TArray<FPlayerMatchResult> RankedResults = PlayerResults;
    
    // Sort by elimination order (winner first, then by elimination order)
    RankedResults.Sort([](const FPlayerMatchResult& A, const FPlayerMatchResult& B)
    {
        if (A.bIsWinner && !B.bIsWinner) return true;
        if (!A.bIsWinner && B.bIsWinner) return false;
        return A.EliminationOrder < B.EliminationOrder;
    });

    return RankedResults;
}

float AMultiplayerMatchGameState::GetMatchDuration() const
{
    if (MatchStartTime <= 0.0f)
    {
        return 0.0f;
    }
    
    return GetWorld()->GetTimeSeconds() - MatchStartTime;
}

void AMultiplayerMatchGameState::OnRep_MatchState()
{
    OnMatchStateChanged.Broadcast(CurrentMatchState);
}

void AMultiplayerMatchGameState::OnRep_PlayerResults()
{
    // This is called when PlayerResults is replicated
    // UI can bind to this or use the getter functions
}

void AMultiplayerMatchGameState::OnRep_AlivePlayerCount()
{
    // This is called when AlivePlayerCount is replicated
}

void AMultiplayerMatchGameState::OnRep_WinnerName()
{
    // This is called when WinnerName is replicated
}

void AMultiplayerMatchGameState::UpdateAlivePlayerCount()
{
    if (!HasAuthority())
    {
        return;
    }

    AlivePlayerCount = 0;
    for (const FPlayerMatchResult& Result : PlayerResults)
    {
        if (Result.bIsAlive)
        {
            AlivePlayerCount++;
        }
    }
}

void AMultiplayerMatchGameState::CheckForMatchEnd()
{
    if (!HasAuthority())
    {
        return;
    }

    if (CurrentMatchState != EMatchState::InProgress)
    {
        return;
    }

    // Match ends when only 1 player is alive (or 0 if all eliminated simultaneously)
    if (AlivePlayerCount <= 1)
    {
        // Find the winner (last player alive)
        for (FPlayerMatchResult& Result : PlayerResults)
        {
            if (Result.bIsAlive)
            {
                SetWinner(Result.PlayerName);
                break;
            }
        }

        // If no winner found but match should end, it's a draw (all eliminated at same time)
        if (WinnerName.IsEmpty())
        {
            WinnerName = TEXT("Draw");
        }

        SetMatchState(EMatchState::Finished);
        
        UE_LOG(LogTemp, Log, TEXT("MultiplayerMatchGameState: Match ended! Winner: %s"), *WinnerName);
    }
}