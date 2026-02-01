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
    virtual void Logout(AController* Exiting) override;
    virtual void PostSeamlessTravel() override;
    
private:
	void SetupPlayerInput(APlayerController* PlayerController);
    
    UFUNCTION()
    void SpawnPlayerWithCharacter(APlayerController* PlayerController);
};