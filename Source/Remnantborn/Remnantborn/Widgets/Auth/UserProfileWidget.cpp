#include "UserProfileWidget.h"
#include "ProfileEditWidget.h" // show/hide embedded editor
#include "Remnantborn/Remnantborn/OnlineService/MyOnlineGameInstance.h"
#include "Remnantborn/Remnantborn/CharacterSelection/CharacterSelectionSubsystem.h"
#include "OwnedCharacterCardWidget.h"
#include "MatchHistoryCardWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/PanelWidget.h"
#include "Engine/Texture2D.h"
#include "UObject/UObjectGlobals.h"
#include "Blueprint/WidgetTree.h"

#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Modules/ModuleManager.h"

void UUserProfileWidget::NativeConstruct()
{
	Super::NativeConstruct();
    
	// Bind button events
	if (LogoutButton)
	{
		LogoutButton->OnClicked.AddDynamic(this, &UUserProfileWidget::OnLogoutClicked);
	}
    
	if (RefreshButton)
	{
		RefreshButton->OnClicked.AddDynamic(this, &UUserProfileWidget::OnRefreshClicked);
	}

	// open profile edit panel if embedded
	if (EditProfileButton)
	{
		EditProfileButton->OnClicked.AddDynamic(this, &UUserProfileWidget::OnEditProfileClicked);
	}

	// if a class was specified in defaults but no instance was bound already, spawn it
	if (!ProfileEditWidget && ProfileEditWidgetClass)
	{
		ProfileEditWidget = CreateWidget<UProfileEditWidget>(this, ProfileEditWidgetClass);
		if (ProfileEditWidget)
		{
			if (UPanelWidget* Root = Cast<UPanelWidget>(GetRootWidget()))
			{
				Root->AddChild(ProfileEditWidget);
			}
		}
	}

	// ensure editor starts hidden
	if (ProfileEditWidget)
	{
		ProfileEditWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (!MatchHistoryCardClass)
	{
		MatchHistoryCardClass = LoadClass<UMatchHistoryCardWidget>(
			nullptr,
			TEXT("/Game/Widgets/Auth/WBP_MatchHistoryCardWidget.WBP_MatchHistoryCardWidget_C")
		);

		if (!MatchHistoryCardClass)
		{
			MatchHistoryCardClass = UMatchHistoryCardWidget::StaticClass();
		}
	}

	if (!MatchHistoryScrollBox && WidgetTree)
	{
		MatchHistoryScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("DynamicMatchHistoryScrollBox"));
		if (MatchHistoryScrollBox)
		{
			if (UPanelWidget* Root = Cast<UPanelWidget>(GetRootWidget()))
			{
				Root->AddChild(MatchHistoryScrollBox);
			}
		}
	}

	if (!MatchHistoryText && WidgetTree)
	{
		MatchHistoryText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DynamicMatchHistoryText"));
		if (MatchHistoryText)
		{
			if (UPanelWidget* Root = Cast<UPanelWidget>(GetRootWidget()))
			{
				Root->AddChild(MatchHistoryText);
				MatchHistoryText->SetText(FText::GetEmpty());
			}
		}
	}

	// Subscribe to profile/auth changes so widget works anywhere (tabs, menus, etc.)
	UMyOnlineGameInstance* GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance());
	if (GameInstance)
	{
		GameInstance->OnProfileUpdated.AddDynamic(this, &UUserProfileWidget::HandleProfileUpdated);
		GameInstance->OnAuthStateChanged.AddDynamic(this, &UUserProfileWidget::HandleAuthStateChanged);

		// initialize display with current profile if already present
		const FUserProfile& Existing = GameInstance->GetCurrentUserProfile();
		if (Existing.bIsValid)
		{
			HandleProfileUpdated(Existing);
		}

		// apply gating based on current login state
		ApplyLoginGating(GameInstance->IsLoggedIn());
	}
}

void UUserProfileWidget::UpdateProfile(const FString& Username, int32 Level, int32 RemnantCount, const FString& AvatarUrl, const FString& Email, const FString& Bio, const FString& CreatedAt)
{
    // propagate data to the embedded editor if present
    if (ProfileEditWidget)
    {
        UMyOnlineGameInstance* GI = Cast<UMyOnlineGameInstance>(GetGameInstance());
        if (GI)
        {
            ProfileEditWidget->SetProfileData(GI->GetCurrentUserProfile());
        }
    }

    if (UsernameText)
    {
        UsernameText->SetText(FText::FromString(Username));
    }
    
    if (LevelText)
    {
        LevelText->SetText(FText::FromString(FString::Printf(TEXT("Lv: %d"), Level)));
    }
    
    if (RemnantText)
    {
        RemnantText->SetText(FText::FromString(FString::Printf(TEXT("Remnants: %d"), RemnantCount)));
    }

    if (EmailText)
    {
        EmailText->SetText(FText::FromString(Email));
	}

	if (BioText)
	{
		BioText->SetText(FText::FromString(Bio));
	}

	if (MemberSinceText)
	{
		MemberSinceText->SetText(FText::FromString(CreatedAt.IsEmpty() ? TEXT("") : FString::Printf(TEXT("Member since: %s"), *CreatedAt.Left(10))));
	}
    
	// Store avatar URL and begin asynchronous load
	CurrentAvatarUrl = AvatarUrl;
	LoadAvatarFromUrl(AvatarUrl);

	PopulateOwnedCharacters();
}

void UUserProfileWidget::PopulateOwnedCharacters()
{
	if (!CharacterScrollBox || !OwnedCharacterCardClass)
	{
		return;
	}

	CharacterScrollBox->ClearChildren();

	UCharacterSelectionSubsystem* CharacterSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCharacterSelectionSubsystem>() : nullptr;
	if (!CharacterSubsystem)
	{
		return;
	}

	TArray<UCharacterDataAsset*> AllCharacters = CharacterSubsystem->GetAllCharactersIncludingLocked();
	for (UCharacterDataAsset* CharacterData : AllCharacters)
	{
		if (!CharacterData)
		{
			continue;
		}

		const bool bUnlocked = CharacterSubsystem->IsCharacterUnlocked(CharacterData->CharacterID);
		if (!bUnlocked)
		{
			continue;
		}

		UOwnedCharacterCardWidget* Card = CreateWidget<UOwnedCharacterCardWidget>(this, OwnedCharacterCardClass);
		if (!Card)
		{
			continue;
		}

		const bool bIsPurchased = !CharacterData->bUnlockedByDefault;
		Card->InitializeCard(CharacterData, bIsPurchased);
		CharacterScrollBox->AddChild(Card);
	}
}

void UUserProfileWidget::OnLogoutClicked()
{
	UMyOnlineGameInstance* GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance());
	if (GameInstance)
	{
		GameInstance->Logout();
		// When widget is embedded in other layouts (tabs) we don't remove it;
		// auth state handler will clear fields or hide as needed.
	}
}

void UUserProfileWidget::OnEditProfileClicked()
{
	if (ProfileEditWidget)
	{
		// make sure editor is populated and visible
		UMyOnlineGameInstance* GI = Cast<UMyOnlineGameInstance>(GetGameInstance());
		if (GI)
		{
			ProfileEditWidget->SetProfileData(GI->GetCurrentUserProfile());
		}
		ProfileEditWidget->SetVisibility(ESlateVisibility::Visible);
	}
}

void UUserProfileWidget::OnRefreshClicked()
{
	UMyOnlineGameInstance* GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance());
	if (GameInstance)
	{
		GameInstance->GetUserProfile();
	}
}

// -------------------------------------------------
// Game instance callbacks
// -------------------------------------------------

void UUserProfileWidget::HandleProfileUpdated(const FUserProfile& UserProfile)
{
	// update UI whenever GameInstance broadcasts a new profile
	if (UserProfile.bIsValid)
	{
		UpdateProfile(
			UserProfile.Username,
			UserProfile.Level,
			UserProfile.RemnantCount,
			UserProfile.AvatarUrl,
			UserProfile.Email,
			UserProfile.Bio,
			UserProfile.CreatedAt
		);
		UpdateMatchHistoryDisplay(UserProfile);
	}
}

void UUserProfileWidget::HandleAuthStateChanged(bool bIsLoggedIn)
{
	ApplyLoginGating(bIsLoggedIn);

	if (!bIsLoggedIn)
	{
		// logged out state – clear fields and owned characters list
		UpdateProfile(TEXT(""), 0, 0, TEXT(""));
		if (MatchHistoryScrollBox)
		{
			MatchHistoryScrollBox->ClearChildren();
		}
		if (MatchHistoryText)
		{
			MatchHistoryText->SetText(FText::GetEmpty());
		}
		if (CharacterScrollBox)
		{
			CharacterScrollBox->ClearChildren();
		}
	}
}

void UUserProfileWidget::UpdateMatchHistoryDisplay(const FUserProfile& UserProfile)
{
	UMyOnlineGameInstance* GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance());
	if (!GameInstance || !GameInstance->IsLoggedIn())
	{
		if (MatchHistoryScrollBox)
		{
			MatchHistoryScrollBox->ClearChildren();
		}

		if (MatchHistoryText)
		{
			MatchHistoryText->SetText(FText::GetEmpty());
		}

		return;
	}

	if (MatchHistoryScrollBox)
	{
		MatchHistoryScrollBox->ClearChildren();

		if (UserProfile.MatchHistory.Num() > 0 && MatchHistoryCardClass)
		{
			const int32 MaxRows = FMath::Max(1, MaxMatchHistoryRows);
			const int32 RowsToShow = FMath::Min(MaxRows, UserProfile.MatchHistory.Num());

			for (int32 Index = 0; Index < RowsToShow; ++Index)
			{
				UMatchHistoryCardWidget* Card = CreateWidget<UMatchHistoryCardWidget>(this, MatchHistoryCardClass);
				if (!Card)
				{
					continue;
				}

				Card->InitializeCard(UserProfile.MatchHistory[Index]);
				MatchHistoryScrollBox->AddChild(Card);
			}

			if (MatchHistoryText)
			{
				MatchHistoryText->SetText(FText::GetEmpty());
			}

			return;
		}
	}

	if (!MatchHistoryText)
	{
		return;
	}

	if (UserProfile.MatchHistory.Num() == 0)
	{
		MatchHistoryText->SetText(FText::FromString(TEXT("Recent Matches: none yet")));
		return;
	}

	const int32 MaxRows = FMath::Max(1, MaxMatchHistoryRows);
	const int32 RowsToShow = FMath::Min(MaxRows, UserProfile.MatchHistory.Num());

	FString HistoryText = TEXT("Recent Matches:\n");
	for (int32 Index = 0; Index < RowsToShow; ++Index)
	{
		const FProfileMatchHistoryEntry& Entry = UserProfile.MatchHistory[Index];
		const FString ResultLabel = Entry.bIsDraw ? TEXT("Draw") : (Entry.bIsWinner ? TEXT("Win") : TEXT("Loss"));
		const FString MapLabel = Entry.MapName.IsEmpty() ? TEXT("UnknownMap") : Entry.MapName;
		const int32 Placement = Entry.Placement > 0 ? Entry.Placement : (Entry.bIsWinner ? 1 : 0);
		const FString PlacementLabel = Placement > 0 ? FString::Printf(TEXT("P%d"), Placement) : TEXT("P?");

		HistoryText += FString::Printf(TEXT("%d) %s | %s | %s | %ds"),
			Index + 1,
			*ResultLabel,
			*MapLabel,
			*PlacementLabel,
			FMath::Max(0, Entry.DurationSeconds));

		if (Index < RowsToShow - 1)
		{
			HistoryText += TEXT("\n");
		}
	}

	MatchHistoryText->SetText(FText::FromString(HistoryText));
}


// -------------------------------------------------
// login gating helper
// -------------------------------------------------

void UUserProfileWidget::ApplyLoginGating(bool bIsLoggedIn)
{
	if (ContentPanel)
	{
		ContentPanel->SetVisibility(bIsLoggedIn ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (LoginRequiredText)
	{
		LoginRequiredText->SetVisibility(bIsLoggedIn ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
		if (!bIsLoggedIn)
		{
			LoginRequiredText->SetText(FText::FromString(TEXT("Please log in to access this feature.")));
		}
	}

	// In layouts without ContentPanel/LoginRequiredText, explicitly hide guest match history UI.
	if (MatchHistoryScrollBox)
	{
		MatchHistoryScrollBox->SetVisibility(bIsLoggedIn ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (!bIsLoggedIn)
		{
			MatchHistoryScrollBox->ClearChildren();
		}
	}

	if (MatchHistoryText)
	{
		MatchHistoryText->SetVisibility(bIsLoggedIn ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (!bIsLoggedIn)
		{
			MatchHistoryText->SetText(FText::GetEmpty());
		}
	}
}


// -------------------------------------------------
// Avatar download helpers
// -------------------------------------------------

void UUserProfileWidget::LoadAvatarFromUrl(const FString& Url)
{
    if (Url.IsEmpty() || !AvatarImage)
    {
        return;
    }

    FHttpModule* Http = &FHttpModule::Get();
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb(TEXT("GET"));
    Request->OnProcessRequestComplete().BindUObject(this, &UUserProfileWidget::OnAvatarDownloaded);
    Request->ProcessRequest();
}

void UUserProfileWidget::OnAvatarDownloaded(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
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
    // GIF files are not supported by the engine; treat them like PNG as a fallback or skip
    // any GIF-specific macro logic has been removed to avoid "Cannot resolve symbol 'GIF'".

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
                AvatarImage->SetBrushFromTexture(Texture, true);
            }
        }
    }
}