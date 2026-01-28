#include "MainMenuGameMode.h"
#include "ElectricDreamsSample/Remnantborn/Widgets/MainMenuWidget.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"

AMainMenuGameMode::AMainMenuGameMode()
{
    static ConstructorHelpers::FClassFinder<UUserWidget> WidgetClassFinder(TEXT("/Game/Remnantborn/Widgets/OnlineService/WBP_MainMenu"));
    if (WidgetClassFinder.Succeeded())
    {
        MainMenuWidgetClass = WidgetClassFinder.Class;
    }
}

void AMainMenuGameMode::BeginPlay()
{
    Super::BeginPlay();
    
    if (MainMenuWidgetClass)
    {
        MainMenuWidget = CreateWidget<UMainMenuWidget>(GetWorld(), MainMenuWidgetClass);
        if (MainMenuWidget)
        {
            MainMenuWidget->AddToViewport();
            
            // Set input mode for UI
            APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
            if (PlayerController)
            {
                FInputModeUIOnly InputMode;
                InputMode.SetWidgetToFocus(MainMenuWidget->TakeWidget());
                InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
                PlayerController->SetInputMode(InputMode);
                PlayerController->SetShowMouseCursor(true);
            }
        }
    }
}

void AMainMenuGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    CleanupInputMode();
    Super::EndPlay(EndPlayReason);
}

void AMainMenuGameMode::CleanupInputMode()
{
    // Clean up UI widget
    if (MainMenuWidget)
    {
        MainMenuWidget->RemoveFromParent();
        MainMenuWidget = nullptr;
    }
    
    // Reset input mode for all players
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (APlayerController* PlayerController = It->Get())
        {
            FInputModeGameOnly GameInputMode;
            PlayerController->SetInputMode(GameInputMode);
            PlayerController->SetShowMouseCursor(false);
            PlayerController->bShowMouseCursor = false;
        }
    }
}