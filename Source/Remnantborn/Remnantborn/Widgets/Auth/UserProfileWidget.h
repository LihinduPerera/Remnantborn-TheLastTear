#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "UserProfileWidget.generated.h"

UCLASS()
class REMNANTBORN_API UUserProfileWidget : public UUserWidget
{
	GENERATED_BODY()
    
public:
	virtual void NativeConstruct() override;
    
	// Update the widget with user data
	UFUNCTION(BlueprintCallable)
	void UpdateProfile(const FString& Username, int32 Level, int32 RemnantCount, const FString& AvatarUrl = "");
    
	// Event handlers
	UFUNCTION()
	void OnLogoutClicked();
    
	UFUNCTION()
	void OnRefreshClicked();
    
	// Widget components
	UPROPERTY(meta = (BindWidget))
	UTextBlock* UsernameText;
    
	UPROPERTY(meta = (BindWidget))
	UTextBlock* LevelText;
    
	UPROPERTY(meta = (BindWidget))
	UTextBlock* RemnantText;
    
	UPROPERTY(meta = (BindWidget))
	UTextBlock* EmailText;
    
	UPROPERTY(meta = (BindWidget))
	UImage* AvatarImage;
    
	UPROPERTY(meta = (BindWidget))
	UButton* LogoutButton;
    
	UPROPERTY(meta = (BindWidget))
	UButton* RefreshButton;
    
private:
	FString CurrentAvatarUrl;
};