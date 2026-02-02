#include "CharacterPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "CharacterSelectionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"

ACharacterPlayerState::ACharacterPlayerState()
{
    SelectedCharacterID = NAME_None;
    CachedCharacterData = nullptr;
    bAlwaysRelevant = true;
    NetUpdateFrequency = 10.0f;
    MinNetUpdateFrequency = 2.0f;
}

void ACharacterPlayerState::BeginPlay()
{
    Super::BeginPlay();
    
    // Cache character data if we already have a selection
    if (HasSelectedCharacter())
    {
        CacheCharacterData();
    }
}

void ACharacterPlayerState::SetSelectedCharacter(UCharacterDataAsset* Character)
{
    if (Character)
    {
        SelectedCharacterID = Character->CharacterID;
        CachedCharacterData = Character;

        OnRep_SelectedCharacterID();
    }
}

bool ACharacterPlayerState::Server_SetSelectedCharacterID_Validate(FName CharacterID)
{
    return !CharacterID.IsNone();
}

void ACharacterPlayerState::Server_SetSelectedCharacterID_Implementation(FName CharacterID)
{
    SelectedCharacterID = CharacterID;
    OnRep_SelectedCharacterID();
}

UCharacterDataAsset* ACharacterPlayerState::GetSelectedCharacter() const
{
    return CachedCharacterData;
}

FName ACharacterPlayerState::GetSelectedCharacterID() const
{
    return SelectedCharacterID;
}

void ACharacterPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION(ACharacterPlayerState, SelectedCharacterID, COND_InitialOnly);
}

void ACharacterPlayerState::OnRep_SelectedCharacterID()
{
    // Use retry mechanism to ensure character data is properly cached
    RetryCacheCharacterData();
}

void ACharacterPlayerState::RetryCacheCharacterData(int32 RetryCount)
{
    // Max retry limit
    if (RetryCount >= 10)
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to cache character data after 10 retries for character: %s"), *SelectedCharacterID.ToString());
        return;
    }

    if (GetWorld() && GetWorld()->GetGameInstance())
    {
        UCharacterSelectionSubsystem* CharacterSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UCharacterSelectionSubsystem>();
        if (CharacterSubsystem)
        {
            CachedCharacterData = CharacterSubsystem->GetCharacterByID(SelectedCharacterID);
            
            if (CachedCharacterData)
            {
                // Success - notify clients that character selection has changed
                if (GetNetMode() != NM_DedicatedServer)
                {
                    UE_LOG(LogTemp, Log, TEXT("Character selection replicated and cached: %s"), *SelectedCharacterID.ToString());
                }

                // Call Blueprint event when ready
                OnCharacterSelectionReady();
                return;
            }
        }
    }

    // Retry with delay
    if (GetWorld())
    {
        FTimerDelegate RetryDelegate;
        RetryDelegate.BindUObject(this, &ACharacterPlayerState::RetryCacheCharacterData, RetryCount + 1);
        GetWorld()->GetTimerManager().SetTimer(RetryCacheHandle, RetryDelegate, 0.1f, false);
    }
}

void ACharacterPlayerState::CacheCharacterData()
{
    if (GetWorld() && GetWorld()->GetGameInstance())
    {
        UCharacterSelectionSubsystem* CharacterSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UCharacterSelectionSubsystem>();
        if (CharacterSubsystem)
        {
            CachedCharacterData = CharacterSubsystem->GetCharacterByID(SelectedCharacterID);
        }
    }
}
