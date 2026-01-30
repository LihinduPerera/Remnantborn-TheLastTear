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
    
    FString ResponseContent = Response->GetContentAsString();
    UE_LOG(LogTemp, Log, TEXT("Login Response: %s"), *ResponseContent);
    UE_LOG(LogTemp, Log, TEXT("Response Code: %d"), Response->GetResponseCode());
    
    if (Response->GetResponseCode() == 200 || Response->GetResponseCode() == 201)
    {
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
            
            UE_LOG(LogTemp, Log, TEXT("Login successful for user: %s, Token: %s"), 
                *CurrentProfile.Username, *AuthToken);
            
            // Trigger profile load after successful login
            GetProfile(CurrentUserId);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Login failed: %s"), *AuthResponse.ErrorMessage);
        }
    }
    else
    {
        AuthResponse.bSuccess = false;
        AuthResponse.ErrorMessage = FString::Printf(TEXT("Login failed (Code: %d)"), Response->GetResponseCode());
        
        // Try to parse error message
        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);
        if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
        {
            FString Message;
            if (JsonObject->TryGetStringField(TEXT("message"), Message))
            {
                AuthResponse.ErrorMessage = Message;
            }
        }
        
        UE_LOG(LogTemp, Error, TEXT("Login error: %s"), *AuthResponse.ErrorMessage);
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
    
    FString ResponseContent = Response->GetContentAsString();
    UE_LOG(LogTemp, Log, TEXT("Profile Response: %s"), *ResponseContent);
    UE_LOG(LogTemp, Log, TEXT("Profile Response Code: %d"), Response->GetResponseCode());
    
    if (Response->GetResponseCode() == 200)
    {
        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);
        
        if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
        {
            // Log for debugging
            FString OutputString;
            TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
            FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
            UE_LOG(LogTemp, Log, TEXT("Profile JSON: %s"), *OutputString);
            
            // Try to parse data field
            const TSharedPtr<FJsonObject>* DataObjectPtr = nullptr;
            if (JsonObject->TryGetObjectField(TEXT("data"), DataObjectPtr) && DataObjectPtr != nullptr)
            {
                CurrentProfile = ParseUserProfile(*DataObjectPtr);
                OnProfileLoaded.Broadcast(CurrentProfile);
                
                UE_LOG(LogTemp, Log, TEXT("Profile loaded from data field for user: %s (Level: %d, Remnants: %d)"),
                       *CurrentProfile.Username, CurrentProfile.Level, CurrentProfile.RemnantCount);
            }
            else
            {
                // If no "data" field, try to parse the root object
                CurrentProfile = ParseUserProfile(JsonObject);
                OnProfileLoaded.Broadcast(CurrentProfile);
                
                UE_LOG(LogTemp, Log, TEXT("Profile loaded from root object for user: %s (Level: %d, Remnants: %d)"),
                       *CurrentProfile.Username, CurrentProfile.Level, CurrentProfile.RemnantCount);
            }
        }
        else
        {
            OnApiError.Broadcast(TEXT("Failed to parse profile data"));
            UE_LOG(LogTemp, Error, TEXT("Failed to parse profile JSON"));
        }
    }
    else
    {
        FString ErrorMsg = FString::Printf(TEXT("Failed to load profile (Code: %d)"), Response->GetResponseCode());
        OnApiError.Broadcast(ErrorMsg);
        UE_LOG(LogTemp, Error, TEXT("%s"), *ErrorMsg);
    }
}

FAuthResponse UHttpManager::ParseAuthResponse(const FString& JsonString)
{
    FAuthResponse AuthResponse;
    
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
    {
        // Log the entire JSON for debugging
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
        UE_LOG(LogTemp, Log, TEXT("Parsing JSON: %s"), *OutputString);
        
        // Check for success field
        bool bSuccess = false;
        if (JsonObject->TryGetBoolField(TEXT("success"), bSuccess))
        {
            AuthResponse.bSuccess = bSuccess;
        }
        else
        {
            // If no success field, assume successful for 200/201 responses
            AuthResponse.bSuccess = true;
        }
        
        // Try to get token from root or data object
        const TSharedPtr<FJsonObject>* DataObjectPtr = nullptr;
        if (JsonObject->TryGetObjectField(TEXT("data"), DataObjectPtr) && DataObjectPtr != nullptr)
        {
            const TSharedPtr<FJsonObject>& DataObject = *DataObjectPtr;
            
            // Get token
            if (DataObject->TryGetStringField(TEXT("token"), AuthResponse.Token))
            {
                UE_LOG(LogTemp, Log, TEXT("Found token in data object"));
            }
            
            // Parse user profile from data object
            AuthResponse.UserProfile = ParseUserProfile(DataObject);
        }
        else
        {
            // If no data field, try to get token and profile from root
            JsonObject->TryGetStringField(TEXT("token"), AuthResponse.Token);
            AuthResponse.UserProfile = ParseUserProfile(JsonObject);
        }
        
        // If still no token, check if it's in the user profile data
        if (AuthResponse.Token.IsEmpty())
        {
            const TSharedPtr<FJsonObject>* DataObjectPtr2 = nullptr;
            if (JsonObject->TryGetObjectField(TEXT("data"), DataObjectPtr2) && DataObjectPtr2 != nullptr)
            {
                (*DataObjectPtr2)->TryGetStringField(TEXT("token"), AuthResponse.Token);
            }
        }
        
        // Get error message if any
        if (!AuthResponse.bSuccess || AuthResponse.Token.IsEmpty())
        {
            FString Message;
            if (JsonObject->TryGetStringField(TEXT("message"), Message))
            {
                AuthResponse.ErrorMessage = Message;
            }
            else
            {
                AuthResponse.ErrorMessage = TEXT("Authentication failed");
            }
            
            // If we have no token, it's not a successful auth
            if (AuthResponse.Token.IsEmpty())
            {
                AuthResponse.bSuccess = false;
            }
        }
        
        UE_LOG(LogTemp, Log, TEXT("Parse result: Success=%s, Token=%s, Error=%s"), 
            AuthResponse.bSuccess ? TEXT("true") : TEXT("false"),
            *AuthResponse.Token,
            *AuthResponse.ErrorMessage);
    }
    else
    {
        AuthResponse.bSuccess = false;
        AuthResponse.ErrorMessage = TEXT("Failed to parse response");
        UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON response"));
    }
    
    return AuthResponse;
}


FUserProfile UHttpManager::ParseUserProfile(const TSharedPtr<FJsonObject>& JsonObject)
{
    FUserProfile Profile;
    
    // Try different field names for user ID
    if (!JsonObject->TryGetStringField(TEXT("userId"), Profile.UserId))
    {
        JsonObject->TryGetStringField(TEXT("user_id"), Profile.UserId);
    }
    
    // Try different field names for username
    if (!JsonObject->TryGetStringField(TEXT("username"), Profile.Username))
    {
        FString TempUsername;
        if (JsonObject->TryGetStringField(TEXT("userName"), TempUsername))
        {
            Profile.Username = TempUsername;
        }
    }
    
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
    else if (JsonObject->TryGetNumberField(TEXT("remnantCount"), RemnantCount))
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
                UE_LOG(LogTemp, Log, TEXT("Found saved auth: UserId=%s, Token=%s"), 
                    *SavedUserId, *SavedToken);
                
                // Store and verify the token
                AuthToken = SavedToken;
                CurrentUserId = SavedUserId;
                
                // Verify the token immediately
                VerifyToken(SavedToken);
                return true;
            }
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("No saved auth found"));
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