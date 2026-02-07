#include "LobbyCharacterManager.h"
#include "GameFramework/Character.h"
#include "Remnantborn/Remnantborn/GameplayAbilitySystem/Characters/RemnantbornCharacterBase.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/PlayerState.h"
#include "Remnantborn/Remnantborn/CharacterSelection/CharacterPlayerState.h"

ULobbyCharacterManager::ULobbyCharacterManager()
{
    MaxPlayers = 4;
    DefaultCharacterClass = nullptr;
}

void ULobbyCharacterManager::Initialize()
{
    FindSpawnPoints();
    UE_LOG(LogTemp, Log, TEXT("LobbyCharacterManager: Initialized with %d spawn points"), AvailableSpawnPoints.Num());
}

void ULobbyCharacterManager::SpawnCharacterForPlayer(APlayerController* PlayerController, UCharacterDataAsset* CharacterData)
{
    if (!PlayerController || !CharacterData)
    {
        UE_LOG(LogTemp, Warning, TEXT("LobbyCharacterManager: Invalid parameters for character spawning"));
        return;
    }

    FLobbyCharacterInfo* ExistingInfo = FindCharacterInfo(PlayerController);
    if (ExistingInfo && ExistingInfo->SpawnedCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("LobbyCharacterManager: Player already has a character spawned, updating instead"));
        UpdateCharacterForPlayer(PlayerController, CharacterData);
        return;
    }

    ALobbyCharacterSpawnPoint* SpawnPoint = GetAvailableSpawnPoint();
    if (!SpawnPoint)
    {
        UE_LOG(LogTemp, Warning, TEXT("LobbyCharacterManager: No available spawn points"));
        return;
    }

    ARemnantbornCharacterBase* CharacterInstance = CreateCharacterInstance(CharacterData);
    if (!CharacterInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("LobbyCharacterManager: Failed to create character instance"));
        ReleaseSpawnPoint(SpawnPoint);
        return;
    }

    FTransform SpawnTransform = SpawnPoint->GetSpawnTransform();
    CharacterInstance->SetActorTransform(SpawnTransform, false, nullptr, ETeleportType::ResetPhysics);
    
    ConfigureCharacterForLobby(CharacterInstance);

    SpawnPoint->SetOccupied(true);

    FLobbyCharacterInfo CharacterInfo;
    CharacterInfo.PlayerController = PlayerController;
    CharacterInfo.CharacterData = CharacterData;
    CharacterInfo.SpawnedCharacter = CharacterInstance;
    CharacterInfo.SpawnPoint = SpawnPoint;

    SpawnedCharacters.Add(CharacterInfo);

    // Update PlayerState with lobby character reference
    if (ACharacterPlayerState* PlayerState = PlayerController->GetPlayerState<ACharacterPlayerState>())
    {
        PlayerState->SetLobbyCharacterInstance(CharacterInstance);
    }

    FString PlayerName = PlayerController->PlayerState ? PlayerController->PlayerState->GetPlayerName() : TEXT("Unknown");
    UE_LOG(LogTemp, Log, TEXT("LobbyCharacterManager: Spawned character for player %s at spawn point %d"), 
        *PlayerName, SpawnPoint->PlayerIndex);
}

void ULobbyCharacterManager::DespawnCharacterForPlayer(APlayerController* PlayerController)
{
    FLobbyCharacterInfo* CharacterInfo = FindCharacterInfo(PlayerController);
    if (!CharacterInfo || !CharacterInfo->SpawnedCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("LobbyCharacterManager: No character found to despawn for player"));
        return;
    }

    // Clear PlayerState lobby character reference
    if (ACharacterPlayerState* PlayerState = PlayerController->GetPlayerState<ACharacterPlayerState>())
    {
        PlayerState->SetLobbyCharacterInstance(nullptr);
    }

    CleanupCharacter(CharacterInfo->SpawnedCharacter);

    if (CharacterInfo->SpawnPoint)
    {
        CharacterInfo->SpawnPoint->SetOccupied(false);
    }

    FString PlayerName = PlayerController->PlayerState ? PlayerController->PlayerState->GetPlayerName() : TEXT("Unknown");
    UE_LOG(LogTemp, Log, TEXT("LobbyCharacterManager: Despawned character for player %s"), *PlayerName);

    SpawnedCharacters.RemoveAll([PlayerController](const FLobbyCharacterInfo& Info) {
        return Info.PlayerController == PlayerController;
    });
}

void ULobbyCharacterManager::UpdateCharacterForPlayer(APlayerController* PlayerController, UCharacterDataAsset* NewCharacterData)
{
    FLobbyCharacterInfo* CharacterInfo = FindCharacterInfo(PlayerController);
    if (!CharacterInfo)
    {
        SpawnCharacterForPlayer(PlayerController, NewCharacterData);
        return;
    }

    if (!NewCharacterData || NewCharacterData == CharacterInfo->CharacterData)
    {
        return;
    }

    CleanupCharacter(CharacterInfo->SpawnedCharacter);

    ARemnantbornCharacterBase* NewCharacter = CreateCharacterInstance(NewCharacterData);
    if (!NewCharacter)
    {
        UE_LOG(LogTemp, Error, TEXT("LobbyCharacterManager: Failed to create new character for update"));
        return;
    }

    if (CharacterInfo->SpawnPoint)
    {
        FTransform SpawnTransform = CharacterInfo->SpawnPoint->GetSpawnTransform();
        NewCharacter->SetActorTransform(SpawnTransform, false, nullptr, ETeleportType::ResetPhysics);
    }

    ConfigureCharacterForLobby(NewCharacter);

    CharacterInfo->SpawnedCharacter = NewCharacter;
    CharacterInfo->CharacterData = NewCharacterData;

    APlayerState* PlayerState = PlayerController->GetPlayerState<APlayerState>();
    FString PlayerName = PlayerState ? PlayerState->GetPlayerName() : TEXT("Unknown");
    UE_LOG(LogTemp, Log, TEXT("LobbyCharacterManager: Updated character for player %s"), *PlayerName);
}

void ULobbyCharacterManager::ClearAllCharacters()
{
    for (FLobbyCharacterInfo& CharacterInfo : SpawnedCharacters)
    {
        // Clear PlayerState lobby character references
        if (CharacterInfo.PlayerController)
        {
            if (ACharacterPlayerState* PlayerState = CharacterInfo.PlayerController->GetPlayerState<ACharacterPlayerState>())
            {
                PlayerState->SetLobbyCharacterInstance(nullptr);
            }
        }

        if (CharacterInfo.SpawnedCharacter)
        {
            CleanupCharacter(CharacterInfo.SpawnedCharacter);
        }

        if (CharacterInfo.SpawnPoint)
        {
            CharacterInfo.SpawnPoint->SetOccupied(false);
        }
    }

    SpawnedCharacters.Empty();
    UE_LOG(LogTemp, Log, TEXT("LobbyCharacterManager: Cleared all characters"));
}

ALobbyCharacterSpawnPoint* ULobbyCharacterManager::GetAvailableSpawnPoint()
{
    for (ALobbyCharacterSpawnPoint* SpawnPoint : AvailableSpawnPoints)
    {
        if (SpawnPoint && !SpawnPoint->IsOccupied())
        {
            return SpawnPoint;
        }
    }
    return nullptr;
}

ARemnantbornCharacterBase* ULobbyCharacterManager::GetSpawnedCharacter(APlayerController* PlayerController)
{
    const FLobbyCharacterInfo* CharacterInfo = FindCharacterInfo(PlayerController);
    return CharacterInfo ? CharacterInfo->SpawnedCharacter : nullptr;
}

void ULobbyCharacterManager::SetMaxPlayers(int32 NewMaxPlayers)
{
    MaxPlayers = FMath::Clamp(NewMaxPlayers, 1, AvailableSpawnPoints.Num());
    UE_LOG(LogTemp, Log, TEXT("LobbyCharacterManager: Set max players to %d"), MaxPlayers);
}

ARemnantbornCharacterBase* ULobbyCharacterManager::CreateCharacterInstance(UCharacterDataAsset* CharacterData)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    TSubclassOf<ARemnantbornCharacterBase> CharacterClass = CharacterData ? CharacterData->CharacterClass : DefaultCharacterClass;
    if (!CharacterClass)
    {
        UE_LOG(LogTemp, Error, TEXT("LobbyCharacterManager: No valid character class found"));
        return nullptr;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    SpawnParams.bNoFail = true;

    ARemnantbornCharacterBase* Character = World->SpawnActor<ARemnantbornCharacterBase>(CharacterClass, SpawnParams);
    if (!Character)
    {
        UE_LOG(LogTemp, Error, TEXT("LobbyCharacterManager: Failed to spawn character"));
        return nullptr;
    }

    return Character;
}

void ULobbyCharacterManager::ConfigureCharacterForLobby(ARemnantbornCharacterBase* Character)
{
    if (!Character)
    {
        return;
    }

    if (Character->GetCapsuleComponent())
    {
        Character->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    Character->SetActorEnableCollision(false);

    if (Character->GetMesh())
    {
        Character->GetMesh()->SetVisibility(true);
        Character->GetMesh()->SetRelativeRotation(FRotator(0.f, 0.f, 0.f));
    }

    Character->SetActorTickEnabled(false);

    Character->NetDormancy = ENetDormancy::DORM_DormantAll;
}

void ULobbyCharacterManager::ReleaseSpawnPoint(ALobbyCharacterSpawnPoint* SpawnPoint)
{
    if (SpawnPoint)
    {
        SpawnPoint->SetOccupied(false);
    }
}

void ULobbyCharacterManager::FindSpawnPoints()
{
    AvailableSpawnPoints.Empty();

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("LobbyCharacterManager: No world context available"));
        return;
    }

    TArray<AActor*> FoundSpawnPoints;
    UGameplayStatics::GetAllActorsOfClass(World, ALobbyCharacterSpawnPoint::StaticClass(), FoundSpawnPoints);

    for (AActor* Actor : FoundSpawnPoints)
    {
        ALobbyCharacterSpawnPoint* SpawnPoint = Cast<ALobbyCharacterSpawnPoint>(Actor);
        if (SpawnPoint)
        {
            AvailableSpawnPoints.Add(SpawnPoint);
        }
    }

    AvailableSpawnPoints.Sort([](const ALobbyCharacterSpawnPoint& A, const ALobbyCharacterSpawnPoint& B) {
        return A.PlayerIndex < B.PlayerIndex;
    });

    UE_LOG(LogTemp, Log, TEXT("LobbyCharacterManager: Found %d spawn points"), AvailableSpawnPoints.Num());
}

void ULobbyCharacterManager::CleanupCharacter(ARemnantbornCharacterBase* Character)
{
    if (!Character)
    {
        return;
    }

    if (UWorld* World = GetWorld())
    {
        World->DestroyActor(Character);
    }
}

FLobbyCharacterInfo* ULobbyCharacterManager::FindCharacterInfo(APlayerController* PlayerController)
{
    for (FLobbyCharacterInfo& CharacterInfo : SpawnedCharacters)
    {
        if (CharacterInfo.PlayerController == PlayerController)
        {
            return &CharacterInfo;
        }
    }
    return nullptr;
}

const FLobbyCharacterInfo* ULobbyCharacterManager::FindCharacterInfo(APlayerController* PlayerController) const
{
    for (const FLobbyCharacterInfo& CharacterInfo : SpawnedCharacters)
    {
        if (CharacterInfo.PlayerController == PlayerController)
        {
            return &CharacterInfo;
        }
    }
    return nullptr;
}