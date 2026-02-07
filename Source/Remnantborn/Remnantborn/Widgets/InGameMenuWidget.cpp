#include "InGameMenuWidget.h"
#include "Components/Button.h"
#include "Remnantborn/Remnantborn/OnlineService/MyOnlineGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void UInGameMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    if (ReturnToMenuButton)
    {
        ReturnToMenuButton->OnClicked.AddDynamic(this, &UInGameMenuWidget::OnReturnToMenuClicked);
    }
    
    if (QuitGameButton)
    {
        QuitGameButton->OnClicked.AddDynamic(this, &UInGameMenuWidget::OnQuitGameClicked);
    }
    
    if (CancelButton)
    {
        CancelButton->OnClicked.AddDynamic(this, &UInGameMenuWidget::OnCancelClicked);
    }
    
    // Initially hidden
    SetVisibility(ESlateVisibility::Hidden);
}

void UInGameMenuWidget::OnReturnToMenuClicked()
{
    UMyOnlineGameInstance* GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance());
    if (GameInstance)
    {
        GameInstance->LeaveGame();
    }
}

void UInGameMenuWidget::OnQuitGameClicked()
{
    UKismetSystemLibrary::QuitGame(GetWorld(), GetWorld()->GetFirstPlayerController(), EQuitPreference::Quit, false);
}

void UInGameMenuWidget::OnCancelClicked()
{
    ToggleMenu();
}

void UInGameMenuWidget::ToggleMenu()
{
    bIsMenuOpen = !bIsMenuOpen;
    
    if (bIsMenuOpen)
    {
        SetVisibility(ESlateVisibility::Visible);
        
        APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
        if (PlayerController)
        {
            PlayerController->SetInputMode(FInputModeUIOnly());
            PlayerController->SetShowMouseCursor(true);
            PlayerController->SetPause(true);
        }
    }
    else
    {
        SetVisibility(ESlateVisibility::Hidden);
        
        APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
        if (PlayerController)
        {
            PlayerController->SetInputMode(FInputModeGameOnly());
            PlayerController->SetShowMouseCursor(false);
            PlayerController->SetPause(false);
        }
    }
}