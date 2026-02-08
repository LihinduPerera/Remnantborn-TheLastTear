#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "MultiplayerMatchGameState.generated.h"

UENUM(BlueprintType)
enum class EMatchState : uint8
{
    InProgress,
    Finished,
    ReturningToLobby
};

USTRUCT(BlueprintType)
struct FPlayerMatchResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString PlayerName = TEXT("");

    UPROPERTY(BlueprintReadOnly)
    int32 PlayerId = 0;

    UPROPERTY(BlueprintReadOnly)
    bool bIsAlive = true;

    UPROPERTY(BlueprintReadOnly)
    bool bIsWinner = false;

    UPROPERTY(BlueprintReadOnly)
    float SurvivalTime = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    int32 EliminationOrder = 0; // 0 for winner, 1 for first eliminated, etc.

    FPlayerMatchResult() {}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchStateChanged, EMatchState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerEliminated, const FString&, PlayerName, int32, EliminationOrder);

UCLASS()
class REMNANTBORN_API AMultiplayerMatchGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    AMultiplayerMatchGameState();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void BeginPlay() override;

    // Match state
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
    EMatchState CurrentMatchState = EMatchState::InProgress;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
    TArray<FPlayerMatchResult> PlayerResults;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
    int32 AlivePlayerCount = 0;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
    FString WinnerName = TEXT("");

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
    float MatchStartTime = 0.0f;

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Match|Events")
    FOnMatchStateChanged OnMatchStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "Match|Events")
    FOnPlayerEliminated OnPlayerEliminated;

    // Server-side functions (called by GameMode)
    UFUNCTION(BlueprintAuthorityOnly, Category = "Match")
    void InitializePlayerResults();

    UFUNCTION(BlueprintAuthorityOnly, Category = "Match")
    void OnPlayerDeath(const FString& PlayerName, int32 PlayerId);

    UFUNCTION(BlueprintAuthorityOnly, Category = "Match")
    void SetMatchState(EMatchState NewState);

    UFUNCTION(BlueprintAuthorityOnly, Category = "Match")
    void SetWinner(const FString& WinnerPlayerName);

    // Client-side utility functions
    UFUNCTION(BlueprintPure, Category = "Match")
    bool IsMatchFinished() const { return CurrentMatchState == EMatchState::Finished; }

    UFUNCTION(BlueprintPure, Category = "Match")
    FPlayerMatchResult GetPlayerResult(const FString& PlayerName) const;

    UFUNCTION(BlueprintPure, Category = "Match")
    TArray<FPlayerMatchResult> GetRankedResults() const;

    UFUNCTION(BlueprintPure, Category = "Match")
    float GetMatchDuration() const;

protected:
    UFUNCTION()
    void OnRep_MatchState();

    UFUNCTION()
    void OnRep_PlayerResults();

    UFUNCTION()
    void OnRep_AlivePlayerCount();

    UFUNCTION()
    void OnRep_WinnerName();

private:
    int32 NextEliminationOrder = 1;

    void UpdateAlivePlayerCount();
    void CheckForMatchEnd();
};