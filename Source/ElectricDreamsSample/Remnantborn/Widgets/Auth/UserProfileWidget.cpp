#include "UserProfileWidget.h"
#include "ElectricDreamsSample/Remnantborn/OnlineService/MyOnlineGameInstance.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"

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

void UUserProfileWidget::UpdateProfile(const FString& Username, int32 Level, int32 RemnantCount, const FString& AvatarUrl)
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
    
	// Store avatar URL for potential loading
	CurrentAvatarUrl = AvatarUrl;
    
	// Note: Loading images from URL requires additional setup
	// You might want to implement an async image loader or use a placeholder
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