#include "ProfileEditWidget.h"
#include "Remnantborn/Remnantborn/OnlineService/MyOnlineGameInstance.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/VerticalBox.h"
#include "Components/ProgressBar.h"

void UProfileEditWidget::NativeConstruct()
{
	Super::NativeConstruct();
    
	if (EditButton)
	{
		EditButton->OnClicked.AddDynamic(this, &UProfileEditWidget::OnEditClicked);
	}
    
	if (SaveButton)
	{
		SaveButton->OnClicked.AddDynamic(this, &UProfileEditWidget::OnSaveClicked);
	}
    
	if (CancelButton)
	{
		CancelButton->OnClicked.AddDynamic(this, &UProfileEditWidget::OnCancelClicked);
	}
    
	if (ChangeAvatarButton)
	{
		ChangeAvatarButton->OnClicked.AddDynamic(this, &UProfileEditWidget::OnChangeAvatarClicked);
	}
    
	if (LogoutButton)
	{
		LogoutButton->OnClicked.AddDynamic(this, &UProfileEditWidget::OnLogoutClicked);
	}
    
	UMyOnlineGameInstance* GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance());
	if (GameInstance)
	{
		GameInstance->OnProfileUpdated.AddDynamic(this, &UProfileEditWidget::OnProfileUpdated);
        
		if (GameInstance->IsLoggedIn())
		{
			SetProfileData(GameInstance->GetCurrentUserProfile());
		}
	}
    
	if (LoadingBar)
	{
		LoadingBar->SetVisibility(ESlateVisibility::Collapsed);
	}
    
	SetEditMode(false);
}

void UProfileEditWidget::NativeDestruct()
{
	Super::NativeDestruct();
    
	UMyOnlineGameInstance* GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance());
	if (GameInstance)
	{
		GameInstance->OnProfileUpdated.RemoveDynamic(this, &UProfileEditWidget::OnProfileUpdated);
	}
}

void UProfileEditWidget::SetProfileData(const FUserProfile& Profile)
{
	CurrentProfile = Profile;
	UpdateDisplayWithProfile();
}

void UProfileEditWidget::UpdateDisplayWithProfile()
{
	if (UsernameText)
	{
		UsernameText->SetText(FText::FromString(CurrentProfile.Username));
	}
    
	if (LevelText)
	{
		LevelText->SetText(FText::FromString(FString::Printf(TEXT("Level: %d"), CurrentProfile.Level)));
	}
    
	if (RemnantText)
	{
		RemnantText->SetText(FText::FromString(FString::Printf(TEXT("Remnants: %d"), CurrentProfile.RemnantCount)));
	}
    
	if (EmailText && CurrentProfile.bIsValid)
	{
		EmailText->SetText(FText::FromString(CurrentProfile.Email));
	}
    
	if (BioText)
	{
		FString Bio = CurrentProfile.Bio.IsEmpty() ? TEXT("No bio set") : CurrentProfile.Bio;
		BioText->SetText(FText::FromString(Bio));
	}
    
	if (CreatedAtText)
	{
		if (!CurrentProfile.CreatedAt.IsEmpty())
		{
			CreatedAtText->SetText(FText::FromString(FString::Printf(TEXT("Member since: %s"), *CurrentProfile.CreatedAt)));
		}
		else
		{
			CreatedAtText->SetText(FText::FromString(TEXT("Member since: Unknown")));
		}
	}
    
	if (UsernameInput)
	{
		UsernameInput->SetText(FText::FromString(CurrentProfile.Username));
	}
    
	if (BioInput)
	{
		BioInput->SetText(FText::FromString(CurrentProfile.Bio));
	}
    
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(TEXT("")));
	}
}

void UProfileEditWidget::SetEditMode(bool bEditMode)
{
	bIsEditMode = bEditMode;
    
	if (DisplayBox)
	{
		DisplayBox->SetVisibility(bEditMode ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
    
	if (EditBox)
	{
		EditBox->SetVisibility(bEditMode ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
    
	if (EditButton)
	{
		EditButton->SetVisibility(bEditMode ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
    
	if (SaveButton)
	{
		SaveButton->SetVisibility(bEditMode ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
    
	if (CancelButton)
	{
		CancelButton->SetVisibility(bEditMode ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
    
	if (ChangeAvatarButton)
	{
		ChangeAvatarButton->SetVisibility(bEditMode ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
    
	if (EditButtonText)
	{
		EditButtonText->SetText(FText::FromString(bEditMode ? TEXT("Editing...") : TEXT("Edit Profile")));
	}
}

void UProfileEditWidget::OnEditClicked()
{
	SetEditMode(true);
}

void UProfileEditWidget::OnSaveClicked()
{
	FString NewUsername;
	FString NewBio;
    
	if (UsernameInput)
	{
		NewUsername = UsernameInput->GetText().ToString();
	}
    
	if (BioInput)
	{
		NewBio = BioInput->GetText().ToString();
	}
    
	if (NewUsername.IsEmpty())
	{
		if (StatusText)
		{
			StatusText->SetText(FText::FromString(TEXT("Username cannot be empty")));
		}
		return;
	}
    
	if (LoadingBar)
	{
		LoadingBar->SetVisibility(ESlateVisibility::Visible);
	}
    
	UMyOnlineGameInstance* GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance());
	if (GameInstance)
	{
		GameInstance->UpdateProfile(NewUsername, NewBio);
	}
    
	SetEditMode(false);
    
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(TEXT("Saving...")));
	}
}

void UProfileEditWidget::OnCancelClicked()
{
	UpdateDisplayWithProfile();
	SetEditMode(false);
    
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(TEXT("")));
	}
}

void UProfileEditWidget::OnChangeAvatarClicked()
{
	UMyOnlineGameInstance* GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance());
	if (!GameInstance)
	{
		return;
	}

	if (StatusText)
	{
		StatusText->SetText(FText::FromString(TEXT("Avatar selection coming soon...")));
	}

	/* 
	 * Avatar selection implementation options:
	 * 1. Use a UMG ListView with pre-defined avatar options
	 * 2. Implement a Blueprint-callable function to pass the selected file path
	 * 3. Use platform-specific file picker
	 * 
	 * For now, you can call GameInstance->UploadAvatar(FullPath) from Blueprint
	 * after the user selects an image through a file picker widget.
	 */
}

void UProfileEditWidget::UploadSelectedAvatar(FString FilePath)
{
	if (FilePath.IsEmpty())
	{
		if (StatusText)
		{
			StatusText->SetText(FText::FromString(TEXT("Please select an image file")));
		}
		return;
	}

	UMyOnlineGameInstance* GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance());
	if (!GameInstance)
	{
		return;
	}

	if (LoadingBar)
	{
		LoadingBar->SetVisibility(ESlateVisibility::Visible);
	}

	if (StatusText)
	{
		StatusText->SetText(FText::FromString(TEXT("Uploading avatar...")));
	}

	GameInstance->UploadAvatar(FilePath);
}

void UProfileEditWidget::OnLogoutClicked()
{
	UMyOnlineGameInstance* GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance());
	if (GameInstance)
	{
		GameInstance->Logout();
	}
	RemoveFromParent();
}

void UProfileEditWidget::OnProfileUpdated(const FUserProfile& Profile)
{
	CurrentProfile = Profile;
	UpdateDisplayWithProfile();
    
	if (LoadingBar)
	{
		LoadingBar->SetVisibility(ESlateVisibility::Collapsed);
	}
    
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(TEXT("Profile updated!")));
	}
}

void UProfileEditWidget::OnUploadAvatarComplete(bool bSuccess)
{
	if (LoadingBar)
	{
		LoadingBar->SetVisibility(ESlateVisibility::Collapsed);
	}
    
	if (StatusText)
	{
		if (bSuccess)
		{
			StatusText->SetText(FText::FromString(TEXT("Avatar uploaded!")));
		}
		else
		{
			StatusText->SetText(FText::FromString(TEXT("Failed to upload avatar")));
		}
	}
}
