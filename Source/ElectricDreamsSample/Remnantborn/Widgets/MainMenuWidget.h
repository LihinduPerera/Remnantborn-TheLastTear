#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Components/VerticalBox.h"
#include "Auth/LoginWidget.h"
#include "Auth/UserProfileWidget.h"
#include "ElectricDreamsSample/Remnantborn/OnlineService/HttpManager/HttpManager.h"
#include "SessionInfo/SessionInfoObject.h"
#include "MainMenuWidget.generated.h"

UCLASS()
class ELECTRICDREAMSSAMPLE_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()
    
public:
	virtual void NativeConstruct() override;
    
protected:
	// === Multiplayer Handlers ===
	UFUNCTION()
	void OnHostButtonClicked();
    
	UFUNCTION()
	void OnFindSessionsButtonClicked();
    
	UFUNCTION()
	void OnJoinButtonClicked();
    
	UFUNCTION()
	void OnDirectJoinButtonClicked();
    
	UFUNCTION()
	void OnQuitButtonClicked();
    
	// ListView callbacks
	UFUNCTION()
	void OnSessionSelected(UObject* Item);
    
	UFUNCTION()
	void OnSessionDoubleClicked(UObject* Item);
    
	// === Authentication Handlers ===
	UFUNCTION()
	void OnLoginButtonClicked();
    
	UFUNCTION()
	void OnProfileButtonClicked();
    
	// Callbacks from GameInstance
	UFUNCTION()
	void HandleSessionSearchCompleted(bool bSuccess);
    
	UFUNCTION()
	void HandleCreateSessionSuccess();
    
	UFUNCTION()
	void HandleCreateSessionFailed(const FString& ErrorMessage);
    
	UFUNCTION()
	void HandleJoinSessionFailed(const FString& ErrorMessage);
    
	UFUNCTION()
	void HandleLoginComplete(const FAuthResponse& AuthResponse);
    
	UFUNCTION()
	void HandleSignupComplete(const FAuthResponse& AuthResponse);
    
	UFUNCTION()
	void HandleProfileUpdated(const FUserProfile& UserProfile);
    
	// Helper functions
	void UpdateSessionList();
	void ClearErrorMessage();
	void UpdateUserInfo();
    
	// Widget Components - Multiplayer
	UPROPERTY(meta = (BindWidget))
	UButton* HostButton;
    
	UPROPERTY(meta = (BindWidget))
	UButton* FindSessionsButton;
    
	UPROPERTY(meta = (BindWidget))
	UButton* JoinButton;
    
	UPROPERTY(meta = (BindWidget))
	UButton* DirectJoinButton;
    
	UPROPERTY(meta = (BindWidget))
	UButton* QuitButton;
    
	UPROPERTY(meta = (BindWidget))
	UEditableTextBox* ServerNameTextBox;
    
	UPROPERTY(meta = (BindWidget))
	UEditableTextBox* DirectIPTextBox;
    
	UPROPERTY(meta = (BindWidget))
	UListView* SessionListView;
    
	// Widget Components - Authentication
	UPROPERTY(meta = (BindWidget))
	UButton* LoginButton;
    
	UPROPERTY(meta = (BindWidget))
	UButton* ProfileButton;
    
	UPROPERTY(meta = (BindWidget))
	UTextBlock* WelcomeText;
    
	UPROPERTY(meta = (BindWidget))
	UTextBlock* UserLevelText;
    
	UPROPERTY(meta = (BindWidget))
	UTextBlock* UserRemnantText;
    
	UPROPERTY(meta = (BindWidget))
	UVerticalBox* UserInfoPanel;
    
	UPROPERTY(meta = (BindWidget))
	UVerticalBox* LoginPanel;
    
	// Common Widgets
	UPROPERTY(meta = (BindWidget))
	UTextBlock* StatusText;
    
	UPROPERTY(meta = (BindWidget))
	UTextBlock* ErrorText;
    
	// Widget classes
	UPROPERTY(EditAnywhere, Category = "Widgets")
	TSubclassOf<ULoginWidget> LoginWidgetClass;
    
	UPROPERTY(EditAnywhere, Category = "Widgets")
	TSubclassOf<UUserProfileWidget> ProfileWidgetClass;
    
private:
	int32 SelectedSessionIndex;
	TArray<USessionInfoObject*> SessionListItems;
    
	FTimerHandle ErrorClearTimer;
    
	// Current user data
	FUserProfile CurrentUserProfile;
	bool bIsLoggedIn = false;
    
	// Widget instances
	UPROPERTY()
	ULoginWidget* LoginWidget;
    
	UPROPERTY()
	UUserProfileWidget* ProfileWidget;
};