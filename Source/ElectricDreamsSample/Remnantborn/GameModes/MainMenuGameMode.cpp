#include "MainMenuGameMode.h"
#include "ElectricDreamsSample/Remnantborn/Widgets/MainMenuWidget.h"
#include "Blueprint/UserWidget.h"

AMainMenuGameMode::AMainMenuGameMode()
{
	static ConstructorHelpers::FClassFinder<UUserWidget> WidgetClassFinder(TEXT("/Game/UI/WBP_MainMenu"));
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
            
			// Set input mode
			APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
			if (PlayerController)
			{
				PlayerController->SetInputMode(FInputModeUIOnly());
				PlayerController->SetShowMouseCursor(true);
			}
		}
	}
}