#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Remnantborn/Remnantborn/GameModes/MultiplayerMatchGameState.h"
#include "MatchResultsWidget.generated.h"

class UTextBlock;
class UButton;
class UVerticalBox;
class UScrollBox;

UCLASS()
class REMNANTBORN_API UMatchResultsWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // Initialize the widget with match results
    UFUNCTION(BlueprintCallable, Category = "Match Results")
    void InitializeMatchResults();

    // Update the UI when match state changes
    UFUNCTION(BlueprintCallable, Category = "Match Results")
    void OnMatchStateChanged(EMatchState NewState);

    // Update the results display
    UFUNCTION(BlueprintCallable, Category = "Match Results")
    void UpdateResultsDisplay();

protected:
    // Main title text
    UPROPERTY(meta = (BindWidget))
    UTextBlock* TitleText;

    // Winner announcement
    UPROPERTY(meta = (BindWidget))
    UTextBlock* WinnerText;

    // Results container
    UPROPERTY(meta = (BindWidget))
    UScrollBox* ResultsScrollBox;

    // Player results container
    UPROPERTY(meta = (BindWidget))
    UVerticalBox* PlayerResultsBox;

    // Action buttons
    UPROPERTY(meta = (BindWidget))
    UButton* ReturnToLobbyButton;

    UPROPERTY(meta = (BindWidget))
    UButton* PlayAgainButton;

    // UI Elements for individual player results
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> PlayerResultEntryWidgetClass;

private:
    UFUNCTION()
    void OnReturnToLobbyClicked();

    UFUNCTION()
    void OnPlayAgainClicked();

    UFUNCTION()
    void OnMatchStateChangedDelegate(EMatchState NewState);

    // Helper function to create player result entry
    void CreatePlayerResultEntry(const FPlayerMatchResult& PlayerResult, int32 Rank);

    // Reference to game state for data binding
    UPROPERTY()
    AMultiplayerMatchGameState* MatchGameState;

    // Auto-show timer
    float TimeSinceMatchEnd = 0.0f;
    bool bShouldAutoShow = false;
};