#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/ProgressBar.h"
#include "Remnantborn/Remnantborn/OnlineService/UEdsHttpService.h"
#include "ProfileEditWidget.generated.h"

UCLASS()
class REMNANTBORN_API UProfileEditWidget : public UUserWidget
{
	GENERATED_BODY()
    
public:
	virtual void NativeConstruct() override;
    
	void SetProfileData(const FUserProfile& Profile);
    
	UFUNCTION(BlueprintCallable)
	void OnSaveClicked();
    
	UFUNCTION(BlueprintCallable)
	void OnCancelClicked();
    
	UFUNCTION(BlueprintCallable)
	void OnChangeAvatarClicked();


	UFUNCTION(BlueprintCallable)
	void UploadSelectedAvatar(FString FilePath);
    
	UFUNCTION()
	void OnProfileUpdated(const FUserProfile& Profile);
    
	UFUNCTION()
	void OnUploadAvatarComplete(bool bSuccess);
    
protected:
	virtual void NativeDestruct() override;
    
private:
	void UpdateDisplayWithProfile();

	// avatar download helper
	void LoadAvatarFromUrl(const FString& Url);
	void OnAvatarDownloaded(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);
    
	UPROPERTY(meta = (BindWidget))
	UImage* AvatarImage;
    
	UPROPERTY(meta = (BindWidget))
	UButton* SaveButton;
    
	UPROPERTY(meta = (BindWidget))
	UButton* CancelButton;
    
	UPROPERTY(meta = (BindWidget))
	UButton* ChangeAvatarButton;
    

    
	UPROPERTY(meta = (BindWidget))
	UTextBlock* UsernameText;
    
	UPROPERTY(meta = (BindWidget))
	UTextBlock* LevelText;
    
	UPROPERTY(meta = (BindWidget))
	UTextBlock* RemnantText;
    
	UPROPERTY(meta = (BindWidget))
	UTextBlock* EmailText;
    
	UPROPERTY(meta = (BindWidget))
	UTextBlock* BioText;
    
	UPROPERTY(meta = (BindWidget))
	UTextBlock* CreatedAtText;
    
	UPROPERTY(meta = (BindWidget))
	UTextBlock* StatusText;
    
	UPROPERTY(meta = (BindWidget))
	UEditableTextBox* UsernameInput;
    
	UPROPERTY(meta = (BindWidget))
	UEditableTextBox* BioInput;
    
	UPROPERTY(meta = (BindWidget))
	UVerticalBox* DisplayBox;
    
	UPROPERTY(meta = (BindWidget))
	UVerticalBox* EditBox;
    
	UPROPERTY(meta = (BindWidget))
	UProgressBar* LoadingBar;
    
	FUserProfile CurrentProfile;
};
