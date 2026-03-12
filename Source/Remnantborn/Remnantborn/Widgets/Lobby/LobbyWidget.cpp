#include "LobbyWidget.h"
#include "Remnantborn/Remnantborn/GameModes/LobbyGameState.h"
#include "Remnantborn/Remnantborn/MapSelection/MapDataAsset.h"
#include "Remnantborn/Remnantborn/MapSelection/MapSelectionSubsystem.h"
#include "Remnantborn/Remnantborn/OnlineService/LobbyPlayerController/LobbyPlayerController.h"
#include "Remnantborn/Remnantborn/OnlineService/MyOnlineGameInstance.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "MapSelectorEntryWidget.h"
#include "PlayerListEntryWidget.h"
#include "Remnantborn/Remnantborn/CharacterSelection/CharacterPlayerState.h"
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

    for (UMapSelectorEntryWidget* Entry : MapEntries)
    {
        if (Entry)
        {
            Entry->RemoveFromParent();
        }
    }
    MapEntries.Empty();

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



void ULobbyWidget::HandleLobbyStateChanged()
{
    UpdateUI();
}

void ULobbyWidget::HandleMapEntrySelected(FName MapID)
{
    if (ALobbyPlayerController* LobbyPC = GetLobbyPlayerController())
    {
        LobbyPC->SetSelectedMap(MapID);
    }
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
    UpdateMapSelection();
    UpdateReadyButton();

    int32 ReadyPlayers = 0;
    for (const FLobbyPlayerInfo& PlayerInfo : LobbyGS->PlayerInfoArray)
    {
        if (PlayerInfo.bIsReady && PlayerInfo.bHasSelectedCharacter)
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



    if (StatusText)
    {
        if (LobbyGS->bCountdownActive)
        {
            StatusText->SetText(FText::FromString("Match starting soon..."));
        }
        else if (ReadyPlayers == LobbyGS->MaxPlayers)
        {
            StatusText->SetText(FText::FromString("All players ready!"));
        }
        else
        {
            FString Status = FString::Printf(TEXT("Waiting for ready players... (%d/%d)"),
                ReadyPlayers,
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

void ULobbyWidget::UpdateMapSelection()
{
    ALobbyGameState* LobbyGS = GetLobbyGameState();
    ALobbyPlayerController* LobbyPC = GetLobbyPlayerController();
    UMapSelectionSubsystem* MapSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMapSelectionSubsystem>() : nullptr;

    if (!LobbyGS || !LobbyPC)
    {
        return;
    }

    const bool bIsHost = LobbyPC->IsHost();

    if (MapSelectorContainer)
    {
        MapSelectorContainer->SetVisibility(bIsHost ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    UMapDataAsset* SelectedMap = MapSubsystem ? MapSubsystem->GetMapByID(LobbyGS->SelectedMapID) : nullptr;
    if (SelectedMapNameText)
    {
        SelectedMapNameText->SetText(SelectedMap ? SelectedMap->MapName : FText::FromString(TEXT("Map Not Selected")));
    }

    if (!bIsHost)
    {
        RebuildMapList(TArray<UMapDataAsset*>(), LobbyGS->SelectedMapID);
        return;
    }

    const TArray<UMapDataAsset*> AvailableMaps = MapSubsystem ? MapSubsystem->GetAvailableMaps() : TArray<UMapDataAsset*>();
    RebuildMapList(AvailableMaps, LobbyGS->SelectedMapID);
}

void ULobbyWidget::RebuildMapList(const TArray<UMapDataAsset*>& AvailableMaps, const FName& SelectedMapID)
{
    for (UMapSelectorEntryWidget* Entry : MapEntries)
    {
        if (Entry)
        {
            Entry->RemoveFromParent();
        }
    }
    MapEntries.Empty();

    if (!MapListContainer || !MapEntryWidgetClass)
    {
        return;
    }

    for (UMapDataAsset* MapData : AvailableMaps)
    {
        UMapSelectorEntryWidget* Entry = CreateWidget<UMapSelectorEntryWidget>(this, MapEntryWidgetClass);
        if (!Entry || !MapData)
        {
            continue;
        }

        Entry->SetMapInfo(MapData, MapData->MapID == SelectedMapID);
        Entry->OnMapSelected.AddDynamic(this, &ULobbyWidget::HandleMapEntrySelected);
        MapListContainer->AddChildToVerticalBox(Entry);
        MapEntries.Add(Entry);
    }
}

void ULobbyWidget::UpdateReadyButton()
{
    ALobbyPlayerController* LobbyPC = GetLobbyPlayerController();
    if (!LobbyPC || !ReadyButton)
    {
        return;
    }

    // Check the replicated variable instead of PlayerState
    bool bHasCharacter = LobbyPC->HasSelectedCharacter();

    UE_LOG(LogTemp, Warning, TEXT("UpdateReadyButton: bHasCharacter = %s, bIsReady = %s"), 
        bHasCharacter ? TEXT("true") : TEXT("false"), 
        LobbyPC->IsReady() ? TEXT("true") : TEXT("false"));

    if (bHasCharacter)
    {
        ReadyButton->SetIsEnabled(true);
        UE_LOG(LogTemp, Warning, TEXT("UpdateReadyButton: Ready button enabled"));
    }
    else
    {
        ReadyButton->SetIsEnabled(false);
        UE_LOG(LogTemp, Warning, TEXT("UpdateReadyButton: Ready button disabled"));
    }
}
