#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "LoginWidget.generated.h"

UCLASS()
class ELECTRICDREAMSSAMPLE_API ULoginWidget : public UUserWidget
{
    GENERATED_BODY()
    
public:
    virtual void NativeConstruct() override;
    
    // Event handlers
    UFUNCTION()
    void OnLoginClicked();
    
    UFUNCTION()
    void OnSignupClicked();
    
    UFUNCTION()
    void OnDevLoginClicked();
    
    UFUNCTION()
    void OnCloseClicked();
    
    // Callback for auth state changes
    UFUNCTION()
    void HandleAuthStateChanged(bool bIsLoggedIn);
    
    // Widget components
    UPROPERTY(meta = (BindWidget))
    UEditableTextBox* EmailInput;
    
    UPROPERTY(meta = (BindWidget))
    UEditableTextBox* PasswordInput;
    
    UPROPERTY(meta = (BindWidget))
    UButton* LoginButton;
    
    UPROPERTY(meta = (BindWidget))
    UButton* SignupButton;
    
    UPROPERTY(meta = (BindWidget))
    UButton* DevLoginButton;
    
    UPROPERTY(meta = (BindWidget))
    UButton* CloseButton;
    
    UPROPERTY(meta = (BindWidget))
    UTextBlock* StatusText;
    
    UPROPERTY(meta = (BindWidget))
    UTextBlock* ErrorText;
    
private:
    void ClearError();
    FTimerHandle ErrorClearTimer;
};