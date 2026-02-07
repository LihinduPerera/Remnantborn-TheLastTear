#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MainMenuGameMode.generated.h"

UCLASS()
class REMNANTBORN_API AMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()
    
public:
	AMainMenuGameMode();
    
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UMainMenuWidget> MainMenuWidgetClass;
    
private:
	UPROPERTY()
	class UMainMenuWidget* MainMenuWidget;
    
	void CleanupInputMode();
};