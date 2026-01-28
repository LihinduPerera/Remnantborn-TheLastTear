#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MultiplayerGameMode.generated.h"

UCLASS()
class ELECTRICDREAMSSAMPLE_API AMultiplayerGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMultiplayerGameMode();

protected:
	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;

private:
	void SetupPlayerInput(APlayerController* PlayerController);
};