#include "CharacterSelectionSubsystem.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"

void UCharacterSelectionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    LoadAvailableCharacters();
}

void UCharacterSelectionSubsystem::Deinitialize()
{
    // Clear selections
    PlayerSelections.Empty();
    
    Super::Deinitialize();
}

void UCharacterSelectionSubsystem::LoadAvailableCharacters()
{
    AvailableCharacters.Empty();
    CharacterUnlockStatus.Empty();

    // Load all character data assets from the content browser
    LoadCharactersFromDirectory(TEXT("/Game/Remnantborn/Blueprints/DataStructures/CharacterDataAsset"));
    
    // Initialize unlock status
    for (UCharacterDataAsset* Character : AvailableCharacters)
    {
        if (Character)
        {
            CharacterUnlockStatus.Add(Character->CharacterID, Character->bUnlockedByDefault);
        }
    }
}

void UCharacterSelectionSubsystem::LoadCharactersFromDirectory(const FString& DirectoryPath)
{
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    
    // Convert directory path to asset path
    FString PackagePath = DirectoryPath;
    if (!PackagePath.StartsWith(TEXT("/Game/")))
    {
        PackagePath = FPaths::Combine(TEXT("/Game"), PackagePath);
    }
    
    TArray<FAssetData> AssetDataArray;
    FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    
    FARFilter Filter;
    Filter.PackagePaths.Add(*PackagePath);
    Filter.bRecursivePaths = true;
    Filter.ClassPaths.Add(UCharacterDataAsset::StaticClass()->GetClassPathName());
    
    AssetRegistry.Get().GetAssets(Filter, AssetDataArray);
    
    for (const FAssetData& AssetData : AssetDataArray)
    {
        UCharacterDataAsset* CharacterAsset = Cast<UCharacterDataAsset>(AssetData.GetAsset());
        if (CharacterAsset)
        {
            AvailableCharacters.Add(CharacterAsset);
        }
    }
    
    // Sort by character ID
    AvailableCharacters.Sort([](const UCharacterDataAsset& A, const UCharacterDataAsset& B) {
        return A.CharacterID.FastLess(B.CharacterID);
    });
}

TArray<UCharacterDataAsset*> UCharacterSelectionSubsystem::GetAvailableCharacters() const
{
    TArray<UCharacterDataAsset*> UnlockedCharacters;
    
    for (UCharacterDataAsset* Character : AvailableCharacters)
    {
        if (Character && IsCharacterUnlocked(Character->CharacterID))
        {
            UnlockedCharacters.Add(Character);
        }
    }
    
    return UnlockedCharacters;
}

TArray<UCharacterDataAsset*> UCharacterSelectionSubsystem::GetAllCharactersIncludingLocked() const
{
    return AvailableCharacters;
}

UCharacterDataAsset* UCharacterSelectionSubsystem::GetCharacterByID(const FName& CharacterID) const
{
    for (UCharacterDataAsset* Character : AvailableCharacters)
    {
        if (Character && Character->CharacterID == CharacterID)
        {
            return Character;
        }
    }
    return nullptr;
}

bool UCharacterSelectionSubsystem::IsCharacterUnlocked(const FName& CharacterID) const
{
    const bool* bUnlocked = CharacterUnlockStatus.Find(CharacterID);
    return bUnlocked ? *bUnlocked : false;
}

void UCharacterSelectionSubsystem::UnlockCharacter(const FName& CharacterID)
{
    CharacterUnlockStatus.Add(CharacterID, true);
    OnCharacterUnlocked.Broadcast(CharacterID, true);
}

void UCharacterSelectionSubsystem::SyncUnlocksFromBackend(const TArray<FString>& PurchasedItems)
{
    for (UCharacterDataAsset* Character : AvailableCharacters)
    {
        if (Character)
        {
            CharacterUnlockStatus.Add(Character->CharacterID, Character->bUnlockedByDefault);
        }
    }

    for (const FString& ItemId : PurchasedItems)
    {
        const FName CharacterID(*ItemId);
        if (CharacterUnlockStatus.Contains(CharacterID) && !CharacterUnlockStatus[CharacterID])
        {
            CharacterUnlockStatus[CharacterID] = true;
            OnCharacterUnlocked.Broadcast(CharacterID, true);
        }
    }
}

void UCharacterSelectionSubsystem::SelectCharacterForPlayer(APlayerController* PlayerController, UCharacterDataAsset* Character)
{
    if (PlayerController && Character && IsCharacterUnlocked(Character->CharacterID))
    {
        PlayerSelections.Add(PlayerController, Character);
        OnCharacterSelected.Broadcast(Character);
    }
}

UCharacterDataAsset* UCharacterSelectionSubsystem::GetSelectedCharacter(APlayerController* PlayerController) const
{
    UCharacterDataAsset* const* FoundCharacter = PlayerSelections.Find(PlayerController);
    return FoundCharacter ? *FoundCharacter : nullptr;
}

void UCharacterSelectionSubsystem::ClearPlayerSelection(APlayerController* PlayerController)
{
    PlayerSelections.Remove(PlayerController);
}