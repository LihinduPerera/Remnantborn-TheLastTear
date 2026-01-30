#include "MainMenuWidget.h"
#include "ElectricDreamsSample/Remnantborn/OnlineService/MyOnlineGameInstance.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TimerManager.h"

void UMainMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // Bind button events
    if (HostButton)
    {
        HostButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnHostButtonClicked);
    }

    if (FindSessionsButton)
    {
        FindSessionsButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnFindSessionsButtonClicked);
    }

    if (JoinButton)
    {
        JoinButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnJoinButtonClicked);
    }

    if (DirectJoinButton)
    {
        DirectJoinButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnDirectJoinButtonClicked);
    }

    if (QuitButton)
    {
        QuitButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnQuitButtonClicked);
    }

    // Bind ListView events
    if (SessionListView)
    {
        SessionListView->OnItemClicked().AddUObject(this, &UMainMenuWidget::OnSessionSelected);
        SessionListView->OnItemDoubleClicked().AddUObject(this, &UMainMenuWidget::OnSessionDoubleClicked);
        SessionListView->ClearListItems();
    }

    // Get GameInstance and bind events
    UMyOnlineGameInstance *GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance());
    if (GameInstance)
    {
        GameInstance->OnSessionSearchCompleted.AddDynamic(this, &UMainMenuWidget::HandleSessionSearchCompleted);
        GameInstance->OnCreateSessionSuccess.AddDynamic(this, &UMainMenuWidget::HandleCreateSessionSuccess);
        GameInstance->OnCreateSessionFailed.AddDynamic(this, &UMainMenuWidget::HandleCreateSessionFailed);
        GameInstance->OnJoinSessionFailed.AddDynamic(this, &UMainMenuWidget::HandleJoinSessionFailed);
    }

    SelectedSessionIndex = -1;
    SessionListItems.Empty();

    if (ErrorText)
    {
        ErrorText->SetVisibility(ESlateVisibility::Hidden);
    }

    if (StatusText)
    {
        StatusText->SetText(FText::FromString("Ready"));
    }

    // Set default server name
    if (ServerNameTextBox)
    {
        ServerNameTextBox->SetText(FText::FromString("MyLANServer"));
    }
}

void UMainMenuWidget::OnHostButtonClicked()
{
    FString SessionName = "MyLANServer";
    if (ServerNameTextBox && !ServerNameTextBox->GetText().IsEmpty())
    {
        SessionName = ServerNameTextBox->GetText().ToString();
    }

    UMyOnlineGameInstance *GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance());
    if (GameInstance)
    {
        if (StatusText)
        {
            StatusText->SetText(FText::FromString("Creating session..."));
        }
        // Clear any errors
        if (ErrorText)
        {
            ErrorText->SetVisibility(ESlateVisibility::Hidden);
        }
        GameInstance->CreateSession(SessionName, 4);
    }
}

void UMainMenuWidget::OnFindSessionsButtonClicked()
{
    UMyOnlineGameInstance *GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance());
    if (GameInstance)
    {
        if (StatusText)
        {
            StatusText->SetText(FText::FromString("Searching for LAN games..."));
        }
        // Clear previous results
        for (USessionInfoObject *Item : SessionListItems)
        {
            if (Item)
            {
                Item->ConditionalBeginDestroy();
            }
        }
        SessionListItems.Empty();
        if (SessionListView)
        {
            SessionListView->ClearListItems();
        }
        // Clear any errors
        if (ErrorText)
        {
            ErrorText->SetVisibility(ESlateVisibility::Hidden);
        }
        SelectedSessionIndex = -1;
        GameInstance->FindSessions();
    }
}

void UMainMenuWidget::OnJoinButtonClicked()
{
    if (SelectedSessionIndex >= 0)
    {
        UMyOnlineGameInstance *GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance());
        if (GameInstance)
        {
            if (StatusText)
            {
                StatusText->SetText(FText::FromString("Joining session..."));
            }
            // Clear any errors
            if (ErrorText)
            {
                ErrorText->SetVisibility(ESlateVisibility::Hidden);
            }
            GameInstance->JoinSessionByIndex(SelectedSessionIndex);
        }
    }
    else
    {
        if (ErrorText)
        {
            ErrorText->SetText(FText::FromString("Please select a session first"));
            ErrorText->SetVisibility(ESlateVisibility::Visible);
            // Clear error after 3 seconds
            GetWorld()->GetTimerManager().SetTimer(ErrorClearTimer, this, &UMainMenuWidget::ClearErrorMessage, 3.0f, false);
        }
    }
}

void UMainMenuWidget::OnDirectJoinButtonClicked()
{
    if (DirectIPTextBox)
    {
        FString IPAddress = DirectIPTextBox->GetText().ToString();
        if (!IPAddress.IsEmpty())
        {
            UMyOnlineGameInstance *GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance());
            if (GameInstance)
            {
                if (StatusText)
                {
                    StatusText->SetText(FText::FromString("Connecting..."));
                }
                // Clear any errors
                if (ErrorText)
                {
                    ErrorText->SetVisibility(ESlateVisibility::Hidden);
                }
                GameInstance->JoinByIP(IPAddress, 7777);
            }
        }
        else
        {
            if (ErrorText)
            {
                ErrorText->SetText(FText::FromString("Please enter an IP address"));
                ErrorText->SetVisibility(ESlateVisibility::Visible);
                // Clear error after 3 seconds
                GetWorld()->GetTimerManager().SetTimer(ErrorClearTimer, this, &UMainMenuWidget::ClearErrorMessage, 3.0f, false);
            }
        }
    }
}

void UMainMenuWidget::OnQuitButtonClicked()
{
    UKismetSystemLibrary::QuitGame(GetWorld(), GetWorld()->GetFirstPlayerController(), EQuitPreference::Quit, false);
}

void UMainMenuWidget::OnSessionSelected(UObject *Item)
{
    if (USessionInfoObject *SessionInfo = Cast<USessionInfoObject>(Item))
    {
        SelectedSessionIndex = SessionInfo->SessionIndex;
        if (StatusText)
        {
            StatusText->SetText(FText::FromString(FString::Printf(TEXT("Selected: %s"), *SessionInfo->SessionName)));
        }
        // Clear any errors
        if (ErrorText)
        {
            ErrorText->SetVisibility(ESlateVisibility::Hidden);
        }
    }
}

void UMainMenuWidget::OnSessionDoubleClicked(UObject *Item)
{
    OnSessionSelected(Item);
    OnJoinButtonClicked();
}

void UMainMenuWidget::HandleSessionSearchCompleted(bool bSuccess)
{
    if (StatusText)
    {
        if (bSuccess)
        {
            StatusText->SetText(FText::FromString("Search completed"));
        }
        else
        {
            StatusText->SetText(FText::FromString("Search failed - No sessions found"));
        }
    }

    if (bSuccess)
    {
        UpdateSessionList();
    }
    else
    {
        if (ErrorText)
        {
            ErrorText->SetText(FText::FromString("No games found on LAN"));
            ErrorText->SetVisibility(ESlateVisibility::Visible);
            // Clear error after 3 seconds
            GetWorld()->GetTimerManager().SetTimer(ErrorClearTimer, this, &UMainMenuWidget::ClearErrorMessage, 3.0f, false);
        }
    }
}

void UMainMenuWidget::HandleCreateSessionSuccess()
{
    if (StatusText)
    {
        StatusText->SetText(FText::FromString("Session created successfully!"));
    }
}

void UMainMenuWidget::HandleCreateSessionFailed(const FString &ErrorMessage)
{
    if (StatusText)
    {
        StatusText->SetText(FText::FromString("Failed to create session"));
    }

    if (ErrorText)
    {
        ErrorText->SetText(FText::FromString(ErrorMessage));
        ErrorText->SetVisibility(ESlateVisibility::Visible);
        // Clear error after 3 seconds
        GetWorld()->GetTimerManager().SetTimer(ErrorClearTimer, this, &UMainMenuWidget::ClearErrorMessage, 3.0f, false);
    }
}

void UMainMenuWidget::HandleJoinSessionFailed(const FString &ErrorMessage)
{
    if (StatusText)
    {
        StatusText->SetText(FText::FromString("Failed to join session"));
    }

    if (ErrorText)
    {
        ErrorText->SetText(FText::FromString(ErrorMessage));
        ErrorText->SetVisibility(ESlateVisibility::Visible);
        // Clear error after 3 seconds
        GetWorld()->GetTimerManager().SetTimer(ErrorClearTimer, this, &UMainMenuWidget::ClearErrorMessage, 3.0f, false);
    }
}

void UMainMenuWidget::UpdateSessionList()
{
    UMyOnlineGameInstance *GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance());
    if (!GameInstance || !SessionListView)
    {
        return;
    }

    // Clear old items - no need to destroy manually, ListView will handle it
    SessionListItems.Empty();
    SessionListView->ClearListItems();

    // Get search results from GameInstance
    for (int32 i = 0; i < GameInstance->SessionSearchResults.Num(); i++)
    {
        const FSessionInfo &SessionInfo = GameInstance->SessionSearchResults[i];

        // Create new session info object
        USessionInfoObject *ListItem = NewObject<USessionInfoObject>(this);
        if (ListItem)
        {
            // Ensure we have valid data
            FString SessionName = SessionInfo.SessionName;
            if (SessionName.IsEmpty())
            {
                SessionName = FString::Printf(TEXT("Session_%d"), i + 1);
            }

            // Ensure valid player count
            int32 CurrentPlayers = FMath::Max(0, SessionInfo.CurrentPlayers);
            int32 MaxPlayers = FMath::Max(1, SessionInfo.MaxPlayers);
            
            // Ensure valid ping
            int32 PingValue = FMath::Max(0, SessionInfo.Ping);

            ListItem->Setup(
                SessionName,
                FString::Printf(TEXT("%d/%d"), CurrentPlayers, MaxPlayers),
                FString::Printf(TEXT("%d ms"), PingValue),
                i);

            SessionListItems.Add(ListItem);
            SessionListView->AddItem(ListItem);

            UE_LOG(LogTemp, Log, TEXT("Added session to list: %s, Players: %d/%d, Ping: %d"),
                   *SessionName, CurrentPlayers, MaxPlayers, PingValue);
        }
    }

    // Refresh the ListView to show new items
    SessionListView->RequestRefresh();

    // Update status text with count
    if (SessionListItems.Num() > 0)
    {
        if (StatusText)
        {
            StatusText->SetText(FText::FromString(FString::Printf(TEXT("Found %d sessions"), SessionListItems.Num())));
        }
    }
    else
    {
        if (StatusText)
        {
            StatusText->SetText(FText::FromString("No sessions found"));
        }
    }
}

void UMainMenuWidget::ClearErrorMessage()
{
    if (ErrorText)
    {
        ErrorText->SetVisibility(ESlateVisibility::Hidden);
    }
}