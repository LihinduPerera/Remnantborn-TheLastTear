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

void UEdsHttpService::GetMyProfile(const FString& AuthToken, FOnProfileResponse Callback)
{
    SendRequestWithAuth(TEXT("/profile/me"), TEXT("GET"), nullptr, AuthToken,
        [this, Callback](const FHttpResponsePtr& Response, bool bSuccess)
        {
            HandleProfileResponse(Response, bSuccess, Callback);
        });
}

void UEdsHttpService::UpdateProfile(const FString& UserId, const FString& AuthToken, const FString& Username, const FString& Bio, FOnProfileUpdateResponse Callback)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    if (!Username.IsEmpty()) JsonObject->SetStringField(TEXT("username"), Username);
    if (!Bio.IsEmpty()) JsonObject->SetStringField(TEXT("bio"), Bio);
    
    SendRequestWithAuth(FString::Printf(TEXT("/profile/%s"), *UserId), TEXT("PUT"), JsonObject, AuthToken,
        [this, Callback](const FHttpResponsePtr& Response, bool bSuccess)
        {
            HandleProfileUpdateResponse(Response, bSuccess, Callback);
        });
}

void UEdsHttpService::UpdateProfileWithAvatar(const FString& UserId, const FString& AuthToken, const FString& Username, const FString& Bio, const FString& AvatarUrl, FOnProfileUpdateResponse Callback)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    if (!Username.IsEmpty()) JsonObject->SetStringField(TEXT("username"), Username);
    if (!Bio.IsEmpty()) JsonObject->SetStringField(TEXT("bio"), Bio);
    if (!AvatarUrl.IsEmpty()) JsonObject->SetStringField(TEXT("avatar_url"), AvatarUrl);
    
    SendRequestWithAuth(FString::Printf(TEXT("/profile/%s"), *UserId), TEXT("PUT"), JsonObject, AuthToken,
        [this, Callback](const FHttpResponsePtr& Response, bool bSuccess)
        {
            HandleProfileUpdateResponse(Response, bSuccess, Callback);
        });
}

void UEdsHttpService::UploadAvatar(const FString& AuthToken, const FString& FilePath, FOnAvatarUploadResponse Callback)
{
    SendMultipartRequest(TEXT("/profile/upload-avatar"), FilePath, TEXT("avatar"), AuthToken,
        [this, Callback](const FHttpResponsePtr& Response, bool bSuccess)
        {
            HandleAvatarUploadResponse(Response, bSuccess, Callback);
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

void UEdsHttpService::GetStoreCharacters(const FString& AuthToken, FOnStoreCharactersResponse Callback)
{
    SendRequestWithAuth(TEXT("/store/characters"), TEXT("GET"), nullptr, AuthToken,
        [Callback](const FHttpResponsePtr& Response, bool bSuccess)
        {
            TArray<FStoreCharacterInfo> Results;

            if (bSuccess && Response.IsValid() && Response->GetResponseCode() == 200)
            {
                TSharedPtr<FJsonObject> JsonObject;
                TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

                if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
                {
                    const TSharedPtr<FJsonObject>* DataObjectPtr = nullptr;
                    const TSharedPtr<FJsonObject> DataObject =
                        JsonObject->TryGetObjectField(TEXT("data"), DataObjectPtr) && DataObjectPtr != nullptr
                        ? *DataObjectPtr
                        : JsonObject;

                    const TArray<TSharedPtr<FJsonValue>>* CharactersArray = nullptr;
                    if (DataObject.IsValid() && DataObject->TryGetArrayField(TEXT("characters"), CharactersArray))
                    {
                        for (const TSharedPtr<FJsonValue>& CharacterValue : *CharactersArray)
                        {
                            const TSharedPtr<FJsonObject>* CharacterObjPtr = nullptr;
                            if (!CharacterValue.IsValid() || !CharacterValue->TryGetObject(CharacterObjPtr) || CharacterObjPtr == nullptr)
                            {
                                continue;
                            }

                            const TSharedPtr<FJsonObject>& CharacterObj = *CharacterObjPtr;
                            FStoreCharacterInfo CharacterInfo;
                            // parse type first so callers can decide how to treat the item
                            CharacterObj->TryGetStringField(TEXT("item_type"), CharacterInfo.ItemType);
                            CharacterObj->TryGetStringField(TEXT("item_id"), CharacterInfo.ItemId);
                            CharacterObj->TryGetStringField(TEXT("name"), CharacterInfo.Name);
                            CharacterObj->TryGetStringField(TEXT("description"), CharacterInfo.Description);
                            CharacterObj->TryGetStringField(TEXT("image_url"), CharacterInfo.ImageUrl);
                            CharacterObj->TryGetBoolField(TEXT("owned"), CharacterInfo.bOwned);
                            CharacterObj->TryGetBoolField(TEXT("can_afford"), CharacterInfo.bCanAfford);

                            double Price = 0.0;
                            if (CharacterObj->TryGetNumberField(TEXT("price"), Price))
                            {
                                CharacterInfo.Price = static_cast<int32>(Price);
                            }

                            Results.Add(CharacterInfo);
                        }
                    }
                }
            }

            if (Callback.IsBound())
            {
                Callback.Execute(Results);
            }
        });
}

void UEdsHttpService::GetRemnantPackages(const FString& AuthToken, FOnRemnantPackagesResponse Callback)
{
    SendRequestWithAuth(TEXT("/store/packages"), TEXT("GET"), nullptr, AuthToken,
        [Callback](const FHttpResponsePtr& Response, bool bSuccess)
        {
            TArray<FRemnantPackage> Results;

            if (bSuccess && Response.IsValid() && Response->GetResponseCode() == 200)
            {
                TSharedPtr<FJsonObject> JsonObject;
                TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

                if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
                {
                    const TSharedPtr<FJsonObject>* DataObjectPtr = nullptr;
                    const TSharedPtr<FJsonObject> DataObject =
                        JsonObject->TryGetObjectField(TEXT("data"), DataObjectPtr) && DataObjectPtr != nullptr
                        ? *DataObjectPtr
                        : JsonObject;

                    const TArray<TSharedPtr<FJsonValue>>* PackagesArray = nullptr;
                    if (DataObject.IsValid())
                    {
                        if (!DataObject->TryGetArrayField(TEXT("packages"), PackagesArray))
                        {
                            DataObject->TryGetArrayField(TEXT("data"), PackagesArray);
                        }
                    }

                    if (PackagesArray == nullptr && JsonObject.IsValid())
                    {
                        JsonObject->TryGetArrayField(TEXT("data"), PackagesArray);
                    }

                    if (PackagesArray != nullptr)
                    {
                        for (const TSharedPtr<FJsonValue>& PackageValue : *PackagesArray)
                        {
                            const TSharedPtr<FJsonObject>* PackageObjPtr = nullptr;
                            if (!PackageValue.IsValid() || !PackageValue->TryGetObject(PackageObjPtr) || PackageObjPtr == nullptr)
                            {
                                continue;
                            }

                            const TSharedPtr<FJsonObject>& PackageObj = *PackageObjPtr;
                            FRemnantPackage PackageInfo;
                            PackageObj->TryGetStringField(TEXT("id"), PackageInfo.PackageId);
                            PackageObj->TryGetStringField(TEXT("name"), PackageInfo.Name);
                            PackageObj->TryGetStringField(TEXT("display_price"), PackageInfo.DisplayPrice);

                            double RemnantAmount = 0.0;
                            if (PackageObj->TryGetNumberField(TEXT("remnant_amount"), RemnantAmount))
                            {
                                PackageInfo.RemnantAmount = static_cast<int32>(RemnantAmount);
                            }

                            double SortOrder = 0.0;
                            if (PackageObj->TryGetNumberField(TEXT("sort_order"), SortOrder))
                            {
                                PackageInfo.SortOrder = static_cast<int32>(SortOrder);
                            }

                            Results.Add(PackageInfo);
                        }
                    }
                }
            }

            if (Callback.IsBound())
            {
                Callback.Execute(Results);
            }
        });
}

void UEdsHttpService::BuyCharacter(const FString& AuthToken, const FString& CharacterId, FOnCharacterPurchaseResponse Callback)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    JsonObject->SetStringField(TEXT("characterId"), CharacterId);

    SendRequestWithAuth(TEXT("/store/buy-character"), TEXT("POST"), JsonObject, AuthToken,
        [Callback](const FHttpResponsePtr& Response, bool bSuccess)
        {
            FCharacterPurchaseResponse Result;

            if (!bSuccess || !Response.IsValid())
            {
                Result.bSuccess = false;
                Result.ErrorMessage = TEXT("Network error");
            }
            else
            {
                TSharedPtr<FJsonObject> JsonResp;
                TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

                if (FJsonSerializer::Deserialize(Reader, JsonResp) && JsonResp.IsValid())
                {
                    bool bSuccessField = false;
                    JsonResp->TryGetBoolField(TEXT("success"), bSuccessField);
                    Result.bSuccess = bSuccessField && (Response->GetResponseCode() == 200 || Response->GetResponseCode() == 201);

                    const TSharedPtr<FJsonObject>* DataObjectPtr = nullptr;
                    const TSharedPtr<FJsonObject> DataObject =
                        JsonResp->TryGetObjectField(TEXT("data"), DataObjectPtr) && DataObjectPtr != nullptr
                        ? *DataObjectPtr
                        : JsonResp;

                    if (DataObject.IsValid())
                    {
                        DataObject->TryGetStringField(TEXT("character_id"), Result.CharacterId);
                        double NewCount = 0.0;
                        if (DataObject->TryGetNumberField(TEXT("new_remnant_count"), NewCount))
                        {
                            Result.NewRemnantCount = static_cast<int32>(NewCount);
                        }
                    }

                    if (!Result.bSuccess)
                    {
                        JsonResp->TryGetStringField(TEXT("message"), Result.ErrorMessage);
                        if (Result.ErrorMessage.IsEmpty())
                        {
                            Result.ErrorMessage = FString::Printf(TEXT("Purchase failed (Code: %d)"), Response->GetResponseCode());
                        }
                    }
                }
                else
                {
                    Result.bSuccess = false;
                    Result.ErrorMessage = TEXT("Invalid response format");
                }
            }

            if (Callback.IsBound())
            {
                Callback.Execute(Result);
            }
        });
}

void UEdsHttpService::BuyRemnants(const FString& AuthToken, const FString& PackageId, const FString& CardNumber, const FString& CardExpiry, const FString& CardCVV, FOnRemnantPurchaseResponse Callback)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    JsonObject->SetStringField(TEXT("packageId"), PackageId);
    JsonObject->SetStringField(TEXT("cardNumber"), CardNumber);
    JsonObject->SetStringField(TEXT("cardExpiry"), CardExpiry);
    JsonObject->SetStringField(TEXT("cardCVV"), CardCVV);

    SendRequestWithAuth(TEXT("/store/buy-remnants"), TEXT("POST"), JsonObject, AuthToken,
        [Callback](const FHttpResponsePtr& Response, bool bSuccess)
        {
            FRemnantPurchaseResponse Result;

            if (!bSuccess || !Response.IsValid())
            {
                Result.bSuccess = false;
                Result.ErrorMessage = TEXT("Network error");
            }
            else
            {
                TSharedPtr<FJsonObject> JsonResp;
                TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

                if (FJsonSerializer::Deserialize(Reader, JsonResp) && JsonResp.IsValid())
                {
                    bool bSuccessField = false;
                    JsonResp->TryGetBoolField(TEXT("success"), bSuccessField);
                    Result.bSuccess = bSuccessField && (Response->GetResponseCode() == 200 || Response->GetResponseCode() == 201);

                    const TSharedPtr<FJsonObject>* DataObjectPtr = nullptr;
                    const TSharedPtr<FJsonObject> DataObject =
                        JsonResp->TryGetObjectField(TEXT("data"), DataObjectPtr) && DataObjectPtr != nullptr
                        ? *DataObjectPtr
                        : JsonResp;

                    if (DataObject.IsValid())
                    {
                        double Added = 0.0;
                        if (DataObject->TryGetNumberField(TEXT("remnants_added"), Added))
                        {
                            Result.RemnantsAdded = static_cast<int32>(Added);
                        }

                        double NewCount = 0.0;
                        if (DataObject->TryGetNumberField(TEXT("new_remnant_count"), NewCount))
                        {
                            Result.NewRemnantCount = static_cast<int32>(NewCount);
                        }

                        DataObject->TryGetStringField(TEXT("receipt_id"), Result.ReceiptId);
                    }

                    if (!Result.bSuccess)
                    {
                        JsonResp->TryGetStringField(TEXT("message"), Result.ErrorMessage);
                        if (Result.ErrorMessage.IsEmpty())
                        {
                            Result.ErrorMessage = FString::Printf(TEXT("Remnant purchase failed (Code: %d)"), Response->GetResponseCode());
                        }
                    }
                }
                else
                {
                    Result.bSuccess = false;
                    Result.ErrorMessage = TEXT("Invalid response format");
                }
            }

            if (Callback.IsBound())
            {
                Callback.Execute(Result);
            }
        });
}

void UEdsHttpService::SubmitMatchReward(const FString& AuthToken, bool bIsWinner, float MatchDuration, int32 EliminationOrder, const FString& MatchId, FOnMatchRewardResponse Callback)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    JsonObject->SetBoolField(TEXT("isWinner"), bIsWinner);
    JsonObject->SetNumberField(TEXT("matchDuration"), MatchDuration);
    JsonObject->SetNumberField(TEXT("eliminationOrder"), EliminationOrder);
    if (!MatchId.IsEmpty())
    {
        JsonObject->SetStringField(TEXT("matchId"), MatchId);
    }

    SendRequestWithAuth(TEXT("/match/reward"), TEXT("POST"), JsonObject, AuthToken,
        [Callback](const FHttpResponsePtr& Response, bool bSuccess)
        {
            FMatchRewardResponse Result;

            if (!bSuccess || !Response.IsValid())
            {
                Result.bSuccess = false;
                Result.ErrorMessage = TEXT("Network error");
            }
            else
            {
                TSharedPtr<FJsonObject> JsonResp;
                TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

                if (FJsonSerializer::Deserialize(Reader, JsonResp) && JsonResp.IsValid())
                {
                    bool bSuccessField = false;
                    JsonResp->TryGetBoolField(TEXT("success"), bSuccessField);
                    Result.bSuccess = bSuccessField && (Response->GetResponseCode() == 200 || Response->GetResponseCode() == 201);

                    const TSharedPtr<FJsonObject>* DataObjectPtr = nullptr;
                    const TSharedPtr<FJsonObject> DataObject =
                        JsonResp->TryGetObjectField(TEXT("data"), DataObjectPtr) && DataObjectPtr != nullptr
                        ? *DataObjectPtr
                        : JsonResp;

                    if (DataObject.IsValid())
                    {
                        double RewardAmount = 0.0;
                        if (DataObject->TryGetNumberField(TEXT("reward_amount"), RewardAmount))
                        {
                            Result.RewardAmount = static_cast<int32>(RewardAmount);
                        }

                        double NewCount = 0.0;
                        if (DataObject->TryGetNumberField(TEXT("new_remnant_count"), NewCount))
                        {
                            Result.NewRemnantCount = static_cast<int32>(NewCount);
                        }

                        DataObject->TryGetBoolField(TEXT("is_winner"), Result.bIsWinner);
                    }

                    if (!Result.bSuccess)
                    {
                        JsonResp->TryGetStringField(TEXT("message"), Result.ErrorMessage);
                        if (Result.ErrorMessage.IsEmpty())
                        {
                            Result.ErrorMessage = FString::Printf(TEXT("Match reward submission failed (Code: %d)"), Response->GetResponseCode());
                        }
                    }
                }
                else
                {
                    Result.bSuccess = false;
                    Result.ErrorMessage = TEXT("Invalid response format");
                }
            }

            if (Callback.IsBound())
            {
                Callback.Execute(Result);
            }
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

void UEdsHttpService::SendMultipartRequest(const FString& Endpoint, const FString& FilePath, const FString& FieldName,
                                          const FString& AuthToken, TFunction<void(const FHttpResponsePtr&, bool)> Callback)
{
    if (AuthToken.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("Attempted multipart request without token"));
        if (Callback)
        {
            FFunctionGraphTask::CreateAndDispatchWhenReady([Callback]()
            {
                Callback(nullptr, false);
            }, TStatId(), nullptr, ENamedThreads::GameThread);
        }
        return;
    }
    
    // prepare file
    TArray<uint8> FileData;
    if (!FFileHelper::LoadFileToArray(FileData, *FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to read file: %s"), *FilePath);
        if (Callback)
        {
            FFunctionGraphTask::CreateAndDispatchWhenReady([Callback]()
            {
                Callback(nullptr, false);
            }, TStatId(), nullptr, ENamedThreads::GameThread);
        }
        return;
    }

    FString FileName = FPaths::GetCleanFilename(FilePath);
    FString ContentType = TEXT("application/octet-stream");
    if (FileName.EndsWith(TEXT(".png")) || FileName.EndsWith(TEXT(".PNG")))
        ContentType = TEXT("image/png");
    else if (FileName.EndsWith(TEXT(".jpg")) || FileName.EndsWith(TEXT(".jpeg")) || FileName.EndsWith(TEXT(".JPG")) || FileName.EndsWith(TEXT(".JPEG")))
        ContentType = TEXT("image/jpeg");
    else if (FileName.EndsWith(TEXT(".gif")))
        ContentType = TEXT("image/gif");
    
    // build multipart body
    FString Boundary = TEXT("----WebKitFormBoundary") + FGuid::NewGuid().ToString();
    FString Header = FString::Printf(TEXT("--%s\r\n"), *Boundary);
    Header += FString::Printf(TEXT("Content-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\n"), *FieldName, *FileName);
    Header += FString::Printf(TEXT("Content-Type: %s\r\n\r\n"), *ContentType);

    FString Footer = FString::Printf(TEXT("\r\n--%s--\r\n"), *Boundary);

    TArray<uint8> Content;
    // append header
    {
        FTCHARToUTF8 Convert(*Header);
        Content.Append((uint8*)Convert.Get(), Convert.Length());
    }
    // append file data
    Content.Append(FileData);
    // append footer
    {
        FTCHARToUTF8 Convert(*Footer);
        Content.Append((uint8*)Convert.Get(), Convert.Length());
    }

    FHttpModule& HttpModule = FHttpModule::Get();
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = HttpModule.CreateRequest();
    
    FString FullUrl = BaseUrl + Endpoint;
    Request->SetURL(FullUrl);
    Request->SetVerb(TEXT("POST"));
    Request->SetTimeout(30);
    Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *AuthToken));
    Request->SetHeader(TEXT("Content-Type"), FString::Printf(TEXT("multipart/form-data; boundary=%s"), *Boundary));
    Request->SetContent(Content);
    
    Request->OnProcessRequestComplete().BindLambda([Callback](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
    {
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
        UE_LOG(LogTemp, Error, TEXT("Failed to process multipart HTTP request"));
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
        Profile = ParseUserProfileFromResponse(ResponseContent);
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

void UEdsHttpService::HandleProfileUpdateResponse(const FHttpResponsePtr& Response, bool bSuccess, FOnProfileUpdateResponse Callback)
{
    FProfileUpdateResponse UpdateResponse;
    
    if (!bSuccess || !Response.IsValid())
    {
        UpdateResponse.bSuccess = false;
        UpdateResponse.ErrorMessage = TEXT("Network error");
    }
    else
    {
        FString ResponseContent = Response->GetContentAsString();
        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);
        
        if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
        {
            bool bSuccessFlag = false;
            if (JsonObject->TryGetBoolField(TEXT("success"), bSuccessFlag))
            {
                UpdateResponse.bSuccess = bSuccessFlag;
            }
            else
            {
                UpdateResponse.bSuccess = (Response->GetResponseCode() == 200 || Response->GetResponseCode() == 201);
            }
            
            // Get data object if present
            const TSharedPtr<FJsonObject>* DataObjectPtr = nullptr;
            if (JsonObject->TryGetObjectField(TEXT("data"), DataObjectPtr) && DataObjectPtr != nullptr)
            {
                const TSharedPtr<FJsonObject>& DataObject = *DataObjectPtr;
                DataObject->TryGetStringField(TEXT("username"), UpdateResponse.Username);
                DataObject->TryGetStringField(TEXT("avatar_url"), UpdateResponse.AvatarUrl);
                DataObject->TryGetStringField(TEXT("bio"), UpdateResponse.Bio);
            }
            else
            {
                JsonObject->TryGetStringField(TEXT("username"), UpdateResponse.Username);
                JsonObject->TryGetStringField(TEXT("avatar_url"), UpdateResponse.AvatarUrl);
                JsonObject->TryGetStringField(TEXT("bio"), UpdateResponse.Bio);
            }
            
            if (!UpdateResponse.bSuccess)
            {
                FString Message;
                if (JsonObject->TryGetStringField(TEXT("message"), Message))
                {
                    UpdateResponse.ErrorMessage = Message;
                }
            }
        }
    }
    
    if (Callback.IsBound())
    {
        Callback.Execute(UpdateResponse);
    }
}

void UEdsHttpService::HandleAvatarUploadResponse(const FHttpResponsePtr& Response, bool bSuccess, FOnAvatarUploadResponse Callback)
{
    FAvatarUploadResponse UploadResponse;
    
    if (!bSuccess || !Response.IsValid())
    {
        UploadResponse.bSuccess = false;
        UploadResponse.ErrorMessage = TEXT("Network error");
    }
    else
    {
        FString ResponseContent = Response->GetContentAsString();
        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);
        
        if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
        {
            bool bSuccessFlag = false;
            if (JsonObject->TryGetBoolField(TEXT("success"), bSuccessFlag))
            {
                UploadResponse.bSuccess = bSuccessFlag;
            }
            else
            {
                UploadResponse.bSuccess = (Response->GetResponseCode() == 200 || Response->GetResponseCode() == 201);
            }
            
            const TSharedPtr<FJsonObject>* DataObjectPtr = nullptr;
            if (JsonObject->TryGetObjectField(TEXT("data"), DataObjectPtr) && DataObjectPtr != nullptr)
            {
                const TSharedPtr<FJsonObject>& DataObject = *DataObjectPtr;
                DataObject->TryGetStringField(TEXT("avatar_url"), UploadResponse.AvatarUrl);
            }
            else
            {
                JsonObject->TryGetStringField(TEXT("avatar_url"), UploadResponse.AvatarUrl);
            }
            
            if (!UploadResponse.bSuccess)
            {
                FString Message;
                if (JsonObject->TryGetStringField(TEXT("message"), Message))
                {
                    UploadResponse.ErrorMessage = Message;
                }
            }
        }
    }
    
    if (Callback.IsBound())
    {
        Callback.Execute(UploadResponse);
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
    JsonObject->TryGetStringField(TEXT("created_at"), Profile.CreatedAt);
    JsonObject->TryGetStringField(TEXT("updated_at"), Profile.UpdatedAt);
    
    // Parse purchased items array
    const TArray<TSharedPtr<FJsonValue>>* PurchasedItemsArray;
    if (JsonObject->TryGetArrayField(TEXT("purchased_items"), PurchasedItemsArray))
    {
        for (const auto& Item : *PurchasedItemsArray)
        {
            FString ItemStr;
            if (Item->TryGetString(ItemStr))
            {
                Profile.PurchasedItems.Add(ItemStr);
            }
        }
    }
    
    return Profile;
}

FUserProfile UEdsHttpService::ParseUserProfileFromResponse(const FString& JsonString)
{
    FUserProfile Profile;
    Profile.bIsValid = false;
    
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
    {
        bool bSuccess = false;
        if (JsonObject->TryGetBoolField(TEXT("success"), bSuccess))
        {
            if (!bSuccess)
            {
                return Profile;
            }
        }
        
        // Get data object if present
        const TSharedPtr<FJsonObject>* DataObjectPtr = nullptr;
        if (JsonObject->TryGetObjectField(TEXT("data"), DataObjectPtr) && DataObjectPtr != nullptr)
        {
            Profile = ParseUserProfile(*DataObjectPtr);
            Profile.bIsValid = true;
        }
        else
        {
            // Parse directly from root
            Profile = ParseUserProfile(JsonObject);
            Profile.bIsValid = true;
        }
    }
    
    return Profile;
}
