// Copyright Epic Games, Inc. All Rights Reserved.

#include "UEdsHttpService.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/ConfigCacheIni.h"
#include "Async/TaskGraphInterfaces.h"

const FString UEdsHttpService::TOKEN_KEY = TEXT("AuthToken");
const FString UEdsHttpService::USER_ID_KEY = TEXT("UserId");

UEdsHttpService::UEdsHttpService()
{
    BaseUrl = TEXT("http://localhost:3000/api");
}

void UEdsHttpService::Initialize(const FString& InBaseUrl)
{
    if (!InBaseUrl.IsEmpty())
    {
        BaseUrl = InBaseUrl;
    }
    
    UE_LOG(LogTemp, Log, TEXT("EDS HTTP Service initialized with URL: %s"), *BaseUrl);
}

void UEdsHttpService::Login(const FString& Email, const FString& Password, FOnAuthResponse Callback)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    JsonObject->SetStringField(TEXT("email"), Email);
    JsonObject->SetStringField(TEXT("password"), Password);
    
    SendRequest(TEXT("/auth/login"), TEXT("POST"), JsonObject,
        [this, Callback](const FHttpResponsePtr& Response, bool bSuccess)
        {
            HandleAuthResponse(Response, bSuccess, Callback);
        });
}

void UEdsHttpService::Signup(const FString& Email, const FString& Password, const FString& Username, FOnAuthResponse Callback)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    JsonObject->SetStringField(TEXT("email"), Email);
    JsonObject->SetStringField(TEXT("password"), Password);
    JsonObject->SetStringField(TEXT("username"), Username);
    
    SendRequest(TEXT("/auth/signup"), TEXT("POST"), JsonObject,
        [this, Callback](const FHttpResponsePtr& Response, bool bSuccess)
        {
            HandleAuthResponse(Response, bSuccess, Callback);
        });
}

void UEdsHttpService::DevLogin(const FString& Email, FOnAuthResponse Callback)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    JsonObject->SetStringField(TEXT("email"), Email);
    
    SendRequest(TEXT("/auth/dev-login"), TEXT("POST"), JsonObject,
        [this, Callback](const FHttpResponsePtr& Response, bool bSuccess)
        {
            HandleAuthResponse(Response, bSuccess, Callback);
        });
}

void UEdsHttpService::VerifyToken(const FString& Token, FOnAuthResponse Callback)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    
    SendRequestWithAuth(TEXT("/auth/verify-token"), TEXT("POST"), JsonObject, Token,
        [this, Callback](const FHttpResponsePtr& Response, bool bSuccess)
        {
            HandleAuthResponse(Response, bSuccess, Callback);
        });
}

void UEdsHttpService::GetProfile(const FString& UserId, const FString& AuthToken, FOnProfileResponse Callback)
{
    SendRequestWithAuth(FString::Printf(TEXT("/profile/%s"), *UserId), TEXT("GET"), nullptr, AuthToken,
        [this, Callback](const FHttpResponsePtr& Response, bool bSuccess)
        {
            HandleProfileResponse(Response, bSuccess, Callback);
        });
}

void UEdsHttpService::UpdateProfile(const FString& UserId, const FString& AuthToken, const FString& Username, const FString& Bio, FOnSimpleResponse Callback)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    if (!Username.IsEmpty()) JsonObject->SetStringField(TEXT("username"), Username);
    if (!Bio.IsEmpty()) JsonObject->SetStringField(TEXT("bio"), Bio);
    
    SendRequestWithAuth(FString::Printf(TEXT("/profile/%s"), *UserId), TEXT("PUT"), JsonObject, AuthToken,
        [this, Callback](const FHttpResponsePtr& Response, bool bSuccess)
        {
            HandleSimpleResponse(Response, bSuccess, Callback);
        });
}

void UEdsHttpService::UpdateGameStats(const FString& UserId, const FString& AuthToken, int32 Level, int32 RemnantCount, const FString& Operation, FOnSimpleResponse Callback)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    
    if (Level >= 0) JsonObject->SetNumberField(TEXT("level"), Level);
    if (RemnantCount >= 0) JsonObject->SetNumberField(TEXT("remnant_count"), RemnantCount);
    JsonObject->SetStringField(TEXT("operation"), Operation);
    
    SendRequestWithAuth(FString::Printf(TEXT("/profile/%s/game-stats"), *UserId), TEXT("PATCH"), JsonObject, AuthToken,
        [this, Callback](const FHttpResponsePtr& Response, bool bSuccess)
        {
            HandleSimpleResponse(Response, bSuccess, Callback);
        });
}

bool UEdsHttpService::LoadSavedAuth(FString& OutToken, FString& OutUserId)
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
                OutToken = SavedToken;
                OutUserId = SavedUserId;
                return true;
            }
        }
    }
    return false;
}

void UEdsHttpService::SaveAuth(const FString& Token, const FString& UserId)
{
    if (GConfig)
    {
        GConfig->SetString(TEXT("Auth"), *TOKEN_KEY, *Token, GGameIni);
        GConfig->SetString(TEXT("Auth"), *USER_ID_KEY, *UserId, GGameIni);
        GConfig->Flush(false, GGameIni);
    }
}

void UEdsHttpService::ClearAuth()
{
    if (GConfig)
    {
        GConfig->RemoveKey(TEXT("Auth"), *TOKEN_KEY, GGameIni);
        GConfig->RemoveKey(TEXT("Auth"), *USER_ID_KEY, GGameIni);
        GConfig->Flush(false, GGameIni);
    }
}

void UEdsHttpService::SendRequest(const FString& Endpoint, const FString& Verb, const TSharedPtr<FJsonObject>& JsonBody,
                                 TFunction<void(const FHttpResponsePtr&, bool)> Callback)
{
    FHttpModule& HttpModule = FHttpModule::Get();
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = HttpModule.CreateRequest();
    
    FString FullUrl = BaseUrl + Endpoint;
    Request->SetURL(FullUrl);
    Request->SetVerb(Verb);
    Request->SetTimeout(10); // 10 second timeout
    
    if (JsonBody.IsValid())
    {
        FString Content;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Content);
        FJsonSerializer::Serialize(JsonBody.ToSharedRef(), Writer);
        
        Request->SetContentAsString(Content);
        Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    }
    
    Request->SetHeader(TEXT("Accept"), TEXT("application/json"));
    
    Request->OnProcessRequestComplete().BindLambda([Callback](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
    {
        // Always execute callback on game thread
        if (Callback)
        {
            FFunctionGraphTask::CreateAndDispatchWhenReady([Response, bSuccess, Callback]()
            {
                Callback(Response, bSuccess);
            }, TStatId(), nullptr, ENamedThreads::GameThread);
        }
    });
    
    if (!Request->ProcessRequest())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to process HTTP request"));
        if (Callback)
        {
            FFunctionGraphTask::CreateAndDispatchWhenReady([Callback]()
            {
                Callback(nullptr, false);
            }, TStatId(), nullptr, ENamedThreads::GameThread);
        }
    }
}

void UEdsHttpService::SendRequestWithAuth(const FString& Endpoint, const FString& Verb, const TSharedPtr<FJsonObject>& JsonBody,
                                         const FString& AuthToken, TFunction<void(const FHttpResponsePtr&, bool)> Callback)
{
    if (AuthToken.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("Attempted authenticated request without token"));
        if (Callback)
        {
            FFunctionGraphTask::CreateAndDispatchWhenReady([Callback]()
            {
                Callback(nullptr, false);
            }, TStatId(), nullptr, ENamedThreads::GameThread);
        }
        return;
    }
    
    FHttpModule& HttpModule = FHttpModule::Get();
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = HttpModule.CreateRequest();
    
    FString FullUrl = BaseUrl + Endpoint;
    Request->SetURL(FullUrl);
    Request->SetVerb(Verb);
    Request->SetTimeout(10);
    Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *AuthToken));
    
    if (JsonBody.IsValid())
    {
        FString Content;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Content);
        FJsonSerializer::Serialize(JsonBody.ToSharedRef(), Writer);
        
        Request->SetContentAsString(Content);
        Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    }
    
    Request->SetHeader(TEXT("Accept"), TEXT("application/json"));
    
    Request->OnProcessRequestComplete().BindLambda([Callback](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
    {
        // Always execute callback on game thread
        if (Callback)
        {
            FFunctionGraphTask::CreateAndDispatchWhenReady([Response, bSuccess, Callback]()
            {
                Callback(Response, bSuccess);
            }, TStatId(), nullptr, ENamedThreads::GameThread);
        }
    });
    
    if (!Request->ProcessRequest())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to process authenticated HTTP request"));
        if (Callback)
        {
            FFunctionGraphTask::CreateAndDispatchWhenReady([Callback]()
            {
                Callback(nullptr, false);
            }, TStatId(), nullptr, ENamedThreads::GameThread);
        }
    }
}

void UEdsHttpService::HandleAuthResponse(const FHttpResponsePtr& Response, bool bSuccess, FOnAuthResponse Callback)
{
    FAuthResponse AuthResponse;
    
    if (!bSuccess || !Response.IsValid())
    {
        AuthResponse.bSuccess = false;
        AuthResponse.ErrorMessage = TEXT("Network error");
        AuthResponse.ResponseCode = 0;
    }
    else
    {
        AuthResponse.ResponseCode = Response->GetResponseCode();
        FString ResponseContent = Response->GetContentAsString();
        
        if (Response->GetResponseCode() == 200 || Response->GetResponseCode() == 201)
        {
            AuthResponse = ParseAuthResponse(ResponseContent);
        }
        else
        {
            AuthResponse.bSuccess = false;
            AuthResponse.ErrorMessage = FString::Printf(TEXT("Request failed (Code: %d)"), Response->GetResponseCode());
            
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
        }
    }
    
    if (Callback.IsBound())
    {
        Callback.Execute(AuthResponse);
    }
}

void UEdsHttpService::HandleProfileResponse(const FHttpResponsePtr& Response, bool bSuccess, FOnProfileResponse Callback)
{
    FUserProfile Profile;
    
    if (!bSuccess || !Response.IsValid())
    {
        Profile.bIsValid = false;
    }
    else if (Response->GetResponseCode() == 200)
    {
        FString ResponseContent = Response->GetContentAsString();
        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);
        
        if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
        {
            Profile = ParseUserProfile(JsonObject);
            Profile.bIsValid = true;
        }
    }
    
    if (Callback.IsBound())
    {
        Callback.Execute(Profile);
    }
}

void UEdsHttpService::HandleSimpleResponse(const FHttpResponsePtr& Response, bool bSuccess, FOnSimpleResponse Callback)
{
    bool bSuccessResult = bSuccess && Response.IsValid() && (Response->GetResponseCode() == 200 || Response->GetResponseCode() == 201);
    
    if (Callback.IsBound())
    {
        Callback.Execute(bSuccessResult);
    }
}

FAuthResponse UEdsHttpService::ParseAuthResponse(const FString& JsonString)
{
    FAuthResponse AuthResponse;
    AuthResponse.bSuccess = false;
    
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
    {
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
        
        // Try to get token
        JsonObject->TryGetStringField(TEXT("token"), AuthResponse.Token);
        
        // If token is in a data object
        const TSharedPtr<FJsonObject>* DataObjectPtr = nullptr;
        if (JsonObject->TryGetObjectField(TEXT("data"), DataObjectPtr) && DataObjectPtr != nullptr)
        {
            const TSharedPtr<FJsonObject>& DataObject = *DataObjectPtr;
            
            // Get token from data object if not already found
            if (AuthResponse.Token.IsEmpty())
            {
                DataObject->TryGetStringField(TEXT("token"), AuthResponse.Token);
            }
            
            // Parse user profile
            AuthResponse.UserProfile = ParseUserProfile(DataObject);
        }
        else
        {
            // Parse user profile from root
            AuthResponse.UserProfile = ParseUserProfile(JsonObject);
        }
        
        // Get error message if any
        if (!AuthResponse.bSuccess || AuthResponse.Token.IsEmpty())
        {
            JsonObject->TryGetStringField(TEXT("message"), AuthResponse.ErrorMessage);
            if (AuthResponse.ErrorMessage.IsEmpty())
            {
                AuthResponse.ErrorMessage = TEXT("Authentication failed");
            }
        }
    }
    
    return AuthResponse;
}

FUserProfile UEdsHttpService::ParseUserProfile(const TSharedPtr<FJsonObject>& JsonObject)
{
    FUserProfile Profile;
    
    // Try different field names for user ID
    if (!JsonObject->TryGetStringField(TEXT("userId"), Profile.UserId))
    {
        if (!JsonObject->TryGetStringField(TEXT("user_id"), Profile.UserId))
        {
            JsonObject->TryGetStringField(TEXT("id"), Profile.UserId);
        }
    }
    
    // Try different field names for username
    if (!JsonObject->TryGetStringField(TEXT("username"), Profile.Username))
    {
        FString TempUsername;
        if (JsonObject->TryGetStringField(TEXT("userName"), TempUsername))
        {
            Profile.Username = TempUsername;
        }
        else if (JsonObject->TryGetStringField(TEXT("name"), TempUsername))
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
