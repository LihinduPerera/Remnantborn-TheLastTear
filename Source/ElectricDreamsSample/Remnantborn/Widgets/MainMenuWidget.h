#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "SessionInfo/SessionInfoObject.h"
#include "MainMenuWidget.generated.h"

// Forward declaration
class UMyOnlineGameInstance;

UCLASS()
class ELECTRICDREAMSSAMPLE_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()
    
public:
	virtual void NativeConstruct() override;
    
protected:
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
    
	// Callbacks from GameInstance
	UFUNCTION()
	void HandleSessionSearchCompleted(bool bSuccess);
    
	UFUNCTION()
	void HandleCreateSessionSuccess();
    
	UFUNCTION()
	void HandleCreateSessionFailed(const FString& ErrorMessage);
    
	UFUNCTION()
	void HandleJoinSessionFailed(const FString& ErrorMessage);
    
	// Helper function to update list
	void UpdateSessionList();
    
	// Clear error message with timer
	void ClearErrorMessage();
    
	// Widget Components
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
    
	UPROPERTY(meta = (BindWidget))
	UTextBlock* StatusText;
    
	UPROPERTY(meta = (BindWidget))
	UTextBlock* ErrorText;
    
private:
	int32 SelectedSessionIndex;
	TArray<USessionInfoObject*> SessionListItems;
    
	FTimerHandle ErrorClearTimer;
};