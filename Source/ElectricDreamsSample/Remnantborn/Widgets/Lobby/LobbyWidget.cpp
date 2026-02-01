#include "LobbyWidget.h"
#include "ElectricDreamsSample/Remnantborn/GameModes/LobbyGameState.h"
#include "ElectricDreamsSample/Remnantborn/OnlineService/LobbyPlayerController/LobbyPlayerController.h"
#include "ElectricDreamsSample/Remnantborn/OnlineService/MyOnlineGameInstance.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "PlayerListEntryWidget.h"
#include "ElectricDreamsSample/Remnantborn/CharacterSelection/CharacterPlayerState.h"
#include "Kismet/GameplayStatics.h"

void ULobbyWidget::NativeConstruct()
{
    Super::NativeConstruct();

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

    if (SelectCharacterButton)
    {
        SelectCharacterButton->OnClicked.AddDynamic(this, &ULobbyWidget::ShowCharacterSelection);
    }

    ALobbyGameState* LobbyGS = GetLobbyGameState();
    if (LobbyGS)
    {
        LobbyGS->OnLobbyStateChanged.AddDynamic(this, &ULobbyWidget::HandleLobbyStateChanged);
    }

    UpdateUI();
}

void ULobbyWidget::NativeDestruct()
{
    ALobbyGameState* LobbyGS = GetLobbyGameState();
    if (LobbyGS)
    {
        LobbyGS->OnLobbyStateChanged.RemoveAll(this);
    }

    for (UPlayerListEntryWidget* Entry : PlayerEntries)
    {
        if (Entry)
        {
            Entry->RemoveFromParent();
        }
    }
    PlayerEntries.Empty();

    Super::NativeDestruct();
}

void ULobbyWidget::UpdatePlayerList()
{
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

    ALobbyGameState* LobbyGS = GetLobbyGameState();
    if (!LobbyGS)
    {
        return;
    }

    for (const FLobbyPlayerInfo& PlayerInfo : LobbyGS->PlayerInfoArray)
    {
        UPlayerListEntryWidget* Entry = CreateWidget<UPlayerListEntryWidget>(this, PlayerListEntryClass);
        if (Entry && PlayerListContainer)
        {
            Entry->SetPlayerInfo(PlayerInfo.PlayerName, PlayerInfo.bIsReady, PlayerInfo.bHasSelectedCharacter);
            PlayerListContainer->AddChildToVerticalBox(Entry);
            PlayerEntries.Add(Entry);
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

void ULobbyWidget::ShowCharacterSelection()
{
    ALobbyPlayerController* LobbyPC = GetLobbyPlayerController();
    if (LobbyPC)
    {
        LobbyPC->Client_ShowCharacterSelection();
    }
}

void ULobbyWidget::HandleLobbyStateChanged()
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

ALobbyGameState* ULobbyWidget::GetLobbyGameState() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    return Cast<ALobbyGameState>(World->GetGameState());
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
    ALobbyGameState* LobbyGS = GetLobbyGameState();
    ALobbyPlayerController* LobbyPC = GetLobbyPlayerController();

    if (!LobbyGS || !LobbyPC)
    {
        return;
    }

    UpdatePlayerList();
    UpdatePlayerCountText();
    UpdateCountdownText();
    UpdateHostControls();
    UpdateReadyButton();

    int32 ReadyPlayers = 0;
    for (const FLobbyPlayerInfo& PlayerInfo : LobbyGS->PlayerInfoArray)
    {
        if (PlayerInfo.bHasSelectedCharacter)
        {
            ReadyPlayers++;
        }
    }

    if (StartButton)
    {
        StartButton->SetIsEnabled(ReadyPlayers == LobbyGS->MaxPlayers && !LobbyGS->bCountdownActive);
    }

    if (CancelButton)
    {
        CancelButton->SetIsEnabled(LobbyGS->bCountdownActive);
    }

    if (SelectCharacterButton)
    {
        ACharacterPlayerState* PS = LobbyPC->GetPlayerState<ACharacterPlayerState>();
        bool bHasCharacter = PS && PS->GetSelectedCharacter() != nullptr;
        SelectCharacterButton->SetIsEnabled(!bHasCharacter && !LobbyGS->bCountdownActive);
    }

    if (StatusText)
    {
        if (LobbyGS->bCountdownActive)
        {
            StatusText->SetText(FText::FromString("Match starting soon..."));
        }
        else if (ReadyPlayers == LobbyGS->MaxPlayers)
        {
            StatusText->SetText(FText::FromString("Ready to start!"));
        }
        else
        {
            FString Status = FString::Printf(TEXT("Waiting for players... (%d/%d)"),
                LobbyGS->CurrentPlayerCount,
                LobbyGS->MaxPlayers);
            StatusText->SetText(FText::FromString(Status));
        }
    }
}

void ULobbyWidget::UpdatePlayerCountText()
{
    ALobbyGameState* LobbyGS = GetLobbyGameState();
    if (!LobbyGS || !PlayerCountText)
    {
        return;
    }

    FString Text = FString::Printf(TEXT("Players: %d/%d"),
        LobbyGS->CurrentPlayerCount,
        LobbyGS->MaxPlayers);
    PlayerCountText->SetText(FText::FromString(Text));
}

void ULobbyWidget::UpdateCountdownText()
{
    ALobbyGameState* LobbyGS = GetLobbyGameState();
    if (!LobbyGS || !CountdownText)
    {
        return;
    }

    if (LobbyGS->bCountdownActive)
    {
        FString Text = FString::Printf(TEXT("Starting in: %d"), LobbyGS->CountdownTime);
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

    bool bIsHost = LobbyPC->IsHost();
    HostControlsContainer->SetVisibility(bIsHost ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void ULobbyWidget::UpdateReadyButton()
{
    ALobbyPlayerController* LobbyPC = GetLobbyPlayerController();
    if (!LobbyPC || !ReadyButton)
    {
        return;
    }

    ACharacterPlayerState* PS = LobbyPC->GetPlayerState<ACharacterPlayerState>();
    bool bHasCharacter = PS && PS->GetSelectedCharacter() != nullptr;

    if (bHasCharacter)
    {
        ReadyButton->SetIsEnabled(true);
    }
    else
    {
        ReadyButton->SetIsEnabled(false);
    }
}
