#include "LobbyPlayerController.h"
#include "ElectricDreamsSample/Remnantborn/Widgets/Lobby/LobbyWidget.h"
#include "ElectricDreamsSample/Remnantborn/GameModes/LobbyGameMode.h"
#include "ElectricDreamsSample/Remnantborn/GameModes/LobbyGameState.h"
#include "ElectricDreamsSample/Remnantborn/Widgets/CharacterSelection/CharacterSelectionWidget.h"
#include "ElectricDreamsSample/Remnantborn/CharacterSelection/CharacterSelectionSubsystem.h"
#include "ElectricDreamsSample/Remnantborn/CharacterSelection/CharacterPlayerState.h"
#include "ElectricDreamsSample/Remnantborn/OnlineService/MyOnlineGameInstance.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"

void ALobbyPlayerController::BeginPlay()
{
    Super::BeginPlay();
    SetupInputMode();

    FTimerHandle WidgetTimer;
    FTimerDelegate WidgetDelegate;
    WidgetDelegate.BindUFunction(this, "CreateLobbyWidget");
    GetWorld()->GetTimerManager().SetTimer(WidgetTimer, WidgetDelegate, 0.5f, false);
}

void ALobbyPlayerController::CreateLobbyWidget()
{
    if (!LobbyWidgetClass || LobbyWidget)
    {
        return;
    }

    LobbyWidget = CreateWidget<ULobbyWidget>(this, LobbyWidgetClass);
    if (LobbyWidget)
    {
        LobbyWidget->AddToViewport();
    }
}

void ALobbyPlayerController::Client_ShowCharacterSelection_Implementation()
{
    ShowCharacterSelection();
}

void ALobbyPlayerController::ShowCharacterSelection()
{
    if (!CharacterSelectionWidgetClass || CharacterSelectionWidget)
    {
        return;
    }

    CharacterSelectionWidget = CreateWidget<UCharacterSelectionWidget>(this, CharacterSelectionWidgetClass);
    if (CharacterSelectionWidget)
    {
        CharacterSelectionWidget->AddToViewport();

        if (LobbyWidget)
        {
            LobbyWidget->SetVisibility(ESlateVisibility::Hidden);
        }

        CharacterSelectionWidget->OnCharacterConfirmed.AddDynamic(this, &ALobbyPlayerController::HandleCharacterSelected);
        CharacterSelectionWidget->OnCharacterCancelled.AddDynamic(this, &ALobbyPlayerController::HandleCharacterCancelled);
    }
}

void ALobbyPlayerController::HideCharacterSelection()
{
    if (CharacterSelectionWidget)
    {
        CharacterSelectionWidget->RemoveFromParent();
        CharacterSelectionWidget = nullptr;
    }

    if (LobbyWidget)
    {
        LobbyWidget->SetVisibility(ESlateVisibility::Visible);
        LobbyWidget->UpdateUI();
    }
}

void ALobbyPlayerController::Client_CleanupLobbyWidgets_Implementation()
{
    if (CharacterSelectionWidget)
    {
        CharacterSelectionWidget->RemoveFromParent();
        CharacterSelectionWidget = nullptr;
    }

    if (LobbyWidget)
    {
        LobbyWidget->RemoveFromParent();
        LobbyWidget = nullptr;
    }

    FInputModeGameOnly InputMode;
    SetInputMode(InputMode);
    SetShowMouseCursor(false);
    bEnableClickEvents = false;
    bEnableMouseOverEvents = false;
}

void ALobbyPlayerController::HandleCharacterSelected(UCharacterDataAsset* CharacterData)
{
    if (CharacterData)
    {
        Server_ConfirmCharacterSelection(CharacterData->CharacterID);
    }
    HideCharacterSelection();
}

void ALobbyPlayerController::HandleCharacterCancelled(UCharacterDataAsset* CharacterData)
{
    CharacterSelectionWidget = nullptr;
    HideCharacterSelection();
}

void ALobbyPlayerController::SetupInputMode()
{
    FInputModeUIOnly InputMode;
    SetInputMode(InputMode);
    SetShowMouseCursor(true);
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;
}

bool ALobbyPlayerController::IsHost() const
{
    if (GetWorld())
    {
        AGameModeBase* GameMode = GetWorld()->GetAuthGameMode();
        if (GameMode)
        {
            return GetNetMode() == NM_ListenServer || GetNetMode() == NM_DedicatedServer;
        }
    }
    return false;
}

void ALobbyPlayerController::SetPlayerReady(bool bReady)
{
    if (!HasAuthority())
    {
        Server_SetPlayerReady(bReady);
    }
    else
    {
        Server_SetPlayerReady_Implementation(bReady);
    }
}

bool ALobbyPlayerController::Server_SetPlayerReady_Validate(bool bReady)
{
    return true;
}

void ALobbyPlayerController::Server_SetPlayerReady_Implementation(bool bReady)
{
    if (bIsReady != bReady)
    {
        bIsReady = bReady;
        OnRep_IsReady();

        if (ALobbyGameMode* LobbyGM = Cast<ALobbyGameMode>(GetWorld()->GetAuthGameMode()))
        {
            LobbyGM->OnPlayerReadyChanged(this, bReady);
        }

        if (ALobbyGameState* LobbyGS = Cast<ALobbyGameState>(GetWorld()->GetGameState()))
        {
            LobbyGS->UpdatePlayerInfo(this, bIsReady, bHasSelectedCharacter);
        }
    }
}

void ALobbyPlayerController::OnRep_IsReady()
{
    if (LobbyWidget)
    {
        LobbyWidget->UpdateUI();
    }
}

void ALobbyPlayerController::OnRep_HasSelectedCharacter()
{
    if (LobbyWidget)
    {
        LobbyWidget->UpdateUI();
    }
}

void ALobbyPlayerController::StartMatchCountdown()
{
    if (!IsHost())
    {
        return;
    }

    Server_StartMatchCountdown();
}

bool ALobbyPlayerController::Server_StartMatchCountdown_Validate()
{
    return true;
}

void ALobbyPlayerController::Server_StartMatchCountdown_Implementation()
{
    ALobbyGameMode* LobbyGameMode = Cast<ALobbyGameMode>(GetWorld()->GetAuthGameMode());
    if (LobbyGameMode)
    {
        LobbyGameMode->StartMatchCountdown();
    }
}

void ALobbyPlayerController::CancelMatchCountdown()
{
    if (!IsHost())
    {
        return;
    }

    Server_CancelMatchCountdown();
}

bool ALobbyPlayerController::Server_CancelMatchCountdown_Validate()
{
    return true;
}

void ALobbyPlayerController::Server_CancelMatchCountdown_Implementation()
{
    ALobbyGameMode* LobbyGameMode = Cast<ALobbyGameMode>(GetWorld()->GetAuthGameMode());
    if (LobbyGameMode)
    {
        LobbyGameMode->CancelMatchCountdown();
    }
}

void ALobbyPlayerController::SetMaxPlayers(int32 MaxPlayers)
{
    if (!IsHost())
    {
        return;
    }

    Server_SetMaxPlayers(MaxPlayers);
}

bool ALobbyPlayerController::Server_SetMaxPlayers_Validate(int32 MaxPlayers)
{
    return MaxPlayers == 2 || MaxPlayers == 4;
}

void ALobbyPlayerController::Server_SetMaxPlayers_Implementation(int32 MaxPlayers)
{
    ALobbyGameMode* LobbyGameMode = Cast<ALobbyGameMode>(GetWorld()->GetAuthGameMode());
    if (LobbyGameMode)
    {
        LobbyGameMode->SetMaxPlayers(MaxPlayers);
    }
}

bool ALobbyPlayerController::Server_ConfirmCharacterSelection_Validate(FName CharacterID)
{
    return !CharacterID.IsNone();
}

void ALobbyPlayerController::Server_ConfirmCharacterSelection_Implementation(FName CharacterID)
{
    if (!CharacterID.IsNone())
    {
        ACharacterPlayerState* PS = GetPlayerState<ACharacterPlayerState>();
        if (PS)
        {
            UCharacterSelectionSubsystem* Subsystem = GetGameInstance()->GetSubsystem<UCharacterSelectionSubsystem>();
            if (Subsystem)
            {
                UCharacterDataAsset* CharacterData = Subsystem->GetCharacterByID(CharacterID);
                if (CharacterData)
                {
                    PS->SetSelectedCharacter(CharacterData);
                    bHasSelectedCharacter = true;
                    OnRep_HasSelectedCharacter();

                    // Store character selection in GameInstance for persistence during travel
                    if (UMyOnlineGameInstance* GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance()))
                    {
                        GameInstance->SetLocalCharacterSelection(CharacterID);
                    }

                    if (ALobbyGameState* LobbyGS = Cast<ALobbyGameState>(GetWorld()->GetGameState()))
                    {
                        LobbyGS->UpdatePlayerInfo(this, bIsReady, bHasSelectedCharacter);
                    }

                    if (ALobbyGameMode* LobbyGM = Cast<ALobbyGameMode>(GetWorld()->GetAuthGameMode()))
                    {
                        LobbyGM->OnPlayerSelectedCharacter(this, CharacterData);
                    }
                }
            }
        }
    }
}

void ALobbyPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ALobbyPlayerController, bIsReady);
    DOREPLIFETIME(ALobbyPlayerController, bHasSelectedCharacter);
}
