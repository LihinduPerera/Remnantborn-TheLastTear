#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "GameplayCueNotify_DamageBurstGrunt.generated.h"

class USoundBase;

UCLASS(BlueprintType, Blueprintable)
class REMNANTBORN_API UGameplayCueNotify_DamageBurstGrunt : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	UGameplayCueNotify_DamageBurstGrunt();

	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio|Damage")
	TArray<USoundBase*> MaleDamageBurstGruntSounds;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio|Damage")
	TArray<USoundBase*> FemaleDamageBurstGruntSounds;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio|Damage", meta=(ClampMin="0.0"))
	float GruntVolumeMultiplier = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio|Damage", meta=(ClampMin="0.0", ClampMax="1.0"))
	float PitchJitter = 0.05f;
};
