#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "UserProfileWidget.generated.h"

class UOwnedCharacterCardWidget;

UCLASS()
class REMNANTBORN_API UUserProfileWidget : public UUserWidget
{
	GENERATED_BODY()
    
public:
	virtual void NativeConstruct() override;
    
	// Update the widget with user data
	UFUNCTION(BlueprintCallable)
	void UpdateProfile(const FString& Username, int32 Level, int32 RemnantCount, const FString& AvatarUrl = "", const FString& Email = "", const FString& Bio = "", const FString& CreatedAt = "");

	UFUNCTION(BlueprintCallable)
	void PopulateOwnedCharacters();
    
	// Event handlers
	UFUNCTION()
	void OnLogoutClicked();
    
	UFUNCTION()
	void OnRefreshClicked();

	// callbacks from GameInstance
	UFUNCTION()
	void HandleProfileUpdated(const FUserProfile& UserProfile);

	UFUNCTION()
	void HandleAuthStateChanged(bool bIsLoggedIn);
    
	// Widget components
	UPROPERTY(meta = (BindWidget))
	UTextBlock* UsernameText;
    
	UPROPERTY(meta = (BindWidget))
	UTextBlock* LevelText;
    
	UPROPERTY(meta = (BindWidget))
	UTextBlock* RemnantText;
    
	UPROPERTY(meta = (BindWidget))
	UTextBlock* EmailText;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* BioText;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* MemberSinceText;
    
	UPROPERTY(meta = (BindWidget))
	UImage* AvatarImage;
    
	UPROPERTY(meta = (BindWidget))
	UButton* LogoutButton;
    
	UPROPERTY(meta = (BindWidget))
	UButton* RefreshButton;

	UPROPERTY(meta = (BindWidgetOptional))
	UScrollBox* CharacterScrollBox;

	UPROPERTY(EditAnywhere, Category = "Widgets")
	TSubclassOf<UOwnedCharacterCardWidget> OwnedCharacterCardClass;
    
private:
	FString CurrentAvatarUrl;
};