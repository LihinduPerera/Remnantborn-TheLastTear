#include "HttpManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"

const FString UHttpManager::TOKEN_KEY = TEXT("AuthToken");
const FString UHttpManager::USER_ID_KEY = TEXT("UserId");

UHttpManager::UHttpManager()
{
    // Default to localhost for development
    BaseUrl = TEXT("http://localhost:3000/api");
}

void UHttpManager::Initialize(const FString& InBaseUrl)
{
    if (!InBaseUrl.IsEmpty())
    {
        BaseUrl = InBaseUrl;
    }
    
    UE_LOG(LogTemp, Log, TEXT("HTTP Manager initialized with base URL: %s"), *BaseUrl);
}

void UHttpManager::SendRequest(const FString& Endpoint, const FString& Verb, const FString& Content,
                              TFunction<void(FHttpRequestPtr, FHttpResponsePtr, bool)> Callback)
{
    FHttpModule& HttpModule = FHttpModule::Get();
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = HttpModule.CreateRequest();
    
    FString FullUrl = BaseUrl + Endpoint;
    Request->SetURL(FullUrl);
    Request->SetVerb(Verb);
    
    if (!Content.IsEmpty())
    {
        Request->SetContentAsString(Content);
        Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    }
    
    Request->SetHeader(TEXT("Accept"), TEXT("application/json"));
    Request->OnProcessRequestComplete().BindLambda(Callback);
    
    UE_LOG(LogTemp, Verbose, TEXT("Sending %s request to: %s"), *Verb, *FullUrl);
    
    if (!Request->ProcessRequest())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to process HTTP request to: %s"), *FullUrl);
        OnApiError.Broadcast(TEXT("Failed to process HTTP request"));
    }
}

void UHttpManager::SendRequestWithAuth(const FString& Endpoint, const FString& Verb, const FString& Content,
                                      TFunction<void(FHttpRequestPtr, FHttpResponsePtr, bool)> Callback)
{
    // Allow requests even if bIsLoggedIn is false but AuthToken exists
    // This is for token verification during LoadSavedAuth
    if (AuthToken.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("Attempted to send authenticated request without valid token"));
        OnApiError.Broadcast(TEXT("Not authenticated"));
        return;
    }
    
    FHttpModule& HttpModule = FHttpModule::Get();
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = HttpModule.CreateRequest();
    
    FString FullUrl = BaseUrl + Endpoint;
    Request->SetURL(FullUrl);
    Request->SetVerb(Verb);
    Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *AuthToken));
    
    if (!Content.IsEmpty())
    {
        Request->SetContentAsString(Content);
        Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    }
    
    Request->SetHeader(TEXT("Accept"), TEXT("application/json"));
    Request->OnProcessRequestComplete().BindLambda(Callback);
    
    UE_LOG(LogTemp, Verbose, TEXT("Sending authenticated %s request to: %s"), *Verb, *FullUrl);
    
    if (!Request->ProcessRequest())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to process authenticated HTTP request"));
        OnApiError.Broadcast(TEXT("Failed to process authenticated request"));
    }
}

void UHttpManager::Login(const FString& Email, const FString& Password)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    JsonObject->SetStringField(TEXT("email"), Email);
    JsonObject->SetStringField(TEXT("password"), Password);
    
    FString Content;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Content);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    
    SendRequest(TEXT("/auth/login"), TEXT("POST"), Content,
                [this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
                {
                    HandleLoginResponse(Request, Response, bWasSuccessful);
                });
}

void UHttpManager::Signup(const FString& Email, const FString& Password, const FString& Username)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    JsonObject->SetStringField(TEXT("email"), Email);
    JsonObject->SetStringField(TEXT("password"), Password);
    JsonObject->SetStringField(TEXT("username"), Username);
    
    FString Content;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Content);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    
    SendRequest(TEXT("/auth/signup"), TEXT("POST"), Content,
                [this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
                {
                    HandleSignupResponse(Request, Response, bWasSuccessful);
                });
}

void UHttpManager::VerifyToken(const FString& Token)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    FString Content;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Content);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    
    // Store token temporarily for this request
    FString TempToken = AuthToken;
    AuthToken = Token;
    
    SendRequestWithAuth(TEXT("/auth/verify-token"), TEXT("POST"), Content,
                       [this, TempToken](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
                       {
                           HandleTokenVerifyResponse(Request, Response, bWasSuccessful);
                           AuthToken = TempToken; // Restore original token
                       });
}

void UHttpManager::DevLogin(const FString& Email)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    JsonObject->SetStringField(TEXT("email"), Email);
    
    FString Content;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Content);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    
    SendRequest(TEXT("/auth/dev-login"), TEXT("POST"), Content,
                [this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
                {
                    HandleLoginResponse(Request, Response, bWasSuccessful);
                });
}

void UHttpManager::Logout()
{
    AuthToken.Empty();
    CurrentUserId.Empty();
    bIsLoggedIn = false;
    
    // Clear saved auth
    if (GConfig)
    {
        GConfig->RemoveKey(TEXT("Auth"), *TOKEN_KEY, GGameIni);
        GConfig->RemoveKey(TEXT("Auth"), *USER_ID_KEY, GGameIni);
        GConfig->Flush(false, GGameIni);
    }
    
    UE_LOG(LogTemp, Log, TEXT("User logged out"));
}

void UHttpManager::GetProfile(const FString& UserId)
{
    SendRequestWithAuth(FString::Printf(TEXT("/profile/%s"), *UserId), TEXT("GET"), TEXT(""),
                       [this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
                       {
                           HandleProfileResponse(Request, Response, bWasSuccessful);
                       });
}

void UHttpManager::UpdateProfile(const FString& UserId, const FString& Username, const FString& Bio)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    if (!Username.IsEmpty()) JsonObject->SetStringField(TEXT("username"), Username);
    if (!Bio.IsEmpty()) JsonObject->SetStringField(TEXT("bio"), Bio);
    
    FString Content;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Content);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    
    SendRequestWithAuth(FString::Printf(TEXT("/profile/%s"), *UserId), TEXT("PUT"), Content,
                       [this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
                       {
                           if (bWasSuccessful && Response.IsValid())
                           {
                               if (Response->GetResponseCode() == 200)
                               {
                                   UE_LOG(LogTemp, Log, TEXT("Profile updated successfully"));
                                   // Refresh profile
                                   GetProfile(CurrentProfile.UserId);
                               }
                           }
                       });
}

void UHttpManager::UpdateGameStats(const FString& UserId, int32 Level, int32 RemnantCount, const FString& Operation)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    
    if (Level >= 0) JsonObject->SetNumberField(TEXT("level"), Level);
    if (RemnantCount >= 0) JsonObject->SetNumberField(TEXT("remnant_count"), RemnantCount);
    JsonObject->SetStringField(TEXT("operation"), Operation);
    
    FString Content;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Content);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    
    SendRequestWithAuth(FString::Printf(TEXT("/profile/%s/game-stats"), *UserId), TEXT("PATCH"), Content,
                       [this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
                       {
                           if (bWasSuccessful && Response.IsValid())
                           {
                               if (Response->GetResponseCode() == 200)
                               {
                                   UE_LOG(LogTemp, Log, TEXT("Game stats updated successfully"));
                                   // Refresh profile
                                   GetProfile(CurrentProfile.UserId);
                               }
                           }
                       });
}

void UHttpManager::HandleLoginResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    FAuthResponse AuthResponse;
    
    if (!bWasSuccessful || !Response.IsValid())
    {
        AuthResponse.bSuccess = false;
        AuthResponse.ErrorMessage = TEXT("Network error");
        OnLoginComplete.Broadcast(AuthResponse);
        return;
    }
    
    if (Response->GetResponseCode() == 200)
    {
        FString ResponseContent = Response->GetContentAsString();
        AuthResponse = ParseAuthResponse(ResponseContent);
        
        if (AuthResponse.bSuccess)
        {
            // Store auth data
            AuthToken = AuthResponse.Token;
            CurrentUserId = AuthResponse.UserProfile.UserId;
            CurrentProfile = AuthResponse.UserProfile;
            bIsLoggedIn = true;
            
            // Save to config
            SaveAuth(AuthToken, CurrentUserId);
            
            UE_LOG(LogTemp, Log, TEXT("Login successful for user: %s"), *CurrentProfile.Username);
        }
    }
    else
    {
        AuthResponse.bSuccess = false;
        AuthResponse.ErrorMessage = FString::Printf(TEXT("Login failed (Code: %d)"), Response->GetResponseCode());
        
        // Try to parse error message
        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
        if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
        {
            FString Message;
            if (JsonObject->TryGetStringField(TEXT("message"), Message))
            {
                AuthResponse.ErrorMessage = Message;
            }
        }
    }
    
    OnLoginComplete.Broadcast(AuthResponse);
}

void UHttpManager::HandleSignupResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    FAuthResponse AuthResponse;
    
    if (!bWasSuccessful || !Response.IsValid())
    {
        AuthResponse.bSuccess = false;
        AuthResponse.ErrorMessage = TEXT("Network error");
        OnSignupComplete.Broadcast(AuthResponse);
        return;
    }
    
    if (Response->GetResponseCode() == 201 || Response->GetResponseCode() == 200)
    {
        FString ResponseContent = Response->GetContentAsString();
        AuthResponse = ParseAuthResponse(ResponseContent);
        
        if (AuthResponse.bSuccess)
        {
            // Store auth data
            AuthToken = AuthResponse.Token;
            CurrentUserId = AuthResponse.UserProfile.UserId;
            CurrentProfile = AuthResponse.UserProfile;
            bIsLoggedIn = true;
            
            // Save to config
            SaveAuth(AuthToken, CurrentUserId);
            
            UE_LOG(LogTemp, Log, TEXT("Signup successful for user: %s"), *CurrentProfile.Username);
        }
    }
    else
    {
        AuthResponse.bSuccess = false;
        AuthResponse.ErrorMessage = FString::Printf(TEXT("Signup failed (Code: %d)"), Response->GetResponseCode());
        
        // Try to parse error message
        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
        if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
        {
            FString Message;
            if (JsonObject->TryGetStringField(TEXT("message"), Message))
            {
                AuthResponse.ErrorMessage = Message;
            }
        }
    }
    
    OnSignupComplete.Broadcast(AuthResponse);
}

void UHttpManager::HandleTokenVerifyResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    FAuthResponse AuthResponse;
    
    if (!bWasSuccessful || !Response.IsValid())
    {
        AuthResponse.bSuccess = false;
        AuthResponse.ErrorMessage = TEXT("Network error");
        OnTokenVerified.Broadcast(AuthResponse);
        return;
    }
    
    if (Response->GetResponseCode() == 200)
    {
        FString ResponseContent = Response->GetContentAsString();
        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);
        
        if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
        {
            AuthResponse.bSuccess = true;
            AuthResponse.UserProfile = ParseUserProfile(JsonObject);
            
            // Update current profile
            CurrentProfile = AuthResponse.UserProfile;
            bIsLoggedIn = true;
            
            UE_LOG(LogTemp, Log, TEXT("Token verified for user: %s"), *CurrentProfile.Username);
        }
        else
        {
            AuthResponse.bSuccess = false;
            AuthResponse.ErrorMessage = TEXT("Failed to parse response");
        }
    }
    else
    {
        AuthResponse.bSuccess = false;
        AuthResponse.ErrorMessage = FString::Printf(TEXT("Token verification failed (Code: %d)"), Response->GetResponseCode());
    }
    
    OnTokenVerified.Broadcast(AuthResponse);
}

void UHttpManager::HandleProfileResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        OnApiError.Broadcast(TEXT("Failed to load profile"));
        return;
    }
    
    if (Response->GetResponseCode() == 200)
    {
        FString ResponseContent = Response->GetContentAsString();
        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);
        
        if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
        {
            // FIXED: Parse data object from the response
            const TSharedPtr<FJsonObject>* DataObjectPtr = nullptr;
            if (JsonObject->TryGetObjectField(TEXT("data"), DataObjectPtr) && DataObjectPtr != nullptr)
            {
                CurrentProfile = ParseUserProfile(*DataObjectPtr);
                OnProfileLoaded.Broadcast(CurrentProfile);
                
                UE_LOG(LogTemp, Log, TEXT("Profile loaded for user: %s (Level: %d, Remnants: %d)"),
                       *CurrentProfile.Username, CurrentProfile.Level, CurrentProfile.RemnantCount);
            }
            else
            {
                // If no "data" field, try to parse the root object
                CurrentProfile = ParseUserProfile(JsonObject);
                OnProfileLoaded.Broadcast(CurrentProfile);
            }
        }
        else
        {
            OnApiError.Broadcast(TEXT("Failed to parse profile data"));
        }
    }
    else
    {
        OnApiError.Broadcast(FString::Printf(TEXT("Failed to load profile (Code: %d)"), Response->GetResponseCode()));
    }
}

FAuthResponse UHttpManager::ParseAuthResponse(const FString& JsonString)
{
    FAuthResponse AuthResponse;
    
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
    {
        bool bSuccess = false;
        if (JsonObject->TryGetBoolField(TEXT("success"), bSuccess))
        {
            AuthResponse.bSuccess = bSuccess;
        }
        
        FString Token;
        if (JsonObject->TryGetStringField(TEXT("token"), Token))
        {
            AuthResponse.Token = Token;
        }
        
        // FIXED: Use pointer to TSharedPtr for TryGetObjectField
        const TSharedPtr<FJsonObject>* DataObjectPtr = nullptr;
        if (JsonObject->TryGetObjectField(TEXT("data"), DataObjectPtr) && DataObjectPtr != nullptr)
        {
            // Dereference the pointer to get the actual object
            const TSharedPtr<FJsonObject>& DataObject = *DataObjectPtr;
            AuthResponse.UserProfile = ParseUserProfile(DataObject);
            
            // Also try to get token from data if not already set
            if (AuthResponse.Token.IsEmpty())
            {
                DataObject->TryGetStringField(TEXT("token"), AuthResponse.Token);
            }
        }
        
        // Get error message if any
        if (!AuthResponse.bSuccess)
        {
            JsonObject->TryGetStringField(TEXT("message"), AuthResponse.ErrorMessage);
        }
    }
    
    return AuthResponse;
}

FUserProfile UHttpManager::ParseUserProfile(const TSharedPtr<FJsonObject>& JsonObject)
{
    FUserProfile Profile;
    
    JsonObject->TryGetStringField(TEXT("userId"), Profile.UserId);
    JsonObject->TryGetStringField(TEXT("username"), Profile.Username);
    JsonObject->TryGetStringField(TEXT("email"), Profile.Email);
    
    int32 Level = 1;
    if (JsonObject->TryGetNumberField(TEXT("level"), Level))
    {
        Profile.Level = Level;
    }
    
    int32 RemnantCount = 100;
    if (JsonObject->TryGetNumberField(TEXT("remnant_count"), RemnantCount))
    {
        Profile.RemnantCount = RemnantCount;
    }
    
    JsonObject->TryGetStringField(TEXT("avatar_url"), Profile.AvatarUrl);
    JsonObject->TryGetStringField(TEXT("bio"), Profile.Bio);
    JsonObject->TryGetStringField(TEXT("last_active"), Profile.LastActive);
    
    return Profile;
}

bool UHttpManager::LoadSavedAuth()
{
    if (GConfig)
    {
        FString SavedToken;
        FString SavedUserId;
        
        if (GConfig->GetString(TEXT("Auth"), *TOKEN_KEY, SavedToken, GGameIni) &&
            GConfig->GetString(TEXT("Auth"), *USER_ID_KEY, SavedUserId, GGameIni))
        {
            if (!SavedToken.IsEmpty() && !SavedUserId.IsEmpty())
            {
                // Store the token temporarily
                AuthToken = SavedToken;
                CurrentUserId = SavedUserId;
                
                // Verify the token without setting bIsLoggedIn yet
                TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
                FString Content;
                TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Content);
                FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
                
                // Create a separate request for verification
                FHttpModule& HttpModule = FHttpModule::Get();
                TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = HttpModule.CreateRequest();
                
                Request->SetURL(BaseUrl + TEXT("/auth/verify-token"));
                Request->SetVerb(TEXT("POST"));
                Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *AuthToken));
                Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
                Request->SetHeader(TEXT("Accept"), TEXT("application/json"));
                
                Request->OnProcessRequestComplete().BindLambda([this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
                {
                    if (bWasSuccessful && Response.IsValid() && Response->GetResponseCode() == 200)
                    {
                        FString ResponseContent = Response->GetContentAsString();
                        TSharedPtr<FJsonObject> JsonObject;
                        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);
                        
                        if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
                        {
                            // Parse the data field
                            const TSharedPtr<FJsonObject>* DataObjectPtr = nullptr;
                            if (JsonObject->TryGetObjectField(TEXT("data"), DataObjectPtr) && DataObjectPtr != nullptr)
                            {
                                CurrentProfile = ParseUserProfile(*DataObjectPtr);
                                bIsLoggedIn = true;
                                UE_LOG(LogTemp, Log, TEXT("Auto-login successful for user: %s"), *CurrentProfile.Username);
                            }
                        }
                    }
                    else
                    {
                        // Clear invalid saved auth
                        AuthToken.Empty();
                        CurrentUserId.Empty();
                        if (GConfig)
                        {
                            GConfig->RemoveKey(TEXT("Auth"), *TOKEN_KEY, GGameIni);
                            GConfig->RemoveKey(TEXT("Auth"), *USER_ID_KEY, GGameIni);
                            GConfig->Flush(false, GGameIni);
                        }
                        UE_LOG(LogTemp, Warning, TEXT("Saved token verification failed"));
                    }
                });
                
                if (Request->ProcessRequest())
                {
                    return true;
                }
            }
        }
    }
    
    return false;
}

void UHttpManager::SaveAuth(const FString& Token, const FString& UserId)
{
    if (GConfig)
    {
        GConfig->SetString(TEXT("Auth"), *TOKEN_KEY, *Token, GGameIni);
        GConfig->SetString(TEXT("Auth"), *USER_ID_KEY, *UserId, GGameIni);
        GConfig->Flush(false, GGameIni);
        
        UE_LOG(LogTemp, Log, TEXT("Auth saved for user: %s"), *UserId);
    }
}