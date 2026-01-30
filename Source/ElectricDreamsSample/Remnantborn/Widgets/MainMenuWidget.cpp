// FILE PATH: D:\projects\UnrealProjects\Remnantborn\Source\ElectricDreamsSample\Remnantborn\Widgets\MainMenuWidget.cpp
#include "MainMenuWidget.h"
#include "ElectricDreamsSample/Remnantborn/OnlineService/MyOnlineGameInstance.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Components/VerticalBox.h"
#include "Auth/LoginWidget.h"
#include "Auth/UserProfileWidget.h"
#include "SessionInfo/SessionInfoObject.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TimerManager.h"

void UMainMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // Bind multiplayer button events
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

    // Bind auth button events
    if (LoginButton)
    {
        LoginButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnLoginButtonClicked);
    }
    
    if (ProfileButton)
    {
        ProfileButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnProfileButtonClicked);
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
        // Multiplayer events
        GameInstance->OnSessionSearchCompleted.AddDynamic(this, &UMainMenuWidget::HandleSessionSearchCompleted);
        GameInstance->OnCreateSessionSuccess.AddDynamic(this, &UMainMenuWidget::HandleCreateSessionSuccess);
        GameInstance->OnCreateSessionFailed.AddDynamic(this, &UMainMenuWidget::HandleCreateSessionFailed);
        GameInstance->OnJoinSessionFailed.AddDynamic(this, &UMainMenuWidget::HandleJoinSessionFailed);
        
        // Authentication events
        GameInstance->OnAuthLoginComplete.AddDynamic(this, &UMainMenuWidget::HandleLoginComplete);
        GameInstance->OnAuthSignupComplete.AddDynamic(this, &UMainMenuWidget::HandleSignupComplete);
        GameInstance->OnProfileUpdated.AddDynamic(this, &UMainMenuWidget::HandleProfileUpdated);
        
        // Check if already logged in
        bIsLoggedIn = GameInstance->IsLoggedIn();
        if (bIsLoggedIn)
        {
            CurrentUserProfile = GameInstance->GetCurrentUserProfile();
            UpdateUserInfo();
        }
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
    
    // Update UI based on login state
    UpdateUserInfo();
}

void UMainMenuWidget::OnHostButtonClicked()
{
    if (!bIsLoggedIn)
    {
        if (ErrorText)
        {
            ErrorText->SetText(FText::FromString("Please login to host a game"));
            ErrorText->SetVisibility(ESlateVisibility::Visible);
            GetWorld()->GetTimerManager().SetTimer(ErrorClearTimer, this, &UMainMenuWidget::ClearErrorMessage, 3.0f, false);
        }
        return;
    }
    
    FString SessionName = "MyLANServer";
    if (ServerNameTextBox && !ServerNameTextBox->GetText().IsEmpty())
    {
        SessionName = ServerNameTextBox->GetText().ToString();
    }
    
    // Include username in session name
    SessionName = FString::Printf(TEXT("%s's Game"), *CurrentUserProfile.Username);

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

void UMainMenuWidget::OnLoginButtonClicked()
{
    if (LoginWidgetClass)
    {
        // Remove existing login widget
        if (LoginWidget)
        {
            LoginWidget->RemoveFromParent();
            LoginWidget = nullptr;
        }
        
        // Create new login widget
        LoginWidget = CreateWidget<ULoginWidget>(GetWorld(), LoginWidgetClass);
        if (LoginWidget)
        {
            LoginWidget->AddToViewport();
            
            // Center the widget
            FVector2D ViewportSize;
            if (GEngine && GEngine->GameViewport)
            {
                GEngine->GameViewport->GetViewportSize(ViewportSize);
                LoginWidget->SetPositionInViewport(FVector2D(
                    (ViewportSize.X - LoginWidget->GetDesiredSize().X) / 2,
                    (ViewportSize.Y - LoginWidget->GetDesiredSize().Y) / 2
                ));
            }
        }
    }
}

void UMainMenuWidget::OnProfileButtonClicked()
{
    if (ProfileWidgetClass && bIsLoggedIn)
    {
        // Remove existing profile widget
        if (ProfileWidget)
        {
            ProfileWidget->RemoveFromParent();
            ProfileWidget = nullptr;
        }
        
        // Create new profile widget
        ProfileWidget = CreateWidget<UUserProfileWidget>(GetWorld(), ProfileWidgetClass);
        if (ProfileWidget)
        {
            ProfileWidget->AddToViewport();
            ProfileWidget->UpdateProfile(
                CurrentUserProfile.Username,
                CurrentUserProfile.Level,
                CurrentUserProfile.RemnantCount,
                CurrentUserProfile.AvatarUrl
            );
            
            // Center the widget
            FVector2D ViewportSize;
            if (GEngine && GEngine->GameViewport)
            {
                GEngine->GameViewport->GetViewportSize(ViewportSize);
                ProfileWidget->SetPositionInViewport(FVector2D(
                    (ViewportSize.X - ProfileWidget->GetDesiredSize().X) / 2,
                    (ViewportSize.Y - ProfileWidget->GetDesiredSize().Y) / 2
                ));
            }
        }
    }
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

void UMainMenuWidget::HandleLoginComplete(const FAuthResponse& AuthResponse)
{
    if (AuthResponse.bSuccess)
    {
        bIsLoggedIn = true;
        CurrentUserProfile = AuthResponse.UserProfile;
        UpdateUserInfo();
        
        if (StatusText)
        {
            StatusText->SetText(FText::FromString("Login successful!"));
        }
    }
    else
    {
        if (ErrorText)
        {
            ErrorText->SetText(FText::FromString(AuthResponse.ErrorMessage));
            ErrorText->SetVisibility(ESlateVisibility::Visible);
            GetWorld()->GetTimerManager().SetTimer(ErrorClearTimer, this, &UMainMenuWidget::ClearErrorMessage, 5.0f, false);
        }
    }
}

void UMainMenuWidget::HandleSignupComplete(const FAuthResponse& AuthResponse)
{
    HandleLoginComplete(AuthResponse); // Same handling as login
}

void UMainMenuWidget::HandleProfileUpdated(const FUserProfile& UserProfile)
{
    CurrentUserProfile = UserProfile;
    UpdateUserInfo();
    
    if (StatusText)
    {
        StatusText->SetText(FText::FromString("Profile updated"));
    }
}

void UMainMenuWidget::UpdateSessionList()
{
    UMyOnlineGameInstance *GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance());
    if (!GameInstance || !SessionListView)
    {
        return;
    }

    // Clear old items
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

void UMainMenuWidget::UpdateUserInfo()
{
    if (bIsLoggedIn)
    {
        // Show user info panel
        if (UserInfoPanel)
        {
            UserInfoPanel->SetVisibility(ESlateVisibility::Visible);
        }
        
        // Hide login panel
        if (LoginPanel)
        {
            LoginPanel->SetVisibility(ESlateVisibility::Collapsed);
        }
        
        // Update user info texts
        if (WelcomeText)
        {
            WelcomeText->SetText(FText::FromString(FString::Printf(TEXT("Welcome, %s!"), *CurrentUserProfile.Username)));
        }
        
        if (UserLevelText)
        {
            UserLevelText->SetText(FText::FromString(FString::Printf(TEXT("Level: %d"), CurrentUserProfile.Level)));
        }
        
        if (UserRemnantText)
        {
            UserRemnantText->SetText(FText::FromString(FString::Printf(TEXT("Remnants: %d"), CurrentUserProfile.RemnantCount)));
        }
    }
    else
    {
        // Show login panel
        if (LoginPanel)
        {
            LoginPanel->SetVisibility(ESlateVisibility::Visible);
        }
        
        // Hide user info panel
        if (UserInfoPanel)
        {
            UserInfoPanel->SetVisibility(ESlateVisibility::Collapsed);
        }
        
        // Clear user info texts
        if (WelcomeText)
        {
            WelcomeText->SetText(FText::FromString(TEXT("Welcome, Guest!")));
        }
        
        if (UserLevelText)
        {
            UserLevelText->SetText(FText::FromString(TEXT("Level: -")));
        }
        
        if (UserRemnantText)
        {
            UserRemnantText->SetText(FText::FromString(TEXT("Remnants: -")));
        }
    }
}