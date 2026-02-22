// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "UEdsHttpService.generated.h"

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

// Callback types
DECLARE_DELEGATE_OneParam(FOnAuthResponse, const FAuthResponse&);
DECLARE_DELEGATE_OneParam(FOnProfileResponse, const FUserProfile&);
DECLARE_DELEGATE_OneParam(FOnSimpleResponse, bool);
DECLARE_DELEGATE_OneParam(FOnProfileUpdateResponse, const FProfileUpdateResponse&);
DECLARE_DELEGATE_OneParam(FOnAvatarUploadResponse, const FAvatarUploadResponse&);

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
