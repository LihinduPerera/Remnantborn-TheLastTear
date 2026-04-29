#include "StoreSubsystem.h"

#include "Remnantborn/Remnantborn/CharacterSelection/CharacterSelectionSubsystem.h"
#include "Remnantborn/Remnantborn/OnlineService/MyOnlineGameInstance.h"

void UStoreSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UStoreSubsystem::Deinitialize()
{
    CachedCharacters.Empty();
    CachedPackages.Empty();
    Super::Deinitialize();
}

void UStoreSubsystem::FetchStoreCatalog()
{
    UEdsHttpService* HttpService = GetHttpService();
    const FString AuthToken = GetAuthToken();

    if (!HttpService || AuthToken.IsEmpty())
    {
        OnStoreError.Broadcast(TEXT("Cannot fetch store catalog: not authenticated"));
        return;
    }

    HttpService->GetStoreCharacters(AuthToken, FOnStoreCharactersResponse::CreateLambda([this](const TArray<FStoreCharacterInfo>& Characters)
    {
        CachedCharacters = Characters;
        OnStoreCatalogLoaded.Broadcast(CachedCharacters);
    }));
}

void UStoreSubsystem::FetchRemnantPackages()
{
    UEdsHttpService* HttpService = GetHttpService();
    const FString AuthToken = GetAuthToken();

    if (!HttpService || AuthToken.IsEmpty())
    {
        OnStoreError.Broadcast(TEXT("Cannot fetch remnant packages: not authenticated"));
        return;
    }

    HttpService->GetRemnantPackages(AuthToken, FOnRemnantPackagesResponse::CreateLambda([this](const TArray<FRemnantPackage>& Packages)
    {
        CachedPackages = Packages;
        OnStorePackagesLoaded.Broadcast(CachedPackages);
    }));
}

void UStoreSubsystem::PurchaseCharacter(const FString& CharacterId)
{
    UEdsHttpService* HttpService = GetHttpService();
    const FString AuthToken = GetAuthToken();

    if (!HttpService || AuthToken.IsEmpty())
    {
        FCharacterPurchaseResponse Response;
        Response.bSuccess = false;
        Response.ErrorMessage = TEXT("Cannot purchase character: not authenticated");
        OnCharacterPurchaseCompleted.Broadcast(false, Response);
        OnStoreError.Broadcast(Response.ErrorMessage);
        return;
    }

    HttpService->BuyCharacter(AuthToken, CharacterId, FOnCharacterPurchaseResponse::CreateLambda([this, CharacterId](const FCharacterPurchaseResponse& Response)
    {
        if (Response.bSuccess)
        {
            if (UCharacterSelectionSubsystem* CharacterSubsystem = GetGameInstance()->GetSubsystem<UCharacterSelectionSubsystem>())
            {
                CharacterSubsystem->UnlockCharacter(FName(*CharacterId));
            }

            if (UMyOnlineGameInstance* GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance()))
            {
                FUserProfile Profile = GameInstance->GetCurrentUserProfile();
                Profile.RemnantCount = Response.NewRemnantCount;
                if (!CharacterId.IsEmpty() && !Profile.PurchasedItems.Contains(CharacterId))
                {
                    Profile.PurchasedItems.Add(CharacterId);
                }
                GameInstance->ApplyProfileUpdate(Profile);
                GameInstance->GetMyProfile();
            }

            for (FStoreCharacterInfo& Character : CachedCharacters)
            {
                if (Character.ItemId == CharacterId)
                {
                    Character.bOwned = true;
                }
                Character.bCanAfford = Response.NewRemnantCount >= Character.Price;
            }
        }
        else
        {
            OnStoreError.Broadcast(Response.ErrorMessage.IsEmpty() ? TEXT("Character purchase failed") : Response.ErrorMessage);
        }

        OnCharacterPurchaseCompleted.Broadcast(Response.bSuccess, Response);
    }));
}

void UStoreSubsystem::PurchaseRemnants(const FString& PackageId, const FString& CardNumber, const FString& CardExpiry, const FString& CardCVV)
{
    UEdsHttpService* HttpService = GetHttpService();
    const FString AuthToken = GetAuthToken();

    if (!HttpService || AuthToken.IsEmpty())
    {
        FRemnantPurchaseResponse Response;
        Response.bSuccess = false;
        Response.ErrorMessage = TEXT("Cannot purchase remnants: not authenticated");
        OnRemnantPurchaseCompleted.Broadcast(false, Response);
        OnStoreError.Broadcast(Response.ErrorMessage);
        return;
    }

    HttpService->BuyRemnants(AuthToken, PackageId, CardNumber, CardExpiry, CardCVV, FOnRemnantPurchaseResponse::CreateLambda([this](const FRemnantPurchaseResponse& Response)
    {
        if (Response.bSuccess)
        {
            if (UMyOnlineGameInstance* GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance()))
            {
                FUserProfile Profile = GameInstance->GetCurrentUserProfile();
                Profile.RemnantCount = Response.NewRemnantCount;
                GameInstance->ApplyProfileUpdate(Profile);
                GameInstance->GetMyProfile();
            }

            for (FStoreCharacterInfo& Character : CachedCharacters)
            {
                Character.bCanAfford = Response.NewRemnantCount >= Character.Price;
            }
        }
        else
        {
            OnStoreError.Broadcast(Response.ErrorMessage.IsEmpty() ? TEXT("Remnant purchase failed") : Response.ErrorMessage);
        }

        OnRemnantPurchaseCompleted.Broadcast(Response.bSuccess, Response);
    }));
}

UEdsHttpService* UStoreSubsystem::GetHttpService() const
{
    if (const UMyOnlineGameInstance* GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance()))
    {
        return GameInstance->GetHttpService();
    }

    return nullptr;
}

FString UStoreSubsystem::GetAuthToken() const
{
    if (const UMyOnlineGameInstance* GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance()))
    {
        return GameInstance->GetAuthToken();
    }

    return FString();
}
