#include "LoginWidget.h"
#include "ElectricDreamsSample/Remnantborn/OnlineService/MyOnlineGameInstance.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "TimerManager.h"

void ULoginWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    // Bind button events
    if (LoginButton)
    {
        LoginButton->OnClicked.AddDynamic(this, &ULoginWidget::OnLoginClicked);
    }
    
    if (SignupButton)
    {
        SignupButton->OnClicked.AddDynamic(this, &ULoginWidget::OnSignupClicked);
    }
    
    if (DevLoginButton)
    {
        DevLoginButton->OnClicked.AddDynamic(this, &ULoginWidget::OnDevLoginClicked);
    }
    
    if (CloseButton)
    {
        CloseButton->OnClicked.AddDynamic(this, &ULoginWidget::OnCloseClicked);
    }
    
    // Bind to GameInstance events
    UMyOnlineGameInstance* GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance());
    if (GameInstance)
    {
        GameInstance->OnAuthLoginComplete.AddDynamic(this, &ULoginWidget::HandleLoginComplete);
    }
    
    // Hide error text initially
    if (ErrorText)
    {
        ErrorText->SetVisibility(ESlateVisibility::Hidden);
    }
    
    // Set default text
    if (StatusText)
    {
        StatusText->SetText(FText::FromString("Enter your credentials"));
    }
}

void ULoginWidget::OnLoginClicked()
{
    if (!EmailInput || !PasswordInput)
    {
        return;
    }
    
    FString Email = EmailInput->GetText().ToString();
    FString Password = PasswordInput->GetText().ToString();
    
    UE_LOG(LogTemp, Log, TEXT("Login attempt: Email=%s"), *Email);
    
    if (Email.IsEmpty() || Password.IsEmpty())
    {
        if (ErrorText)
        {
            ErrorText->SetText(FText::FromString("Please enter both email and password"));
            ErrorText->SetVisibility(ESlateVisibility::Visible);
            GetWorld()->GetTimerManager().SetTimer(ErrorClearTimer, this, &ULoginWidget::ClearError, 3.0f, false);
        }
        return;
    }
    
    if (StatusText)
    {
        StatusText->SetText(FText::FromString("Logging in..."));
    }
    
    // Clear any previous errors
    if (ErrorText)
    {
        ErrorText->SetVisibility(ESlateVisibility::Hidden);
    }
    
    UMyOnlineGameInstance* GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance());
    if (GameInstance)
    {
        UE_LOG(LogTemp, Log, TEXT("Calling GameInstance->Login"));
        GameInstance->Login(Email, Password);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("GameInstance is null!"));
        if (ErrorText)
        {
            ErrorText->SetText(FText::FromString("Game instance not found"));
            ErrorText->SetVisibility(ESlateVisibility::Visible);
        }
    }
}

void ULoginWidget::OnSignupClicked()
{
    // This would typically switch to a signup widget
    // For simplicity, we'll reuse this widget but you might want separate widgets
    if (!EmailInput || !PasswordInput)
    {
        return;
    }
    
    FString Email = EmailInput->GetText().ToString();
    FString Password = PasswordInput->GetText().ToString();
    
    if (Email.IsEmpty() || Password.IsEmpty())
    {
        if (ErrorText)
        {
            ErrorText->SetText(FText::FromString("Please enter both email and password"));
            ErrorText->SetVisibility(ESlateVisibility::Visible);
            GetWorld()->GetTimerManager().SetTimer(ErrorClearTimer, this, &ULoginWidget::ClearError, 3.0f, false);
        }
        return;
    }
    
    if (StatusText)
    {
        StatusText->SetText(FText::FromString("Creating account..."));
    }
    
    // For signup, we need a username - you might want a separate input field
    // For now, use email as username
    FString Username = Email;
    
    UMyOnlineGameInstance* GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance());
    if (GameInstance)
    {
        GameInstance->Signup(Email, Password, Username);
    }
}

void ULoginWidget::OnDevLoginClicked()
{
    if (!EmailInput)
    {
        return;
    }
    
    FString Email = EmailInput->GetText().ToString();
    
    if (Email.IsEmpty())
    {
        if (ErrorText)
        {
            ErrorText->SetText(FText::FromString("Please enter an email for dev login"));
            ErrorText->SetVisibility(ESlateVisibility::Visible);
            GetWorld()->GetTimerManager().SetTimer(ErrorClearTimer, this, &ULoginWidget::ClearError, 3.0f, false);
        }
        return;
    }
    
    if (StatusText)
    {
        StatusText->SetText(FText::FromString("Dev login..."));
    }
    
    UMyOnlineGameInstance* GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance());
    if (GameInstance)
    {
        GameInstance->DevLogin(Email);
    }
}

void ULoginWidget::OnCloseClicked()
{
    RemoveFromParent();
}

void ULoginWidget::HandleLoginComplete(const FAuthResponse& AuthResponse)
{
    UE_LOG(LogTemp, Log, TEXT("LoginWidget: HandleLoginComplete called. Success=%s, Error=%s"),
        AuthResponse.bSuccess ? TEXT("true") : TEXT("false"),
        *AuthResponse.ErrorMessage);
    
    if (AuthResponse.bSuccess)
    {
        if (StatusText)
        {
            StatusText->SetText(FText::FromString("Login successful!"));
        }
        
        // Hide the login widget after successful login
        FTimerHandle TimerHandle;
        GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
        {
            RemoveFromParent();
        }, 1.5f, false);
    }
    else
    {
        if (StatusText)
        {
            StatusText->SetText(FText::FromString("Login failed"));
        }
        
        if (ErrorText)
        {
            ErrorText->SetText(FText::FromString(AuthResponse.ErrorMessage));
            ErrorText->SetVisibility(ESlateVisibility::Visible);
            GetWorld()->GetTimerManager().SetTimer(ErrorClearTimer, this, &ULoginWidget::ClearError, 5.0f, false);
        }
    }
}

void ULoginWidget::ClearError()
{
    if (ErrorText)
    {
        ErrorText->SetVisibility(ESlateVisibility::Hidden);
    }
}