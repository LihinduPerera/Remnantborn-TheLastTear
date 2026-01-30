#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "HttpManager.generated.h"

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
};

// Delegate declarations
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAuthComplete, const FAuthResponse&, AuthResponse);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProfileLoaded, const FUserProfile&, UserProfile);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnApiError, const FString&, ErrorMessage);

UCLASS(BlueprintType)
class ELECTRICDREAMSSAMPLE_API UHttpManager : public UObject
{
    GENERATED_BODY()
    
public:
    UHttpManager();
    
    // Configuration
    UFUNCTION(BlueprintCallable, Category = "HTTP Manager")
    void Initialize(const FString& InBaseUrl);
    
    // Authentication methods
    UFUNCTION(BlueprintCallable, Category = "Authentication")
    void Login(const FString& Email, const FString& Password);
    
    UFUNCTION(BlueprintCallable, Category = "Authentication")
    void Signup(const FString& Email, const FString& Password, const FString& Username);
    
    UFUNCTION(BlueprintCallable, Category = "Authentication")
    void VerifyToken(const FString& Token);
    
    UFUNCTION(BlueprintCallable, Category = "Authentication")
    void DevLogin(const FString& Email);
    
    UFUNCTION(BlueprintCallable, Category = "Authentication")
    void Logout();
    
    // Profile methods
    UFUNCTION(BlueprintCallable, Category = "Profile")
    void GetProfile(const FString& UserId);
    
    UFUNCTION(BlueprintCallable, Category = "Profile")
    void UpdateProfile(const FString& UserId, const FString& Username, const FString& Bio);
    
    // Game stats
    UFUNCTION(BlueprintCallable, Category = "Profile")
    void UpdateGameStats(const FString& UserId, int32 Level, int32 RemnantCount, const FString& Operation = "set");
    
    // Getters
    UFUNCTION(BlueprintPure, Category = "Authentication")
    bool IsLoggedIn() const { return bIsLoggedIn; }
    
    UFUNCTION(BlueprintPure, Category = "Authentication")
    FString GetAuthToken() const { return AuthToken; }
    
    UFUNCTION(BlueprintPure, Category = "Authentication")
    FUserProfile GetCurrentProfile() const { return CurrentProfile; }
    
    // Save/Load token from local storage
    UFUNCTION(BlueprintCallable, Category = "Authentication")
    bool LoadSavedAuth();
    
    UFUNCTION(BlueprintCallable, Category = "Authentication")
    void SaveAuth(const FString& Token, const FString& UserId);
    
    // Events
    UPROPERTY(BlueprintAssignable, Category = "Authentication|Events")
    FOnAuthComplete OnLoginComplete;
    
    UPROPERTY(BlueprintAssignable, Category = "Authentication|Events")
    FOnAuthComplete OnSignupComplete;
    
    UPROPERTY(BlueprintAssignable, Category = "Authentication|Events")
    FOnAuthComplete OnTokenVerified;
    
    UPROPERTY(BlueprintAssignable, Category = "Profile|Events")
    FOnProfileLoaded OnProfileLoaded;
    
    UPROPERTY(BlueprintAssignable, Category = "HTTP Manager|Events")
    FOnApiError OnApiError;
    
protected:
    // HTTP Request helpers
    void SendRequest(const FString& Endpoint, const FString& Verb, const FString& Content, 
                     TFunction<void(FHttpRequestPtr, FHttpResponsePtr, bool)> Callback);
    
    void SendRequestWithAuth(const FString& Endpoint, const FString& Verb, const FString& Content,
                            TFunction<void(FHttpRequestPtr, FHttpResponsePtr, bool)> Callback);
    
    // Response handlers
    void HandleLoginResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    void HandleSignupResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    void HandleTokenVerifyResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    void HandleProfileResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    
    // JSON parsing helpers
    FAuthResponse ParseAuthResponse(const FString& JsonString);
    FUserProfile ParseUserProfile(const TSharedPtr<FJsonObject>& JsonObject);
    
private:
    FString BaseUrl;
    FString AuthToken;
    FString CurrentUserId;
    FUserProfile CurrentProfile;
    bool bIsLoggedIn = false;
    
    // Local storage keys
    static const FString TOKEN_KEY;
    static const FString USER_ID_KEY;
};