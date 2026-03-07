#include "UserProfileWidget.h"
#include "Remnantborn/Remnantborn/OnlineService/MyOnlineGameInstance.h"
#include "Remnantborn/Remnantborn/CharacterSelection/CharacterSelectionSubsystem.h"
#include "OwnedCharacterCardWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"

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
	}
}

void UUserProfileWidget::UpdateProfile(const FString& Username, int32 Level, int32 RemnantCount, const FString& AvatarUrl, const FString& Email, const FString& Bio, const FString& CreatedAt)
{
    if (UsernameText)
    {
        UsernameText->SetText(FText::FromString(Username));
    }
    
    if (LevelText)
    {
        LevelText->SetText(FText::FromString(FString::Printf(TEXT("Level: %d"), Level)));
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
    
	// Store avatar URL for potential loading
	CurrentAvatarUrl = AvatarUrl;
    
	// Note: Loading images from URL requires additional setup
	// You might want to implement an async image loader or use a placeholder

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
	}
}

void UUserProfileWidget::HandleAuthStateChanged(bool bIsLoggedIn)
{
	if (!bIsLoggedIn)
	{
		// logged out state – clear fields and owned characters list
		UpdateProfile(TEXT(""), 0, 0, TEXT(""));
		if (CharacterScrollBox)
		{
			CharacterScrollBox->ClearChildren();
		}
	}
}
