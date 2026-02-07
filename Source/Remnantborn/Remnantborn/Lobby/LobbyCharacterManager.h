#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameFramework/PlayerController.h"
#include "Remnantborn//Remnantborn/CharacterSelection/CharacterDataAsset.h"
#include "LobbyCharacterSpawnPoint.h"
#include "LobbyCharacterManager.generated.h"

USTRUCT(BlueprintType)
struct FLobbyCharacterInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    APlayerController* PlayerController = nullptr;

    UPROPERTY(BlueprintReadOnly)
    UCharacterDataAsset* CharacterData = nullptr;

    UPROPERTY(BlueprintReadOnly)
    class ARemnantbornCharacterBase* SpawnedCharacter = nullptr;

    UPROPERTY(BlueprintReadOnly)
    ALobbyCharacterSpawnPoint* SpawnPoint = nullptr;

    FLobbyCharacterInfo()
    {
        PlayerController = nullptr;
        CharacterData = nullptr;
        SpawnedCharacter = nullptr;
        SpawnPoint = nullptr;
    }
};

UCLASS(BlueprintType, Blueprintable)
class REMNANTBORN_API ULobbyCharacterManager : public UObject
{
    GENERATED_BODY()

public:
    ULobbyCharacterManager();

    virtual void Initialize();

    UFUNCTION(BlueprintCallable, Category = "Lobby Character Manager")
    void SpawnCharacterForPlayer(APlayerController* PlayerController, UCharacterDataAsset* CharacterData);

    UFUNCTION(BlueprintCallable, Category = "Lobby Character Manager")
    void DespawnCharacterForPlayer(APlayerController* PlayerController);

    UFUNCTION(BlueprintCallable, Category = "Lobby Character Manager")
    void UpdateCharacterForPlayer(APlayerController* PlayerController, UCharacterDataAsset* NewCharacterData);

    UFUNCTION(BlueprintCallable, Category = "Lobby Character Manager")
    void ClearAllCharacters();

    UFUNCTION(BlueprintCallable, Category = "Lobby Character Manager")
    ALobbyCharacterSpawnPoint* GetAvailableSpawnPoint();

    UFUNCTION(BlueprintCallable, Category = "Lobby Character Manager")
    class ARemnantbornCharacterBase* GetSpawnedCharacter(APlayerController* PlayerController);

    UFUNCTION(BlueprintCallable, Category = "Lobby Character Manager")
    TArray<FLobbyCharacterInfo> GetAllSpawnedCharacters() const { return SpawnedCharacters; }

    UFUNCTION(BlueprintCallable, Category = "Lobby Character Manager")
    void SetMaxPlayers(int32 MaxPlayers);

protected:
    UPROPERTY(BlueprintReadOnly, Category = "Lobby Character Manager")
    TArray<FLobbyCharacterInfo> SpawnedCharacters;

    UPROPERTY(BlueprintReadOnly, Category = "Lobby Character Manager")
    TArray<ALobbyCharacterSpawnPoint*> AvailableSpawnPoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby Character Manager")
    int32 MaxPlayers;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby Character Manager")
    TSubclassOf<class ARemnantbornCharacterBase> DefaultCharacterClass;

    virtual class ARemnantbornCharacterBase* CreateCharacterInstance(UCharacterDataAsset* CharacterData);
    virtual void ConfigureCharacterForLobby(class ARemnantbornCharacterBase* Character);
    virtual void ReleaseSpawnPoint(ALobbyCharacterSpawnPoint* SpawnPoint);

private:
    void FindSpawnPoints();
    void CleanupCharacter(class ARemnantbornCharacterBase* Character);
    FLobbyCharacterInfo* FindCharacterInfo(APlayerController* PlayerController);
    const FLobbyCharacterInfo* FindCharacterInfo(APlayerController* PlayerController) const;
};