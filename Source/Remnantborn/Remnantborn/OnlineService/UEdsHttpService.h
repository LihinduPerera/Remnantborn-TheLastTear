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

// Callback types
DECLARE_DELEGATE_OneParam(FOnAuthResponse, const FAuthResponse&);
DECLARE_DELEGATE_OneParam(FOnProfileResponse, const FUserProfile&);
DECLARE_DELEGATE_OneParam(FOnSimpleResponse, bool);

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
    void UpdateProfile(const FString& UserId, const FString& AuthToken, const FString& Username, const FString& Bio, FOnSimpleResponse Callback);
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
    
    // Response Handlers
    void HandleAuthResponse(const FHttpResponsePtr& Response, bool bSuccess, FOnAuthResponse Callback);
    void HandleProfileResponse(const FHttpResponsePtr& Response, bool bSuccess, FOnProfileResponse Callback);
    void HandleSimpleResponse(const FHttpResponsePtr& Response, bool bSuccess, FOnSimpleResponse Callback);
    
    // JSON Parsing
    FAuthResponse ParseAuthResponse(const FString& JsonString);
    FUserProfile ParseUserProfile(const TSharedPtr<FJsonObject>& JsonObject);
    
private:
    FString BaseUrl;
    static const FString TOKEN_KEY;
    static const FString USER_ID_KEY;
};
