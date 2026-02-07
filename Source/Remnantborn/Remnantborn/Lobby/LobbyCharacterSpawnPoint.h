#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LobbyCharacterSpawnPoint.generated.h"

UCLASS(BlueprintType, Blueprintable)
class REMNANTBORN_API ALobbyCharacterSpawnPoint : public AActor
{
    GENERATED_BODY()

public:
    ALobbyCharacterSpawnPoint();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class USceneComponent* SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UArrowComponent* SpawnDirection;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UBoxComponent* SpawnArea;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn Point")
    int32 PlayerIndex;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Point")
    bool bIsOccupied;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Point")
    FText DisplayName;

    UFUNCTION(BlueprintCallable, Category = "Spawn Point")
    FTransform GetSpawnTransform() const;

    UFUNCTION(BlueprintCallable, Category = "Spawn Point")
    void SetOccupied(bool bNewOccupied);

    UFUNCTION(BlueprintCallable, Category = "Spawn Point")
    bool IsOccupied() const { return bIsOccupied; }

protected:
    virtual void BeginPlay() override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
    void UpdateVisualState();
};