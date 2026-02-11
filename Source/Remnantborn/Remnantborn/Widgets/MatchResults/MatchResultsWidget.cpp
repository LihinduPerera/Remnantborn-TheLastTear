#include "MatchResultsWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "Components/ScrollBox.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameModeBase.h"

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
        UE_LOG(LogTemp, Warning, TEXT("MatchResultsWidget: Could not get MultiplayerMatchGameState"));
        return;
    }

    // Bind to match state changes
    MatchGameState->OnMatchStateChanged.AddDynamic(this, &UMatchResultsWidget::OnMatchStateChangedDelegate);

    // Update initial state
    OnMatchStateChanged(MatchGameState->CurrentMatchState);
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
            // Start the auto-show timer
            bShouldAutoShow = true;
            TimeSinceMatchEnd = 0.0f;
            
            // Update the results display
            UpdateResultsDisplay();
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
    // Return to main menu/lobby
    if (APlayerController* PC = GetOwningPlayer())
    {
        UGameplayStatics::OpenLevel(PC, FName("LobbyMap"), true);
    }
}

void UMatchResultsWidget::OnPlayAgainClicked()
{
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