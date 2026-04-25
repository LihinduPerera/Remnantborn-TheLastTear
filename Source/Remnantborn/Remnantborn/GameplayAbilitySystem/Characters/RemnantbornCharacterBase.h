// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "RemnantbornCharacterBase.generated.h"

class AMultiplayerGameMode;
class USoundBase;

UENUM(BlueprintType)
enum class ERemnantbornCharacterGender : uint8
{
	Male UMETA(DisplayName = "Male"),
	Female UMETA(DisplayName = "Female")
};

UCLASS()
class REMNANTBORN_API ARemnantbornCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ARemnantbornCharacterBase();
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AbilitySystem")
	UAbilitySystemComponent* AbilitySystemComponent;
	
	//Basic Attributes
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AbilitySystem")
	class UBasicAttributeSet* BasicAttributeSet;
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AbilitySystem")
	EGameplayEffectReplicationMode AscReplicationMode = EGameplayEffectReplicationMode::Mixed;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AbilitySystem")
	TArray<TSubclassOf<UGameplayAbility>> StartingAbilities;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Audio|Damage")
	ERemnantbornCharacterGender CharacterGender = ERemnantbornCharacterGender::Male;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Audio|Damage")
	TArray<USoundBase*> DamageBurstGruntOverrideSounds;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	virtual void PossessedBy(AController* NewController) override;
	
	virtual void OnRep_PlayerState() override;
	
	virtual void OnDeadTagChanged(const FGameplayTag CallbackTag, int32 NewCount);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Damage")
	void HandleDeath();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	UFUNCTION(BlueprintCallable, Category = "AbilitySystem")
	TArray<FGameplayAbilitySpecHandle> GrantAbilities(TArray<TSubclassOf<UGameplayAbility>> AbilitiesToGrant);
	
	UFUNCTION(BlueprintCallable, Category = "AbilitySystem")
	void RemoveAbilities(TArray<FGameplayAbilitySpecHandle> AbilityHandlesToRemove);
	
	UFUNCTION(BlueprintCallable, Category = "AbilitySystem")
	void SendAbilitiesChangedEvent();
	
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "AbilitySystem")
	void ServerSendGameplayEventToSelf(FGameplayEventData EventData);

	// HUD Widget
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UUserWidget> PlayerVitalsWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	class UUserWidget* PlayerVitalsWidget;

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnCharacterReadyForUI();

	// Called when abilities are granted and character is ready for UI
	virtual void OnAbilitiesGrantedAndReady();

	// Client RPC to notify when character is ready for UI
	UFUNCTION(Client, Reliable)
	void Client_OnCharacterReadyForUI();

	// Helper function to set up player HUD
	void SetupPlayerHUD();

	UFUNCTION(BlueprintPure, Category="Audio|Damage")
	ERemnantbornCharacterGender GetCharacterGender() const { return CharacterGender; }

	UFUNCTION(BlueprintPure, Category="Audio|Damage")
	bool HasDamageBurstGruntOverrides() const;

	UFUNCTION(BlueprintPure, Category="Audio|Damage")
	USoundBase* GetRandomDamageBurstGruntOverride() const;

	UFUNCTION(BlueprintCallable, Category="Audio|Damage")
	USoundBase* SelectDamageBurstGruntSound(
		const TArray<USoundBase*>& MaleFallbackSounds,
		const TArray<USoundBase*>& FemaleFallbackSounds) const;

private:
	static USoundBase* PickRandomValidSound(const TArray<USoundBase*>& Sounds);
};
