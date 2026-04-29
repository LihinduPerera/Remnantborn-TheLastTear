#include "MainMenuWidget.h"
#include "Remnantborn/Remnantborn/OnlineService/MyOnlineGameInstance.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Components/VerticalBox.h"
#include "Components/Image.h"
#include "Auth/LoginWidget.h"
#include "Auth/UserProfileWidget.h"
#include "SessionInfo/SessionInfoObject.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TimerManager.h"
#include "Remnantborn/Remnantborn/CharacterSelection/CharacterSelectionSubsystem.h"
#include "HttpModule.h"
#include "IImageWrapperModule.h"
#include "IImageWrapper.h"

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
    
    // avatar click opens profile, if a button wrapper exists
    if (ProfileAvatarButton)
    {
        ProfileAvatarButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnAvatarClicked);
    }
    
    // hide avatar by default; it will be shown when logged in
    if (ProfileAvatarImage)
    {
        ProfileAvatarImage->SetVisibility(ESlateVisibility::Collapsed);
    }

    // Bind ListView events
    if (SessionListView)
    {
        SessionListView->OnItemClicked().AddUObject(this, &UMainMenuWidget::OnSessionSelected);
        SessionListView->OnItemDoubleClicked().AddUObject(this, &UMainMenuWidget::OnSessionDoubleClicked);
        SessionListView->ClearListItems();
    }

    // Get GameInstance and bind events
    UMyOnlineGameInstance* GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance());
    if (GameInstance)
    {
        // Multiplayer events
        GameInstance->OnSessionSearchCompleted.AddDynamic(this, &UMainMenuWidget::HandleSessionSearchCompleted);
        GameInstance->OnCreateSessionSuccess.AddDynamic(this, &UMainMenuWidget::HandleCreateSessionSuccess);
        GameInstance->OnCreateSessionFailed.AddDynamic(this, &UMainMenuWidget::HandleCreateSessionFailed);
        GameInstance->OnJoinSessionFailed.AddDynamic(this, &UMainMenuWidget::HandleJoinSessionFailed);
        
        // Updated auth events
        GameInstance->OnAuthStateChanged.AddDynamic(this, &UMainMenuWidget::HandleAuthStateChanged);
        GameInstance->OnProfileUpdated.AddDynamic(this, &UMainMenuWidget::HandleProfileUpdated);
        
        // Check current auth state
        bIsLoggedIn = GameInstance->IsLoggedIn();
        if (bIsLoggedIn)
        {
            CurrentUserProfile = GameInstance->GetCurrentUserProfile();
            UpdateUserInfo();
            SetStatusText("Welcome back!");
        }
        else
        {
            UpdateUserInfo();
            SetStatusText("Ready");
        }
    }

    SelectedSessionIndex = -1;
    SessionListItems.Empty();

    if (ErrorText)
    {
        ErrorText->SetVisibility(ESlateVisibility::Hidden);
    }

    // Initialize lobby settings
    if (MaxPlayersComboBox)
    {
        MaxPlayersComboBox->AddOption(TEXT("2 Players"));
        MaxPlayersComboBox->AddOption(TEXT("4 Players"));
        MaxPlayersComboBox->SetSelectedOption(TEXT("2 Players"));
        MaxPlayersComboBox->OnSelectionChanged.AddDynamic(this, &UMainMenuWidget::OnMaxPlayersSelectionChanged);
        SelectedMaxPlayers = 2;
    }
    
    if (UseLobbyCheckBox)
    {
        UseLobbyCheckBox->SetIsChecked(bUseLobby);
        UseLobbyCheckBox->OnCheckStateChanged.AddDynamic(this, &UMainMenuWidget::OnUseLobbyChanged);
    }

    // Set default server name
    if (ServerNameTextBox)
    {
        ServerNameTextBox->SetText(FText::FromString("MyLANServer"));
    }
    
    // Initialize character subsystem
    UCharacterSelectionSubsystem* CharacterSubsystem = GetGameInstance()->GetSubsystem<UCharacterSelectionSubsystem>();
    if (CharacterSubsystem)
    {
        CharacterSubsystem->LoadAvailableCharacters();
    }
}

void UMainMenuWidget::NativeDestruct()
{
    // Clean up widgets
    if (LoginWidget && LoginWidget->IsInViewport())
    {
        LoginWidget->RemoveFromParent();
        LoginWidget = nullptr;
    }
    
    if (ProfileWidget && ProfileWidget->IsInViewport())
    {
        ProfileWidget->RemoveFromParent();
        ProfileWidget = nullptr;
    }
    
    Super::NativeDestruct();
}

void UMainMenuWidget::OnHostButtonClicked()
{
    if (!bIsLoggedIn)
    {
        ShowError("Please login to host a game");
        return;
    }
    
    FString SessionName = "MyLANServer";
    if (ServerNameTextBox && !ServerNameTextBox->GetText().IsEmpty())
    {
        SessionName = ServerNameTextBox->GetText().ToString();
    }
    
    // Include username in session name
    SessionName = FString::Printf(TEXT("%s's Game"), *CurrentUserProfile.Username);

    UMyOnlineGameInstance* GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance());
    if (GameInstance)
    {
        // Prepare music for level transition - keeps music state for seamless transition
        GameInstance->PrepareForLevelTravel();
        
        SetStatusText("Creating session...");
        ClearError();
        
        if (bUseLobby)
        {
            // Use lobby system
            FString LobbyMapPath = "/Game/Remnantborn/Levels/Lobby";
            GameInstance->CreateSessionWithLobby(SessionName, SelectedMaxPlayers, LobbyMapPath);
        }
        else
        {
            // Direct to game (old behavior)
            GameInstance->CreateSession(SessionName, SelectedMaxPlayers);
        }
    }
}

void UMainMenuWidget::OnFindSessionsButtonClicked()
{
    UMyOnlineGameInstance* GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance());
    if (GameInstance)
    {
        SetStatusText("Searching for LAN games...");
        
        // Clear previous results
        for (USessionInfoObject* Item : SessionListItems)
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
        
        ClearError();
        SelectedSessionIndex = -1;
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
            GameInstance->PrepareForLevelTravel();
            SetStatusText("Joining session...");
            ClearError();
            GameInstance->JoinSessionByIndex(SelectedSessionIndex);
        }
    }
    else
    {
        ShowError("Please select a session first");
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
                SetStatusText("Connecting...");
                ClearError();
                GameInstance->JoinByIP(IPAddress, 7777);
            }
        }
        else
        {
            ShowError("Please enter an IP address");
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
        if (LoginWidget && LoginWidget->IsInViewport())
        {
            LoginWidget->RemoveFromParent();
            LoginWidget = nullptr;
        }
        
        // Create new login widget
        LoginWidget = CreateWidget<ULoginWidget>(GetWorld(), LoginWidgetClass);
        if (LoginWidget)
        {
            LoginWidget->AddToViewport(100); // Higher Z-order
        }
    }
}

void UMainMenuWidget::OnAvatarClicked()
{
    // same behavior as old profile button
    if (ProfileWidgetClass && bIsLoggedIn)
    {
        if (ProfileWidget && ProfileWidget->IsInViewport())
        {
            ProfileWidget->RemoveFromParent();
            ProfileWidget = nullptr;
        }
        
        ProfileWidget = CreateWidget<UUserProfileWidget>(GetWorld(), ProfileWidgetClass);
        if (ProfileWidget)
        {
            ProfileWidget->AddToViewport(100);
            ProfileWidget->UpdateProfile(
                CurrentUserProfile.Username,
                CurrentUserProfile.Level,
                CurrentUserProfile.RemnantCount,
                CurrentUserProfile.AvatarUrl,
                CurrentUserProfile.Email
            );
        }
    }
}

void UMainMenuWidget::OnSessionSelected(UObject* Item)
{
    if (USessionInfoObject* SessionInfo = Cast<USessionInfoObject>(Item))
    {
        SelectedSessionIndex = SessionInfo->SessionIndex;
        SetStatusText(FString::Printf(TEXT("Selected: %s"), *SessionInfo->SessionName));
        ClearError();
    }
}

void UMainMenuWidget::OnSessionDoubleClicked(UObject* Item)
{
    OnSessionSelected(Item);
    OnJoinButtonClicked();
}

void UMainMenuWidget::HandleSessionSearchCompleted(bool bSuccess)
{
    if (bSuccess)
    {
        SetStatusText("Search completed");
        UpdateSessionList();
    }
    else
    {
        SetStatusText("Search failed");
        ShowError("No games found on LAN");
    }
}

void UMainMenuWidget::HandleCreateSessionSuccess()
{
    SetStatusText("Session created successfully!");
}

void UMainMenuWidget::HandleCreateSessionFailed(const FString& ErrorMessage)
{
    SetStatusText("Failed to create session");
    ShowError(ErrorMessage);
}

void UMainMenuWidget::HandleJoinSessionFailed(const FString& ErrorMessage)
{
    SetStatusText("Failed to join session");
    ShowError(ErrorMessage);
}

void UMainMenuWidget::HandleAuthStateChanged(bool bLoginState)
{
    bIsLoggedIn = bLoginState;
    
    if (bLoginState)
    {
        UMyOnlineGameInstance* GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance());
        if (GameInstance)
        {
            CurrentUserProfile = GameInstance->GetCurrentUserProfile();
        }
        
        UpdateUserInfo();
        SetStatusText("Login successful!");
        
        // Hide login widget if it exists
        if (LoginWidget && LoginWidget->IsInViewport())
        {
            LoginWidget->RemoveFromParent();
            LoginWidget = nullptr;
        }
    }
    else
    {
        CurrentUserProfile = FUserProfile();
        UpdateUserInfo();
        SetStatusText("Logged out");
    }
}

void UMainMenuWidget::HandleProfileUpdated(const FUserProfile& UserProfile)
{
    UE_LOG(LogTemp, Log, TEXT("MainMenuWidget: Profile updated - User: %s, Level: %d"), 
        *UserProfile.Username, UserProfile.Level);
    
    CurrentUserProfile = UserProfile;
    
    // always refresh display and update login flag based on validity
    bIsLoggedIn = UserProfile.bIsValid;
    UpdateUserInfo();
    
    // if we have an avatar URL, trigger download
    if (!CurrentUserProfile.AvatarUrl.IsEmpty())
    {
        LoadAvatarFromUrl(CurrentUserProfile.AvatarUrl);
    }
    
    SetStatusText("Profile updated");
}

void UMainMenuWidget::UpdateSessionList()
{
    UMyOnlineGameInstance* GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance());
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
        const FSessionInfo& SessionInfo = GameInstance->SessionSearchResults[i];

        // Create new session info object
        USessionInfoObject* ListItem = NewObject<USessionInfoObject>(this);
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
        SetStatusText(FString::Printf(TEXT("Found %d sessions"), SessionListItems.Num()));
    }
    else
    {
        SetStatusText("No sessions found");
    }
}

void UMainMenuWidget::ClearError()
{
    if (ErrorText)
    {
        ErrorText->SetVisibility(ESlateVisibility::Hidden);
    }
}

void UMainMenuWidget::ShowError(const FString& Message)
{
    if (ErrorText)
    {
        ErrorText->SetText(FText::FromString(Message));
        ErrorText->SetVisibility(ESlateVisibility::Visible);
        GetWorld()->GetTimerManager().SetTimer(ErrorClearTimer, this, &UMainMenuWidget::ClearError, 5.0f, false);
    }
}

void UMainMenuWidget::SetStatusText(const FString& Message)
{
    if (StatusText)
    {
        StatusText->SetText(FText::FromString(Message));
    }
}

void UMainMenuWidget::UpdateUserInfo()
{
    if (bIsLoggedIn && !CurrentUserProfile.Username.IsEmpty())
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
            WelcomeText->SetText(FText::FromString(FString::Printf(TEXT("%s"), *CurrentUserProfile.Username)));
        }
        
        if (UserLevelText)
        {
            UserLevelText->SetText(FText::FromString(FString::Printf(TEXT("Lv: %d"), CurrentUserProfile.Level)));
        }
        
        if (UserRemnantText)
        {
            UserRemnantText->SetText(FText::FromString(FString::Printf(TEXT(" %d"), CurrentUserProfile.RemnantCount)));
        }
        
        // refresh avatar when returning to the menu or when profile data changes
        if (ProfileAvatarImage)
        {
            const FString& AvatarUrl = CurrentUserProfile.AvatarUrl;
            if (!AvatarUrl.IsEmpty() && AvatarUrl != CurrentAvatarUrl)
            {
                LoadAvatarFromUrl(AvatarUrl);
            }
            else if (!CurrentAvatarUrl.IsEmpty())
            {
                ProfileAvatarImage->SetVisibility(ESlateVisibility::Visible);
            }
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
        
        // hide avatar when logged out
        if (ProfileAvatarImage)
        {
            ProfileAvatarImage->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}

// -------------------------------------------------
// Avatar download helpers
// -------------------------------------------------

void UMainMenuWidget::LoadAvatarFromUrl(const FString& Url)
{
    if (Url.IsEmpty() || !ProfileAvatarImage)
    {
        return;
    }

    CurrentAvatarUrl = Url;

    FHttpModule* Http = &FHttpModule::Get();
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb(TEXT("GET"));
    Request->OnProcessRequestComplete().BindUObject(this, &UMainMenuWidget::OnAvatarDownloaded);
    Request->ProcessRequest();
}

void UMainMenuWidget::OnAvatarDownloaded(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
{
    if (!bSuccess || !Response.IsValid() || Response->GetResponseCode() != 200)
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to download avatar image"));
        return;
    }

    TArray<uint8> ImageData = Response->GetContent();
    if (ImageData.Num() == 0)
    {
        return;
    }

    IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
    EImageFormat Format = EImageFormat::JPEG;
    if (CurrentAvatarUrl.EndsWith(TEXT(".png")) || CurrentAvatarUrl.EndsWith(TEXT(".PNG")))
    {
        Format = EImageFormat::PNG;
    }

    TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(Format);
    if (ImageWrapper.IsValid() && ImageWrapper->SetCompressed(ImageData.GetData(), ImageData.Num()))
    {
        TArray<uint8> RawData;
        if (ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, RawData) && RawData.Num() > 0)
        {
            int32 Width = ImageWrapper->GetWidth();
            int32 Height = ImageWrapper->GetHeight();
            UTexture2D* Texture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
            if (Texture && Texture->GetPlatformData() && Texture->GetPlatformData()->Mips.Num() > 0)
            {
                void* TextureData = Texture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
                FMemory::Memcpy(TextureData, RawData.GetData(), RawData.Num());
                Texture->GetPlatformData()->Mips[0].BulkData.Unlock();
                Texture->UpdateResource();
                ProfileAvatarImage->SetBrushFromTexture(Texture, true);
                ProfileAvatarImage->SetVisibility(ESlateVisibility::Visible);
            }
        }
    }
}


void UMainMenuWidget::OnMaxPlayersSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    if (SelectedItem == "2 Players")
    {
        SelectedMaxPlayers = 2;
    }
    else if (SelectedItem == "4 Players")
    {
        SelectedMaxPlayers = 4;
    }
}

void UMainMenuWidget::OnUseLobbyChanged(bool bIsChecked)
{
    bUseLobby = bIsChecked;
}