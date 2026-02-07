#include "LoginWidget.h"
#include "Remnantborn/Remnantborn/OnlineService/MyOnlineGameInstance.h"
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
    
    // Bind to GameInstance auth state changes
    UMyOnlineGameInstance* GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance());
    if (GameInstance)
    {
        GameInstance->OnAuthStateChanged.AddDynamic(this, &ULoginWidget::HandleAuthStateChanged);
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
    
    // Use email as username for simplicity
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

void ULoginWidget::HandleAuthStateChanged(bool bIsLoggedIn)
{
    // Unbind from events
    UMyOnlineGameInstance* GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance());
    if (GameInstance)
    {
        GameInstance->OnAuthStateChanged.RemoveDynamic(this, &ULoginWidget::HandleAuthStateChanged);
    }
    
    if (bIsLoggedIn)
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
        }, 1.0f, false);
    }
    else
    {
        if (StatusText)
        {
            StatusText->SetText(FText::FromString("Login failed"));
        }
        
        // Note: Error messages are handled by the GameInstance
        // We'll keep the widget open so user can try again
    }
}

void ULoginWidget::ClearError()
{
    if (ErrorText)
    {
        ErrorText->SetVisibility(ESlateVisibility::Hidden);
    }
}
