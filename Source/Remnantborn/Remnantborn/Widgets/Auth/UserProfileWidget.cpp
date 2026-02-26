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
}

void UUserProfileWidget::UpdateProfile(const FString& Username, int32 Level, int32 RemnantCount, const FString& AvatarUrl, const FString& Bio, const FString& CreatedAt)
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
		RemoveFromParent(); // Hide profile widget after logout
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