#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MultiplayerPlayerController.generated.h"

UCLASS()
class ELECTRICDREAMSSAMPLE_API AMultiplayerPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;

public:
	UFUNCTION(Client, Reliable)
	void Client_SetupGameplayInput();

private:
	void SetupInputMode();
};