#include "MatchResultsWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "Components/ScrollBox.h"
#include "Components/Image.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameModeBase.h"
#include "Remnantborn/Remnantborn/OnlineService/MyOnlineGameInstance.h"

void UMatchResultsWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // Bind button events
    if (ReturnToLobbyButton)
    {
        ReturnToLobbyButton->OnClicked.AddDynamic(this, &UMatchResultsWidget::OnReturnToLobbyClicked);
    }

    if (PlayAgainButton)
    {
        PlayAgainButton->OnClicked.AddDynamic(this, &UMatchResultsWidget::OnPlayAgainClicked);
    }

    // Initially hide the widget
    SetVisibility(ESlateVisibility::Hidden);

    // Initialize match results
    InitializeMatchResults();

    if (UMyOnlineGameInstance* GameInstance = GetGameInstance<UMyOnlineGameInstance>())
    {
        GameInstance->OnMatchRewardReceived.AddDynamic(this, &UMatchResultsWidget::DisplayMatchReward);
    }
}

void UMatchResultsWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // Auto-show after match ends with a small delay
    if (bShouldAutoShow && MatchGameState && MatchGameState->IsMatchFinished())
    {
        TimeSinceMatchEnd += InDeltaTime;
        if (TimeSinceMatchEnd >= 1.0f) // Wait 1 second after match ends
        {
            SetVisibility(ESlateVisibility::Visible);
            bShouldAutoShow = false;
        }
    }
}

void UMatchResultsWidget::InitializeMatchResults()
{
    // Get the game state
    MatchGameState = GetWorld() ? GetWorld()->GetGameState<AMultiplayerMatchGameState>() : nullptr;
    
    if (!MatchGameState)
    {
        UE_LOG(LogTemp, Warning, TEXT("MatchResultsWidget: Could not get MultiplayerMatchGameState, will retry..."));
        
        // Retry initialization after a short delay
        FTimerHandle RetryTimer;
        GetWorld()->GetTimerManager().SetTimer(RetryTimer, [this]()
        {
            if (!MatchGameState)
            {
                MatchGameState = GetWorld() ? GetWorld()->GetGameState<AMultiplayerMatchGameState>() : nullptr;
                if (MatchGameState)
                {
                    UE_LOG(LogTemp, Log, TEXT("MatchResultsWidget: Successfully got GameState on retry"));
                    MatchGameState->OnMatchStateChanged.AddDynamic(this, &UMatchResultsWidget::OnMatchStateChangedDelegate);
                    OnMatchStateChanged(MatchGameState->CurrentMatchState);
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("MatchResultsWidget: Still couldn't get GameState after retry!"));
                }
            }
        }, 1.0f, false);
        
        return;
    }

    // Bind to match state changes
    MatchGameState->OnMatchStateChanged.AddDynamic(this, &UMatchResultsWidget::OnMatchStateChangedDelegate);

    // Update initial state
    OnMatchStateChanged(MatchGameState->CurrentMatchState);
    
    UE_LOG(LogTemp, Log, TEXT("MatchResultsWidget: Successfully initialized with GameState"));
}

void UMatchResultsWidget::OnMatchStateChanged(EMatchState NewState)
{
    switch (NewState)
    {
        case EMatchState::InProgress:
            SetVisibility(ESlateVisibility::Hidden);
            bShouldAutoShow = false;
            TimeSinceMatchEnd = 0.0f;
            break;

        case EMatchState::Finished:
            {
                // Start the auto-show timer
                bShouldAutoShow = true;
                TimeSinceMatchEnd = 0.0f;
                
                // Play result music - this works on both host and client
                // because each client has their own GameInstance
                // Use GetWorld()->GetGameInstance() for more reliable access on clients
                UGameInstance* WorldGameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
                UMyOnlineGameInstance* GameInstance = WorldGameInstance ? Cast<UMyOnlineGameInstance>(WorldGameInstance) : nullptr;
                
                if (GameInstance)
                {
                    const bool bLocalPlayerIsWinner = IsLocalPlayerWinner();
                    bool bIsLocal = GetOwningPlayer() ? GetOwningPlayer()->IsLocalController() : false;
                    UE_LOG(LogTemp, Log, TEXT("MatchResultsWidget: Playing result music. IsWinner: %d, IsLocal=%d"), 
                        bLocalPlayerIsWinner, bIsLocal);
                    GameInstance->OnMatchEnded(bLocalPlayerIsWinner);
                }
                else
                {
                    bool bIsLocal = GetOwningPlayer() ? GetOwningPlayer()->IsLocalController() : false;
                    UE_LOG(LogTemp, Error, TEXT("MatchResultsWidget: Could not get GameInstance! IsLocal=%d, HasWorld=%d"), 
                        bIsLocal, GetWorld() != nullptr);
                    
                    // Try fallback methods
                    GameInstance = GetGameInstance<UMyOnlineGameInstance>();
                    if (GameInstance)
                    {
                        UE_LOG(LogTemp, Log, TEXT("MatchResultsWidget: Got GameInstance via GetGameInstance fallback"));
                        const bool bLocalPlayerIsWinner = IsLocalPlayerWinner();
                        GameInstance->OnMatchEnded(bLocalPlayerIsWinner);
                    }
                    else
                    {
                        UE_LOG(LogTemp, Error, TEXT("MatchResultsWidget: All GameInstance retrieval methods failed!"));
                    }
                }
                
                // Update the results display
                UpdateResultsDisplay();
            }
            break;

        case EMatchState::ReturningToLobby:
            // Keep showing during transition
            SetVisibility(ESlateVisibility::Visible);
            break;

        default:
            break;
    }
}

void UMatchResultsWidget::UpdateResultsDisplay()
{
    if (!MatchGameState || !TitleText || !WinnerText || !PlayerResultsBox)
    {
        return;
    }

    // Clear existing results
    PlayerResultsBox->ClearChildren();

    // Check if local player is the winner
    const bool bLocalPlayerIsWinner = IsLocalPlayerWinner();

    // Update personal result display (Victory/Defeat)
    UpdatePersonalResultDisplay(bLocalPlayerIsWinner);

    // Set title based on result
    if (bLocalPlayerIsWinner)
    {
        TitleText->SetText(FText::FromString(TEXT("VICTORY!")));
    }
    else
    {
        TitleText->SetText(FText::FromString(TEXT("DEFEAT")));
    }

    // Set winner text (showing who won)
    if (MatchGameState->WinnerName == TEXT("Draw"))
    {
        WinnerText->SetText(FText::FromString(TEXT("Match ended in a Draw!")));
    }
    else if (bLocalPlayerIsWinner)
    {
        WinnerText->SetText(FText::FromString(TEXT("You are the last survivor!")));
    }
    else
    {
        WinnerText->SetText(FText::FromString(FString::Printf(TEXT("Winner: %s"), *MatchGameState->WinnerName)));
    }

    // Get ranked results
    TArray<FPlayerMatchResult> RankedResults = MatchGameState->GetRankedResults();

    // Create player result entries
    for (int32 i = 0; i < RankedResults.Num(); ++i)
    {
        CreatePlayerResultEntry(RankedResults[i], i + 1);
    }

    UE_LOG(LogTemp, Log, TEXT("MatchResultsWidget: Updated display for %d players. Local player winner: %s"), 
        RankedResults.Num(), bLocalPlayerIsWinner ? TEXT("Yes") : TEXT("No"));
}

void UMatchResultsWidget::UpdatePersonalResultDisplay(bool bIsWinner)
{
    // Show/hide victory/defeat overlays if they exist
    if (VictoryOverlay)
    {
        VictoryOverlay->SetVisibility(bIsWinner ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    }

    if (DefeatOverlay)
    {
        DefeatOverlay->SetVisibility(bIsWinner ? ESlateVisibility::Hidden : ESlateVisibility::Visible);
    }

    // Update personal result text if it exists
    if (PersonalResultText)
    {
        if (bIsWinner)
        {
            PersonalResultText->SetText(FText::FromString(TEXT("YOU WIN!")));
            PersonalResultText->SetColorAndOpacity(FLinearColor(1.0f, 0.84f, 0.0f, 1.0f)); // Gold color
        }
        else
        {
            PersonalResultText->SetText(FText::FromString(TEXT("ELIMINATED")));
            PersonalResultText->SetColorAndOpacity(FLinearColor(0.8f, 0.1f, 0.1f, 1.0f)); // Red color
        }
    }
}

bool UMatchResultsWidget::IsLocalPlayerWinner() const
{
    if (!MatchGameState)
    {
        return false;
    }

    // Get the local player controller
    if (APlayerController* LocalPC = GetOwningPlayer())
    {
        if (APlayerState* LocalPlayerState = LocalPC->GetPlayerState<APlayerState>())
        {
            const FString LocalPlayerName = LocalPlayerState->GetPlayerName();
            
            // Check if this player's result shows them as winner
            for (const FPlayerMatchResult& Result : MatchGameState->PlayerResults)
            {
                if (Result.PlayerName == LocalPlayerName)
                {
                    return Result.bIsWinner;
                }
            }
        }
    }

    return false;
}

void UMatchResultsWidget::CreatePlayerResultEntry(const FPlayerMatchResult& PlayerResult, int32 Rank)
{
    // Create a simple text entry for now (in a real implementation, you might want a dedicated widget)
    UTextBlock* PlayerEntry = NewObject<UTextBlock>(this);
    if (PlayerEntry)
    {
        // Check if this entry is for the local player
        bool bIsLocalPlayer = false;
        if (APlayerController* LocalPC = GetOwningPlayer())
        {
            if (APlayerState* LocalPlayerState = LocalPC->GetPlayerState<APlayerState>())
            {
                bIsLocalPlayer = (PlayerResult.PlayerName == LocalPlayerState->GetPlayerName());
            }
        }

        FString EntryText;
        if (PlayerResult.bIsWinner)
        {
            EntryText = FString::Printf(TEXT("%d. %s - WINNER (Survived: %.1fs)"), 
                Rank, *PlayerResult.PlayerName, PlayerResult.SurvivalTime);
        }
        else if (PlayerResult.bIsAlive)
        {
            EntryText = FString::Printf(TEXT("%d. %s - Survived (Time: %.1fs)"), 
                Rank, *PlayerResult.PlayerName, PlayerResult.SurvivalTime);
        }
        else
        {
            EntryText = FString::Printf(TEXT("%d. %s - Eliminated #%d (Survived: %.1fs)"), 
                Rank, *PlayerResult.PlayerName, PlayerResult.EliminationOrder, PlayerResult.SurvivalTime);
        }

        PlayerEntry->SetText(FText::FromString(EntryText));
        
        // Style the text
        FSlateFontInfo FontInfo = PlayerEntry->GetFont();
        FontInfo.Size = 18;
        
        // Highlight local player entry
        if (bIsLocalPlayer)
        {
            FontInfo.Size = 22; // Larger font for local player
        }
        
        PlayerEntry->SetFont(FontInfo);
        
        if (PlayerResult.bIsWinner)
        {
            PlayerEntry->SetColorAndOpacity(FLinearColor(1.0f, 0.84f, 0.0f, 1.0f)); // Gold for winner
        }
        else if (bIsLocalPlayer)
        {
            PlayerEntry->SetColorAndOpacity(FLinearColor(0.0f, 0.5f, 1.0f, 1.0f)); // Blue for local player
        }
        else if (!PlayerResult.bIsAlive)
        {
            PlayerEntry->SetColorAndOpacity(FLinearColor::Gray);
        }
        else
        {
            PlayerEntry->SetColorAndOpacity(FLinearColor::White);
        }

        // Add to the results box
        PlayerResultsBox->AddChildToVerticalBox(PlayerEntry);
    }
}

void UMatchResultsWidget::OnReturnToLobbyClicked()
{
    // Stop result music and play menu music
    if (UMyOnlineGameInstance* GameInstance = GetGameInstance<UMyOnlineGameInstance>())
    {
        bool bIsLocal = GetOwningPlayer() ? GetOwningPlayer()->IsLocalController() : false;
        UE_LOG(LogTemp, Log, TEXT("MatchResultsWidget: Returning to lobby, IsLocal=%d"), bIsLocal);
        GameInstance->OnReturningToLobby();
    }
    
    // Return to main menu/lobby
    if (APlayerController* PC = GetOwningPlayer())
    {
        UGameplayStatics::OpenLevel(PC, FName("LobbyMap"), true);
    }
}

void UMatchResultsWidget::OnPlayAgainClicked()
{
    // Stop result music and prepare for new match
    if (UMyOnlineGameInstance* GameInstance = GetGameInstance<UMyOnlineGameInstance>())
    {
        bool bIsLocal = GetOwningPlayer() ? GetOwningPlayer()->IsLocalController() : false;
        UE_LOG(LogTemp, Log, TEXT("MatchResultsWidget: Play again, IsLocal=%d"), bIsLocal);
        GameInstance->PrepareForLevelTravel();
    }
    
    // Restart the current level for a new match
    if (APlayerController* PC = GetOwningPlayer())
    {
        UGameplayStatics::OpenLevel(PC, FName(GetWorld()->GetMapName()), true);
    }
}

void UMatchResultsWidget::OnMatchStateChangedDelegate(EMatchState NewState)
{
    OnMatchStateChanged(NewState);
}

void UMatchResultsWidget::DisplayMatchReward(int32 RewardAmount, int32 NewBalance, bool bIsWinner)
{
    if (RewardAmountText)
    {
        RewardAmountText->SetText(FText::FromString(FString::Printf(TEXT("+%d Remnants"), RewardAmount)));
        RewardAmountText->SetColorAndOpacity(bIsWinner
            ? FSlateColor(FLinearColor(1.0f, 0.84f, 0.0f, 1.0f))
            : FSlateColor(FLinearColor(0.85f, 0.85f, 0.85f, 1.0f)));
    }

    if (NewBalanceText)
    {
        NewBalanceText->SetText(FText::FromString(FString::Printf(TEXT("Balance: %d"), NewBalance)));
    }

    if (RewardRemnantIcon)
    {
        RewardRemnantIcon->SetVisibility(ESlateVisibility::Visible);
    }
}