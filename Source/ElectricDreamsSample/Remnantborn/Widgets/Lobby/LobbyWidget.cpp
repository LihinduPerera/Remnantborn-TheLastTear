#include "LobbyWidget.h"
#include "ElectricDreamsSample/Remnantborn/GameModes/LobbyGameMode.h"
#include "ElectricDreamsSample/Remnantborn/OnlineService/LobbyPlayerController/LobbyPlayerController.h"
#include "ElectricDreamsSample/Remnantborn/OnlineService/MyOnlineGameInstance.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "PlayerListEntryWidget.h"
#include "Kismet/GameplayStatics.h"

void ULobbyWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    // Bind button events
    if (ReadyButton)
    {
        ReadyButton->OnClicked.AddDynamic(this, &ULobbyWidget::OnReadyButtonClicked);
    }
    
    if (StartButton)
    {
        StartButton->OnClicked.AddDynamic(this, &ULobbyWidget::StartMatchCountdown);
    }
    
    if (CancelButton)
    {
        CancelButton->OnClicked.AddDynamic(this, &ULobbyWidget::CancelMatchCountdown);
    }
    
    if (LeaveButton)
    {
        LeaveButton->OnClicked.AddDynamic(this, &ULobbyWidget::LeaveLobby);
    }
    
    if (Set2PlayersButton)
    {
        Set2PlayersButton->OnClicked.AddDynamic(this, &ULobbyWidget::OnSet2PlayersClicked);
    }
    
    if (Set4PlayersButton)
    {
        Set4PlayersButton->OnClicked.AddDynamic(this, &ULobbyWidget::OnSet4PlayersClicked);
    }
    
    // Bind to lobby events
    ALobbyGameMode* LobbyGameMode = GetLobbyGameMode();
    if (LobbyGameMode)
    {
        LobbyGameMode->OnLobbyReadyChanged.AddDynamic(this, &ULobbyWidget::HandleLobbyReadyChanged);
        LobbyGameMode->OnLobbyCountdownStarted.AddDynamic(this, &ULobbyWidget::HandleCountdownStarted);
        LobbyGameMode->OnLobbyCountdownUpdated.AddDynamic(this, &ULobbyWidget::HandleCountdownUpdated);
        LobbyGameMode->OnLobbyCountdownCancelled.AddDynamic(this, &ULobbyWidget::HandleCountdownCancelled);
    }
    
    // Initial update
    UpdateUI();
}

void ULobbyWidget::NativeDestruct()
{
    // Unbind events
    ALobbyGameMode* LobbyGameMode = GetLobbyGameMode();
    if (LobbyGameMode)
    {
        LobbyGameMode->OnLobbyReadyChanged.RemoveAll(this);
        LobbyGameMode->OnLobbyCountdownStarted.RemoveAll(this);
        LobbyGameMode->OnLobbyCountdownUpdated.RemoveAll(this);
        LobbyGameMode->OnLobbyCountdownCancelled.RemoveAll(this);
    }
    
    Super::NativeDestruct();
}

void ULobbyWidget::UpdatePlayerList()
{
    // Clear existing entries
    for (UPlayerListEntryWidget* Entry : PlayerEntries)
    {
        if (Entry)
        {
            Entry->RemoveFromParent();
        }
    }
    PlayerEntries.Empty();
    
    if (!PlayerListContainer || !PlayerListEntryClass)
    {
        return;
    }
    
    // Get all player controllers
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }
    
    // This is a simplified version - in a real implementation,
    // you would get player information from the game state or replicate it
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        if (APlayerController* PC = It->Get())
        {
            UPlayerListEntryWidget* Entry = CreateWidget<UPlayerListEntryWidget>(this, PlayerListEntryClass);
            if (Entry && PlayerListContainer)
            {
                // Set player info
                FString PlayerName = PC->IsLocalController() ? "You" : FString::Printf(TEXT("Player %d"), It.GetIndex());
                Entry->SetPlayerInfo(PlayerName, false); // Ready status would come from player controller
                
                PlayerListContainer->AddChildToVerticalBox(Entry);
                PlayerEntries.Add(Entry);
            }
        }
    }
}

void ULobbyWidget::SetReadyStatus(bool bIsReady)
{
    ALobbyPlayerController* LobbyPC = GetLobbyPlayerController();
    if (LobbyPC)
    {
        LobbyPC->SetPlayerReady(bIsReady);
    }
}

void ULobbyWidget::OnReadyButtonClicked()
{
    ALobbyPlayerController* LobbyPC = GetLobbyPlayerController();
    if (LobbyPC)
    {
        LobbyPC->SetPlayerReady(!LobbyPC->IsReady());
    }
}

void ULobbyWidget::OnSet2PlayersClicked()
{
    SetMaxPlayers(2);
}

void ULobbyWidget::OnSet4PlayersClicked()
{
    SetMaxPlayers(4);
}

void ULobbyWidget::StartMatchCountdown()
{
    ALobbyPlayerController* LobbyPC = GetLobbyPlayerController();
    if (LobbyPC)
    {
        LobbyPC->StartMatchCountdown();
    }
}

void ULobbyWidget::CancelMatchCountdown()
{
    ALobbyPlayerController* LobbyPC = GetLobbyPlayerController();
    if (LobbyPC)
    {
        LobbyPC->CancelMatchCountdown();
    }
}

void ULobbyWidget::SetMaxPlayers(int32 MaxPlayers)
{
    ALobbyPlayerController* LobbyPC = GetLobbyPlayerController();
    if (LobbyPC)
    {
        LobbyPC->SetMaxPlayers(MaxPlayers);
    }
}

void ULobbyWidget::LeaveLobby()
{
    UMyOnlineGameInstance* GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance());
    if (GameInstance)
    {
        GameInstance->LeaveGame();
    }
}

void ULobbyWidget::HandleLobbyReadyChanged()
{
    UpdateUI();
}

void ULobbyWidget::HandleCountdownStarted()
{
    UpdateUI();
}

void ULobbyWidget::HandleCountdownUpdated()
{
    UpdateCountdownText();
}

void ULobbyWidget::HandleCountdownCancelled()
{
    UpdateUI();
}

ALobbyGameMode* ULobbyWidget::GetLobbyGameMode() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }
    
    return Cast<ALobbyGameMode>(World->GetAuthGameMode());
}

ALobbyPlayerController* ULobbyWidget::GetLobbyPlayerController() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }
    
    return Cast<ALobbyPlayerController>(World->GetFirstPlayerController());
}

void ULobbyWidget::UpdateUI()
{
    ALobbyGameMode* LobbyGameMode = GetLobbyGameMode();
    ALobbyPlayerController* LobbyPC = GetLobbyPlayerController();
    
    if (!LobbyGameMode || !LobbyPC)
    {
        return;
    }
    
    // Update player list
    UpdatePlayerList();
    
    // Update player count
    UpdatePlayerCountText();
    
    // Update countdown text
    UpdateCountdownText();
    
    // Update host controls visibility
    UpdateHostControls();
    
    // Update button states
    if (ReadyButton)
    {
        FText ButtonText = LobbyPC->IsReady() ? FText::FromString("Not Ready") : FText::FromString("Ready");
        // In C++ we need to access the button's text differently or use a different approach
        // For now, we'll just enable/disable based on logic
    }
    
    if (StartButton)
    {
        StartButton->SetIsEnabled(LobbyGameMode->CanStartMatch() && !LobbyGameMode->IsCountdownActive());
    }
    
    if (CancelButton)
    {
        CancelButton->SetIsEnabled(LobbyGameMode->IsCountdownActive());
    }
    
    if (StatusText)
    {
        if (LobbyGameMode->IsCountdownActive())
        {
            StatusText->SetText(FText::FromString("Match starting soon..."));
        }
        else if (LobbyGameMode->CanStartMatch())
        {
            StatusText->SetText(FText::FromString("Ready to start!"));
        }
        else
        {
            FString Status = FString::Printf(TEXT("Waiting for players... (%d/%d)"),
                LobbyGameMode->GetCurrentPlayerCount(),
                LobbyGameMode->GetMaxPlayers());
            StatusText->SetText(FText::FromString(Status));
        }
    }
}

void ULobbyWidget::UpdatePlayerCountText()
{
    ALobbyGameMode* LobbyGameMode = GetLobbyGameMode();
    if (!LobbyGameMode || !PlayerCountText)
    {
        return;
    }
    
    FString Text = FString::Printf(TEXT("Players: %d/%d"),
        LobbyGameMode->GetCurrentPlayerCount(),
        LobbyGameMode->GetMaxPlayers());
    PlayerCountText->SetText(FText::FromString(Text));
}

void ULobbyWidget::UpdateCountdownText()
{
    ALobbyGameMode* LobbyGameMode = GetLobbyGameMode();
    if (!LobbyGameMode || !CountdownText)
    {
        return;
    }
    
    if (LobbyGameMode->IsCountdownActive())
    {
        FString Text = FString::Printf(TEXT("Starting in: %d"), LobbyGameMode->GetCountdownTime());
        CountdownText->SetText(FText::FromString(Text));
        CountdownText->SetVisibility(ESlateVisibility::Visible);
    }
    else
    {
        CountdownText->SetVisibility(ESlateVisibility::Hidden);
    }
}

void ULobbyWidget::UpdateHostControls()
{
    ALobbyPlayerController* LobbyPC = GetLobbyPlayerController();
    if (!LobbyPC || !HostControlsContainer)
    {
        return;
    }
    
    // Only show host controls to the host
    bool bIsHost = LobbyPC->IsHost();
    HostControlsContainer->SetVisibility(bIsHost ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}
