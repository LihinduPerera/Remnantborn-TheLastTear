#include "MainMenuWidget.h"
#include "ElectricDreamsSample/Remnantborn/OnlineService/MyOnlineGameInstance.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

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
    
    if (SessionListView)
    {
        SessionListView->OnItemClicked().AddUObject(this, &UMainMenuWidget::OnSessionSelected);
    }
    
    // Get GameInstance and bind events
    UMyOnlineGameInstance* GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance());
    if (GameInstance)
    {
        GameInstance->OnSessionSearchCompleted.AddDynamic(this, &UMainMenuWidget::HandleSessionSearchCompleted);
        GameInstance->OnCreateSessionSuccess.AddDynamic(this, &UMainMenuWidget::HandleCreateSessionSuccess);
        GameInstance->OnCreateSessionFailed.AddDynamic(this, &UMainMenuWidget::HandleCreateSessionFailed);
        GameInstance->OnJoinSessionFailed.AddDynamic(this, &UMainMenuWidget::HandleJoinSessionFailed);
    }
    
    SelectedSessionIndex = -1;
    
    if (ErrorText)
    {
        ErrorText->SetVisibility(ESlateVisibility::Hidden);
    }
    
    if (StatusText)
    {
        StatusText->SetText(FText::FromString("Ready"));
    }
}

void UMainMenuWidget::OnHostButtonClicked()
{
    FString SessionName = "MyLANServer";
    if (ServerNameTextBox && !ServerNameTextBox->GetText().IsEmpty())
    {
        SessionName = ServerNameTextBox->GetText().ToString();
    }
    
    UMyOnlineGameInstance* GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance());
    if (GameInstance)
    {
        if (StatusText)
        {
            StatusText->SetText(FText::FromString("Creating session..."));
        }
        GameInstance->CreateSession(SessionName, 4);
    }
}

void UMainMenuWidget::OnFindSessionsButtonClicked()
{
    UMyOnlineGameInstance* GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance());
    if (GameInstance)
    {
        if (StatusText)
        {
            StatusText->SetText(FText::FromString("Searching for LAN games..."));
        }
        
        // Clear previous results
        for (USessionInfoObject* Item : SessionListItems)
        {
            Item->ConditionalBeginDestroy();
        }
        SessionListItems.Empty();
        
        if (SessionListView)
        {
            SessionListView->ClearListItems();
        }
        
        GameInstance->FindSessions();
    }
}

void UMainMenuWidget::OnJoinButtonClicked()
{
    if (SelectedSessionIndex >= 0)
    {
        UMyOnlineGameInstance* GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance());
        if (GameInstance)
        {
            if (StatusText)
            {
                StatusText->SetText(FText::FromString("Joining session..."));
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
            UMyOnlineGameInstance* GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance());
            if (GameInstance)
            {
                if (StatusText)
                {
                    StatusText->SetText(FText::FromString("Connecting..."));
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
            }
        }
    }
}

void UMainMenuWidget::OnQuitButtonClicked()
{
    UKismetSystemLibrary::QuitGame(GetWorld(), GetWorld()->GetFirstPlayerController(), EQuitPreference::Quit, false);
}

void UMainMenuWidget::OnSessionSelected(UObject* Item)
{
    if (USessionInfoObject* SessionInfo = Cast<USessionInfoObject>(Item))
    {
        SelectedSessionIndex = SessionInfo->SessionIndex;
        if (StatusText)
        {
            StatusText->SetText(FText::FromString(FString::Printf(TEXT("Selected: %s"), *SessionInfo->SessionName)));
        }
    }
}

void UMainMenuWidget::HandleSessionSearchCompleted(bool bSuccess)
{
    if (StatusText)
    {
        StatusText->SetText(FText::FromString("Search completed"));
    }
    
    if (bSuccess)
    {
        UMyOnlineGameInstance* GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance());
        if (GameInstance && SessionListView)
        {
            // Clear old items
            for (USessionInfoObject* Item : SessionListItems)
            {
                Item->ConditionalBeginDestroy();
            }
            SessionListItems.Empty();
            SessionListView->ClearListItems();
            
            // Get search results from GameInstance
            for (int32 i = 0; i < GameInstance->SessionSearchResults.Num(); i++)
            {
                const FSessionInfo& SessionInfo = GameInstance->SessionSearchResults[i];
                
                // Create new session info object
                USessionInfoObject* ListItem = NewObject<USessionInfoObject>();
                if (ListItem)
                {
                    ListItem->SessionName = SessionInfo.SessionName;
                    ListItem->PlayerCount = FString::Printf(TEXT("%d/%d"), 
                        SessionInfo.MaxPlayers - SessionInfo.CurrentPlayers,
                        SessionInfo.MaxPlayers);
                    ListItem->Ping = FString::Printf(TEXT("%d ms"), SessionInfo.Ping);
                    ListItem->SessionIndex = i;
                    
                    SessionListItems.Add(ListItem);
                    SessionListView->AddItem(ListItem);
                }
            }
        }
    }
    else
    {
        if (ErrorText)
        {
            ErrorText->SetText(FText::FromString("No games found on LAN"));
            ErrorText->SetVisibility(ESlateVisibility::Visible);
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

void UMainMenuWidget::HandleCreateSessionFailed(const FString& ErrorMessage)
{
    if (StatusText)
    {
        StatusText->SetText(FText::FromString("Failed to create session"));
    }
    
    if (ErrorText)
    {
        ErrorText->SetText(FText::FromString(ErrorMessage));
        ErrorText->SetVisibility(ESlateVisibility::Visible);
    }
}

void UMainMenuWidget::HandleJoinSessionFailed(const FString& ErrorMessage)
{
    if (StatusText)
    {
        StatusText->SetText(FText::FromString("Failed to join session"));
    }
    
    if (ErrorText)
    {
        ErrorText->SetText(FText::FromString(ErrorMessage));
        ErrorText->SetVisibility(ESlateVisibility::Visible);
    }
}