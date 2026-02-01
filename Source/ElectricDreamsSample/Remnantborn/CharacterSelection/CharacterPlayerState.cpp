#include "CharacterPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "CharacterSelectionSubsystem.h"
#include "Engine/GameInstance.h"

ACharacterPlayerState::ACharacterPlayerState()
{
    SelectedCharacterID = NAME_None;
    CachedCharacterData = nullptr;
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

    DOREPLIFETIME(ACharacterPlayerState, SelectedCharacterID);
}

void ACharacterPlayerState::OnRep_SelectedCharacterID()
{
    CacheCharacterData();
}

void ACharacterPlayerState::CacheCharacterData()
{
    if (GetWorld())
    {
        UCharacterSelectionSubsystem* CharacterSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UCharacterSelectionSubsystem>();
        if (CharacterSubsystem)
        {
            CachedCharacterData = CharacterSubsystem->GetCharacterByID(SelectedCharacterID);
        }
    }
}
