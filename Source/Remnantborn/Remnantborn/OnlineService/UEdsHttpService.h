// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "UEdsHttpService.generated.h"

USTRUCT(BlueprintType)
struct FProfileMatchHistoryEntry
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "User Profile")
    FString MatchId;

    UPROPERTY(BlueprintReadWrite, Category = "User Profile")
    FString MapName;

    UPROPERTY(BlueprintReadWrite, Category = "User Profile")
    FString GameMode;

    UPROPERTY(BlueprintReadWrite, Category = "User Profile")
    FString EndedAt;

    UPROPERTY(BlueprintReadWrite, Category = "User Profile")
    int32 DurationSeconds = 0;

    UPROPERTY(BlueprintReadWrite, Category = "User Profile")
    bool bIsDraw = false;

    UPROPERTY(BlueprintReadWrite, Category = "User Profile")
    int32 Placement = 0;

    UPROPERTY(BlueprintReadWrite, Category = "User Profile")
    int32 EliminationOrder = 0;

    UPROPERTY(BlueprintReadWrite, Category = "User Profile")
    float SurvivalTimeSeconds = 0.0f;

    UPROPERTY(BlueprintReadWrite, Category = "User Profile")
    bool bIsWinner = false;

    UPROPERTY(BlueprintReadWrite, Category = "User Profile")
    FString CharacterId;

    UPROPERTY(BlueprintReadWrite, Category = "User Profile")
    int32 RewardAmount = 0;
};

// User Profile Structure
USTRUCT(BlueprintType)
struct FUserProfile
{
    GENERATED_BODY()
    
    UPROPERTY(BlueprintReadWrite, Category = "User Profile")
    FString UserId;
    
    UPROPERTY(BlueprintReadWrite, Category = "User Profile")
    FString Username;
    
    UPROPERTY(BlueprintReadWrite, Category = "User Profile")
    FString Email;
    
    UPROPERTY(BlueprintReadWrite, Category = "User Profile")
    int32 Level = 1;
    
    UPROPERTY(BlueprintReadWrite, Category = "User Profile")
    int32 RemnantCount = 100;
    
    UPROPERTY(BlueprintReadWrite, Category = "User Profile")
    FString AvatarUrl;
    
    UPROPERTY(BlueprintReadWrite, Category = "User Profile")
    FString Bio;
    
    UPROPERTY(BlueprintReadWrite, Category = "User Profile")
    FString LastActive;
    
    UPROPERTY(BlueprintReadWrite, Category = "User Profile")
    FString CreatedAt;
    
    UPROPERTY(BlueprintReadWrite, Category = "User Profile")
    FString UpdatedAt;
    
    UPROPERTY(BlueprintReadWrite, Category = "User Profile")
    TArray<FString> PurchasedItems;

    UPROPERTY(BlueprintReadWrite, Category = "User Profile")
    TArray<FProfileMatchHistoryEntry> MatchHistory;
    
    UPROPERTY(BlueprintReadWrite, Category = "User Profile")
    bool bIsValid = false;
};

// Login/Signup Response
USTRUCT(BlueprintType)
struct FAuthResponse
{
    GENERATED_BODY()
    
    UPROPERTY(BlueprintReadWrite, Category = "Auth")
    bool bSuccess = false;
    
    UPROPERTY(BlueprintReadWrite, Category = "Auth")
    FString Token;
    
    UPROPERTY(BlueprintReadWrite, Category = "Auth")
    FUserProfile UserProfile;
    
    UPROPERTY(BlueprintReadWrite, Category = "Auth")
    FString ErrorMessage;
    
    UPROPERTY(BlueprintReadWrite, Category = "Auth")
    int32 ResponseCode = 0;
};

// Profile Update Response
USTRUCT(BlueprintType)
struct FProfileUpdateResponse
{
    GENERATED_BODY()
    
    UPROPERTY(BlueprintReadWrite, Category = "Profile")
    bool bSuccess = false;
    
    UPROPERTY(BlueprintReadWrite, Category = "Profile")
    FString Username;
    
    UPROPERTY(BlueprintReadWrite, Category = "Profile")
    FString AvatarUrl;
    
    UPROPERTY(BlueprintReadWrite, Category = "Profile")
    FString Bio;
    
    UPROPERTY(BlueprintReadWrite, Category = "Profile")
    FString ErrorMessage;
};

// Avatar Upload Response
USTRUCT(BlueprintType)
struct FAvatarUploadResponse
{
    GENERATED_BODY()
    
    UPROPERTY(BlueprintReadWrite, Category = "Profile")
    bool bSuccess = false;
    
    UPROPERTY(BlueprintReadWrite, Category = "Profile")
    FString AvatarUrl;
    
    UPROPERTY(BlueprintReadWrite, Category = "Profile")
    FString ErrorMessage;
};

USTRUCT(BlueprintType)
struct FStoreCharacterInfo
{
    GENERATED_BODY()

    // type of item returned by the backend ("character", "skin", etc.)
    UPROPERTY(BlueprintReadWrite, Category = "Store")
    FString ItemType = TEXT("");

    UPROPERTY(BlueprintReadWrite, Category = "Store")
    FString ItemId;

    UPROPERTY(BlueprintReadWrite, Category = "Store")
    FString Name;

    UPROPERTY(BlueprintReadWrite, Category = "Store")
    FString Description;

    UPROPERTY(BlueprintReadWrite, Category = "Store")
    int32 Price = 0;

    UPROPERTY(BlueprintReadWrite, Category = "Store")
    FString ImageUrl;

    UPROPERTY(BlueprintReadWrite, Category = "Store")
    bool bOwned = false;

    UPROPERTY(BlueprintReadWrite, Category = "Store")
    bool bCanAfford = false;
};

USTRUCT(BlueprintType)
struct FRemnantPackage
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Store")
    FString PackageId;

    UPROPERTY(BlueprintReadWrite, Category = "Store")
    FString Name;

    UPROPERTY(BlueprintReadWrite, Category = "Store")
    int32 RemnantAmount = 0;

    UPROPERTY(BlueprintReadWrite, Category = "Store")
    FString DisplayPrice;

    UPROPERTY(BlueprintReadWrite, Category = "Store")
    int32 SortOrder = 0;
};

USTRUCT(BlueprintType)
struct FCharacterPurchaseResponse
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Store")
    bool bSuccess = false;

    UPROPERTY(BlueprintReadWrite, Category = "Store")
    FString CharacterId;

    UPROPERTY(BlueprintReadWrite, Category = "Store")
    int32 NewRemnantCount = 0;

    UPROPERTY(BlueprintReadWrite, Category = "Store")
    FString ErrorMessage;
};

USTRUCT(BlueprintType)
struct FRemnantPurchaseResponse
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Store")
    bool bSuccess = false;

    UPROPERTY(BlueprintReadWrite, Category = "Store")
    int32 RemnantsAdded = 0;

    UPROPERTY(BlueprintReadWrite, Category = "Store")
    int32 NewRemnantCount = 0;

    UPROPERTY(BlueprintReadWrite, Category = "Store")
    FString ReceiptId;

    UPROPERTY(BlueprintReadWrite, Category = "Store")
    FString ErrorMessage;
};

USTRUCT(BlueprintType)
struct FMatchRewardResponse
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Store")
    bool bSuccess = false;

    UPROPERTY(BlueprintReadWrite, Category = "Store")
    int32 RewardAmount = 0;

    UPROPERTY(BlueprintReadWrite, Category = "Store")
    int32 NewRemnantCount = 0;

    UPROPERTY(BlueprintReadWrite, Category = "Store")
    bool bIsWinner = false;

    UPROPERTY(BlueprintReadWrite, Category = "Store")
    FString ErrorMessage;
};

USTRUCT(BlueprintType)
struct FMatchParticipantPayload
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Match")
    FString UserId;

    UPROPERTY(BlueprintReadWrite, Category = "Match")
    FString PlayerName;

    UPROPERTY(BlueprintReadWrite, Category = "Match")
    int32 PlayerId = 0;

    UPROPERTY(BlueprintReadWrite, Category = "Match")
    FString CharacterId;

    UPROPERTY(BlueprintReadWrite, Category = "Match")
    int32 Placement = 0;

    UPROPERTY(BlueprintReadWrite, Category = "Match")
    int32 EliminationOrder = 0;

    UPROPERTY(BlueprintReadWrite, Category = "Match")
    float SurvivalTimeSeconds = 0.0f;

    UPROPERTY(BlueprintReadWrite, Category = "Match")
    bool bIsWinner = false;

    UPROPERTY(BlueprintReadWrite, Category = "Match")
    bool bIsAliveAtEnd = false;

    UPROPERTY(BlueprintReadWrite, Category = "Match")
    bool bDisconnected = false;

    UPROPERTY(BlueprintReadWrite, Category = "Match")
    FString DisconnectReason;

    UPROPERTY(BlueprintReadWrite, Category = "Match")
    int32 KillCount = 0;

    UPROPERTY(BlueprintReadWrite, Category = "Match")
    int32 DeathCount = 0;

    UPROPERTY(BlueprintReadWrite, Category = "Match")
    float DamageDealt = 0.0f;

    UPROPERTY(BlueprintReadWrite, Category = "Match")
    float DamageTaken = 0.0f;
};

USTRUCT(BlueprintType)
struct FMatchCompleteRequest
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Match")
    FString MatchId;

    UPROPERTY(BlueprintReadWrite, Category = "Match")
    FString MapName;

    UPROPERTY(BlueprintReadWrite, Category = "Match")
    FString GameMode;

    UPROPERTY(BlueprintReadWrite, Category = "Match")
    FString StartedAt;

    UPROPERTY(BlueprintReadWrite, Category = "Match")
    FString EndedAt;

    UPROPERTY(BlueprintReadWrite, Category = "Match")
    int32 DurationSeconds = 0;

    UPROPERTY(BlueprintReadWrite, Category = "Match")
    int32 ExpectedPlayerCount = 0;

    UPROPERTY(BlueprintReadWrite, Category = "Match")
    TArray<FMatchParticipantPayload> Participants;
};

USTRUCT(BlueprintType)
struct FMatchCompleteResponse
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Match")
    bool bSuccess = false;

    UPROPERTY(BlueprintReadWrite, Category = "Match")
    FString MatchId;

    UPROPERTY(BlueprintReadWrite, Category = "Match")
    bool bIdempotentReplay = false;

    UPROPERTY(BlueprintReadWrite, Category = "Match")
    int32 ParticipantsSaved = 0;

    UPROPERTY(BlueprintReadWrite, Category = "Match")
    int32 RewardsProcessed = 0;

    UPROPERTY(BlueprintReadWrite, Category = "Match")
    int32 MyRewardAmount = 0;

    UPROPERTY(BlueprintReadWrite, Category = "Match")
    int32 MyNewRemnantCount = 0;

    UPROPERTY(BlueprintReadWrite, Category = "Match")
    bool bMyIsWinner = false;

    UPROPERTY(BlueprintReadWrite, Category = "Match")
    FString ErrorMessage;
};

// Callback types
DECLARE_DELEGATE_OneParam(FOnAuthResponse, const FAuthResponse&);
DECLARE_DELEGATE_OneParam(FOnProfileResponse, const FUserProfile&);
DECLARE_DELEGATE_OneParam(FOnSimpleResponse, bool);
DECLARE_DELEGATE_OneParam(FOnProfileUpdateResponse, const FProfileUpdateResponse&);
DECLARE_DELEGATE_OneParam(FOnAvatarUploadResponse, const FAvatarUploadResponse&);
DECLARE_DELEGATE_OneParam(FOnStoreCharactersResponse, const TArray<FStoreCharacterInfo>&);
DECLARE_DELEGATE_OneParam(FOnRemnantPackagesResponse, const TArray<FRemnantPackage>&);
DECLARE_DELEGATE_OneParam(FOnCharacterPurchaseResponse, const FCharacterPurchaseResponse&);
DECLARE_DELEGATE_OneParam(FOnRemnantPurchaseResponse, const FRemnantPurchaseResponse&);
DECLARE_DELEGATE_OneParam(FOnMatchRewardResponse, const FMatchRewardResponse&);
DECLARE_DELEGATE_OneParam(FOnMatchCompleteResponse, const FMatchCompleteResponse&);

UCLASS()
class REMNANTBORN_API UEdsHttpService : public UObject
{
    GENERATED_BODY()
    
public:
    UEdsHttpService();
    
    // Initialize the service
    void Initialize(const FString& BaseUrl);
    
    // === Authentication Methods ===
    void Login(const FString& Email, const FString& Password, FOnAuthResponse Callback);
    void Signup(const FString& Email, const FString& Password, const FString& Username, FOnAuthResponse Callback);
    void DevLogin(const FString& Email, FOnAuthResponse Callback);
    void VerifyToken(const FString& Token, FOnAuthResponse Callback);
    
    // === Profile Methods ===
    void GetProfile(const FString& UserId, const FString& AuthToken, FOnProfileResponse Callback);
    void GetMyProfile(const FString& AuthToken, FOnProfileResponse Callback);
    void UpdateProfile(const FString& UserId, const FString& AuthToken, const FString& Username, const FString& Bio, FOnProfileUpdateResponse Callback);
    void UpdateProfileWithAvatar(const FString& UserId, const FString& AuthToken, const FString& Username, const FString& Bio, const FString& AvatarUrl, FOnProfileUpdateResponse Callback);
    void UploadAvatar(const FString& AuthToken, const FString& FilePath, FOnAvatarUploadResponse Callback);
    void UpdateGameStats(const FString& UserId, const FString& AuthToken, int32 Level, int32 RemnantCount, const FString& Operation, FOnSimpleResponse Callback);

    // === Store Methods ===
    void GetStoreCharacters(const FString& AuthToken, FOnStoreCharactersResponse Callback);
    void GetRemnantPackages(const FString& AuthToken, FOnRemnantPackagesResponse Callback);
    void BuyCharacter(const FString& AuthToken, const FString& CharacterId, FOnCharacterPurchaseResponse Callback);
    void BuyRemnants(const FString& AuthToken, const FString& PackageId, const FString& CardNumber, const FString& CardExpiry, const FString& CardCVV, FOnRemnantPurchaseResponse Callback);
    void SubmitMatchReward(const FString& AuthToken, bool bIsWinner, float MatchDuration, int32 EliminationOrder, const FString& MatchId, FOnMatchRewardResponse Callback);
    void SubmitMatchComplete(const FString& AuthToken, const FMatchCompleteRequest& MatchRequest, FOnMatchCompleteResponse Callback);
    
    // === Local Storage ===
    bool LoadSavedAuth(FString& OutToken, FString& OutUserId);
    void SaveAuth(const FString& Token, const FString& UserId);
    void ClearAuth();
    
private:
    // HTTP Methods
    void SendRequest(const FString& Endpoint, const FString& Verb, const TSharedPtr<FJsonObject>& JsonBody, 
                     TFunction<void(const FHttpResponsePtr&, bool)> Callback);
    void SendRequestWithAuth(const FString& Endpoint, const FString& Verb, const TSharedPtr<FJsonObject>& JsonBody,
                            const FString& AuthToken, TFunction<void(const FHttpResponsePtr&, bool)> Callback);
    void SendMultipartRequest(const FString& Endpoint, const FString& FilePath, const FString& FieldName,
                             const FString& AuthToken, TFunction<void(const FHttpResponsePtr&, bool)> Callback);
    
    // Response Handlers
    void HandleAuthResponse(const FHttpResponsePtr& Response, bool bSuccess, FOnAuthResponse Callback);
    void HandleProfileResponse(const FHttpResponsePtr& Response, bool bSuccess, FOnProfileResponse Callback);
    void HandleSimpleResponse(const FHttpResponsePtr& Response, bool bSuccess, FOnSimpleResponse Callback);
    void HandleProfileUpdateResponse(const FHttpResponsePtr& Response, bool bSuccess, FOnProfileUpdateResponse Callback);
    void HandleAvatarUploadResponse(const FHttpResponsePtr& Response, bool bSuccess, FOnAvatarUploadResponse Callback);
    
    // JSON Parsing
    FAuthResponse ParseAuthResponse(const FString& JsonString);
    FUserProfile ParseUserProfile(const TSharedPtr<FJsonObject>& JsonObject);
    FUserProfile ParseUserProfileFromResponse(const FString& JsonString);
    
private:
    FString BaseUrl;
    static const FString TOKEN_KEY;
    static const FString USER_ID_KEY;
};
