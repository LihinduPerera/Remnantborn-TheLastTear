#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Interfaces/IHttpRequest.h"
#include "UserProfileWidget.generated.h"

class UOwnedCharacterCardWidget;
class UProfileEditWidget; // forward declare profile editor
class UWidget; // container used for login gating

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

	// profile editing
	UFUNCTION()
	void OnEditProfileClicked();

	// callbacks from GameInstance
	UFUNCTION()
	void HandleProfileUpdated(const FUserProfile& UserProfile);

	UFUNCTION()
	void HandleAuthStateChanged(bool bIsLoggedIn);
    
	// Widget components
	UPROPERTY(meta = (BindWidget))
	UTextBlock* UsernameText;

protected:
	// content shown when user is logged in
	UPROPERTY(meta = (BindWidgetOptional))
	UWidget* ContentPanel;

	// message shown when login is required
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* LoginRequiredText;

private:
	// helper used by several widgets to toggle login gate
	void ApplyLoginGating(bool bIsLoggedIn);

	    
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

	// button that appears on the profile page to launch the editor
	UPROPERTY(meta = (BindWidget))
	UButton* EditProfileButton;

	// class to use when spawning the editor at runtime (selectable in defaults)
	UPROPERTY(EditAnywhere, Category = "Widgets")
	TSubclassOf<UProfileEditWidget> ProfileEditWidgetClass;

	// pointer to the spawned/editor instance; if you drag one into the widget tree
	// then this will be assigned automatically via binding, otherwise we spawn it
	UPROPERTY(meta = (BindWidgetOptional))
	UProfileEditWidget* ProfileEditWidget;
    
private:
	FString CurrentAvatarUrl;

	// download helper
	void LoadAvatarFromUrl(const FString& Url);
	void OnAvatarDownloaded(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);

};