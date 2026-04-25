#include "GameplayCueNotify_DamageBurstGrunt.h"

#include "GameplayTagContainer.h"
#include "Kismet/GameplayStatics.h"
#include "Remnantborn/Remnantborn/GameplayAbilitySystem/Characters/RemnantbornCharacterBase.h"

UGameplayCueNotify_DamageBurstGrunt::UGameplayCueNotify_DamageBurstGrunt()
{
	GameplayCueTag = FGameplayTag::RequestGameplayTag(FName("GameplayCue.Damage.Burst"), false);
}

bool UGameplayCueNotify_DamageBurstGrunt::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	if (!IsValid(MyTarget))
	{
		return false;
	}

	UWorld* World = MyTarget->GetWorld();
	if (!IsValid(World) || World->GetNetMode() == NM_DedicatedServer)
	{
		return false;
	}

	const ARemnantbornCharacterBase* Character = Cast<ARemnantbornCharacterBase>(MyTarget);
	if (!Character)
	{
		return false;
	}

	USoundBase* SelectedSound = Character->SelectDamageBurstGruntSound(
		MaleDamageBurstGruntSounds,
		FemaleDamageBurstGruntSounds);

	if (!IsValid(SelectedSound))
	{
		return false;
	}

	FVector SoundLocation = MyTarget->GetActorLocation();
	if (!Parameters.Location.IsNearlyZero())
	{
		SoundLocation = FVector(
			Parameters.Location.X,
			Parameters.Location.Y,
			Parameters.Location.Z);
	}

	float PitchMultiplier = 1.0f;
	if (PitchJitter > 0.0f)
	{
		const float MinPitch = FMath::Max(0.01f, 1.0f - PitchJitter);
		const float MaxPitch = 1.0f + PitchJitter;
		PitchMultiplier = FMath::FRandRange(MinPitch, MaxPitch);
	}

	UGameplayStatics::PlaySoundAtLocation(
		MyTarget,
		SelectedSound,
		SoundLocation,
		GruntVolumeMultiplier,
		PitchMultiplier);

	return true;
}
