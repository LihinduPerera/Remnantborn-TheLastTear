#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Remnantborn/Remnantborn/OnlineService/UEdsHttpService.h"
#include "StoreSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStoreCatalogLoaded, const TArray<FStoreCharacterInfo>&, Characters);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStorePackagesLoaded, const TArray<FRemnantPackage>&, Packages);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCharacterPurchaseCompleted, bool, bSuccess, const FCharacterPurchaseResponse&, Response);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRemnantPurchaseCompleted, bool, bSuccess, const FRemnantPurchaseResponse&, Response);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStoreError, const FString&, ErrorMessage);

UCLASS()
class REMNANTBORN_API UStoreSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "Store")
    void FetchStoreCatalog();

    UFUNCTION(BlueprintCallable, Category = "Store")
    void FetchRemnantPackages();

    UFUNCTION(BlueprintCallable, Category = "Store")
    void PurchaseCharacter(const FString& CharacterId);

    UFUNCTION(BlueprintCallable, Category = "Store")
    void PurchaseRemnants(const FString& PackageId, const FString& CardNumber, const FString& CardExpiry, const FString& CardCVV);

    UFUNCTION(BlueprintPure, Category = "Store")
    const TArray<FStoreCharacterInfo>& GetCachedCharacters() const { return CachedCharacters; }

    UFUNCTION(BlueprintPure, Category = "Store")
    const TArray<FRemnantPackage>& GetCachedPackages() const { return CachedPackages; }

    UPROPERTY(BlueprintAssignable, Category = "Store|Events")
    FOnStoreCatalogLoaded OnStoreCatalogLoaded;

    UPROPERTY(BlueprintAssignable, Category = "Store|Events")
    FOnStorePackagesLoaded OnStorePackagesLoaded;

    UPROPERTY(BlueprintAssignable, Category = "Store|Events")
    FOnCharacterPurchaseCompleted OnCharacterPurchaseCompleted;

    UPROPERTY(BlueprintAssignable, Category = "Store|Events")
    FOnRemnantPurchaseCompleted OnRemnantPurchaseCompleted;

    UPROPERTY(BlueprintAssignable, Category = "Store|Events")
    FOnStoreError OnStoreError;

private:
    TArray<FStoreCharacterInfo> CachedCharacters;
    TArray<FRemnantPackage> CachedPackages;

    UEdsHttpService* GetHttpService() const;
    FString GetAuthToken() const;
};
