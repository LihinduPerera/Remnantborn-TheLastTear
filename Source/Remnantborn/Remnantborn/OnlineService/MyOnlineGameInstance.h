#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "FindSessionsCallbackProxy.h"
#include "UEdsHttpService.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundCue.h"
#include "MyOnlineGameInstance.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionSearchCompleted, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCreateSessionSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCreateSessionFailed, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnJoinSessionFailed, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSessionDestroyed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAuthStateChanged, bool, bIsLoggedIn);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProfileUpdated, const FUserProfile&, UserProfile);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMatchRewardReceived, int32, RewardAmount, int32, NewRemnantCount, bool, bIsWinner);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMatchCompleteSubmitted, bool, bSuccess, const FString&, MatchId, bool, bIdempotentReplay);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAvatarUploadComplete, bool, bSuccess);

USTRUCT(BlueprintType)
struct FSessionInfo
{
    GENERATED_BODY()
    
    UPROPERTY(BlueprintReadOnly)
    int32 CurrentPlayers = 0;
    
    UPROPERTY(BlueprintReadOnly)
    int32 MaxPlayers = 0;
    
    UPROPERTY(BlueprintReadOnly)
    int32 Ping = 0;
    
    UPROPERTY(BlueprintReadOnly)
    FString SessionName;
    
    UPROPERTY(BlueprintReadOnly)
    FBlueprintSessionResult SessionResult;
};

UENUM(BlueprintType)
enum class EMusicState : uint8
{
    None    UMETA(DisplayName = "None"),
    Menu    UMETA(DisplayName = "Menu"),
    Gameplay UMETA(DisplayName = "Gameplay"),
    Result  UMETA(DisplayName = "Result")
};

UCLASS()
class REMNANTBORN_API UMyOnlineGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    UMyOnlineGameInstance();
    
    // === Multiplayer Functions ===
    UFUNCTION(BlueprintCallable, Category = "Multiplayer")
    void CreateSession(FString SessionName = "LAN_Game", int32 MaxPlayers = 4);
    
    UFUNCTION(BlueprintCallable, Category = "Multiplayer")
    void FindSessions();
    
    UFUNCTION(BlueprintCallable, Category = "Multiplayer")
    void JoinSessionByIndex(int32 SessionIndex);
    
    UFUNCTION(BlueprintCallable, Category = "Multiplayer")
    void JoinSessionByResult(const FBlueprintSessionResult& SessionResult);
    
    UFUNCTION(BlueprintCallable, Category = "Multiplayer")
    void DestroySession();
    
    UFUNCTION(BlueprintCallable, Category = "Multiplayer")
    void JoinByIP(FString IPAddress, int32 Port = 7777);
    
    UFUNCTION(BlueprintCallable, Category = "Multiplayer")
    void StartGameAsClient(FString IPAddress, int32 Port = 7777);
    
    UFUNCTION(BlueprintCallable, Category = "Multiplayer")
    void StartGameAsServer(FString MapPath, int32 MaxPlayers = 4);
    
    UFUNCTION(BlueprintCallable, Category = "Multiplayer")
    void LeaveGame();
    
    // === Lobby Functions ===
    UFUNCTION(BlueprintCallable, Category = "Multiplayer")
    void CreateSessionWithLobby(FString SessionName = "LAN_Game", int32 MaxPlayers = 2, FString LobbyMapPath = "/Game/Remnantborn/Levels/Lobby");
    
    UFUNCTION(BlueprintCallable, Category = "Multiplayer")
    void SetLobbyMapPath(const FString& NewLobbyMapPath);
    
    // === Getters for Lobby ===
    UFUNCTION(BlueprintPure, Category = "Multiplayer")
    FString GetLobbyMapPath() const { return LobbyMapPath; }
    
    // === Character Selection Persistence ===
    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    void SetLocalCharacterSelection(FName CharacterID);
    
    UFUNCTION(BlueprintPure, Category = "Character Selection")
    FName GetLocalCharacterSelection() const { return LocalCharacterSelection; }
    
    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    void ClearLocalCharacterSelection();
    
    // === Multiplayer Character Selection Storage (persists through seamless travel) ===
    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    void StorePlayerCharacterSelection(const FString& PlayerName, const FName& CharacterID);
    
    UFUNCTION(BlueprintPure, Category = "Character Selection")
    FName GetPlayerCharacterSelection(const FString& PlayerName) const;
    
    UFUNCTION(BlueprintPure, Category = "Character Selection")
    bool HasPlayerCharacterSelection(const FString& PlayerName) const;
    
    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    void RemovePlayerCharacterSelection(const FString& PlayerName);
    
    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    void ClearAllCharacterSelections();
    
    UFUNCTION(BlueprintPure, Category = "Character Selection")
    TMap<FString, FName> GetAllCharacterSelections() const { return PlayerCharacterSelections; }

    void StorePlayerCharacterSelectionForPlayerState(const class APlayerState* PlayerState, const FName& CharacterID);
    FName GetPlayerCharacterSelectionForPlayerState(const class APlayerState* PlayerState) const;
    FString BuildPlayerCharacterSelectionKey(const class APlayerState* PlayerState) const;

    void StorePlayerCharacterSelectionForPlayerController(const class APlayerController* PlayerController, const FName& CharacterID);
    FName GetPlayerCharacterSelectionForPlayerController(const class APlayerController* PlayerController) const;
    FString BuildPlayerCharacterSelectionKeyFromController(const class APlayerController* PlayerController) const;
    
    UFUNCTION(BlueprintCallable, Category = "Multiplayer")
    void TravelToGameLevel(FString GameMapPath = "/Game/Remnantborn/Levels/TestGround");
    
    // === Background Music Functions ===
    UFUNCTION(BlueprintCallable, Category = "BackgroundMusic")
    void StartBackgroundMusic();
    
    UFUNCTION(BlueprintCallable, Category = "BackgroundMusic")
    void StopBackgroundMusic();
    
    UFUNCTION(BlueprintCallable, Category = "BackgroundMusic")
    void SetMusicVolume(float Volume);
    
    UFUNCTION(BlueprintCallable, Category = "BackgroundMusic")
    void SetMenuMusicVolume(float Volume);

    UFUNCTION(BlueprintCallable, Category = "BackgroundMusic")
    void SetGameplayMusicVolume(float Volume);

    UFUNCTION(BlueprintCallable, Category = "BackgroundMusic")
    void SetResultMusicVolume(float Volume);
    
    UFUNCTION(BlueprintPure, Category = "BackgroundMusic")
    float GetMusicVolume() const { return MenuMusicVolume; }

    UFUNCTION(BlueprintPure, Category = "BackgroundMusic")
    float GetMenuMusicVolume() const { return MenuMusicVolume; }

    UFUNCTION(BlueprintPure, Category = "BackgroundMusic")
    float GetGameplayMusicVolume() const { return GameplayMusicVolume; }

    UFUNCTION(BlueprintPure, Category = "BackgroundMusic")
    float GetResultMusicVolume() const { return ResultMusicVolume; }
    
    UFUNCTION(BlueprintPure, Category = "BackgroundMusic")
    bool IsMusicPlaying() const;

    UFUNCTION(BlueprintPure, Category = "BackgroundMusic")
    EMusicState GetMusicState() const { return CurrentMusicState; }
    
    UFUNCTION(BlueprintCallable, Category = "BackgroundMusic")
    void SetMusicState(EMusicState NewState);
    
    UFUNCTION(BlueprintCallable, Category = "BackgroundMusic")
    void PlayMenuMusic();
    
    UFUNCTION(BlueprintCallable, Category = "BackgroundMusic")
    void PlayGameplayMusic();
    
    UFUNCTION(BlueprintCallable, Category = "BackgroundMusic")
    void PlayResultMusic(bool bIsVictory);
    
    // === Redesigned Music System Methods ===
    UFUNCTION(BlueprintCallable, Category = "BackgroundMusic")
    void PrepareForLevelTravel();
    
    UFUNCTION(BlueprintCallable, Category = "BackgroundMusic")
    void OnEnteredLobby();
    
    UFUNCTION(BlueprintCallable, Category = "BackgroundMusic")
    void OnMatchStarted();
    
    UFUNCTION(BlueprintCallable, Category = "BackgroundMusic")
    void OnMatchEnded(bool bIsVictory);
    
    UFUNCTION(BlueprintCallable, Category = "BackgroundMusic")
    void OnReturningToLobby();
    
    UFUNCTION(BlueprintCallable, Category = "BackgroundMusic")
    void ResumeMenuMusic();
    
    // === Authentication Functions ===
    UFUNCTION(BlueprintCallable, Category = "Authentication")
    void Login(const FString& Email, const FString& Password);
    
    UFUNCTION(BlueprintCallable, Category = "Authentication")
    void Signup(const FString& Email, const FString& Password, const FString& Username);
    
    UFUNCTION(BlueprintCallable, Category = "Authentication")
    void DevLogin(const FString& Email);
    
    UFUNCTION(BlueprintCallable, Category = "Authentication")
    void Logout();
    
    UFUNCTION(BlueprintCallable, Category = "Authentication")
    void LoadSavedAuth();
    
    UFUNCTION(BlueprintCallable, Category = "Authentication")
    void GetUserProfile();
    
    UFUNCTION(BlueprintCallable, Category = "Authentication")
    void GetMyProfile();

    UFUNCTION(BlueprintCallable, Category = "Authentication")
    void ApplyProfileUpdate(const FUserProfile& Profile);
    
    UFUNCTION(BlueprintCallable, Category = "Authentication")
    void UpdateProfile(const FString& Username, const FString& Bio);
    
    UFUNCTION(BlueprintCallable, Category = "Authentication")
    void UpdateProfileWithAvatar(const FString& Username, const FString& Bio, const FString& AvatarUrl);
    
    UFUNCTION(BlueprintCallable, Category = "Authentication")
    void UploadAvatar(const FString& FilePath);

    // convenience helper that shows the desktop file dialog and returns a path to an image file
    // returns true when the user picked a file, false otherwise (cancelled or error)
    UFUNCTION(BlueprintCallable, Category = "Utilities")
    bool PickImageFile(FString& OutFilePath);
    
    UFUNCTION(BlueprintCallable, Category = "Authentication")
    void UpdateGameStats(int32 Level, int32 RemnantCount, const FString& Operation = "set");

    UFUNCTION(BlueprintCallable, Category = "Authentication")
    void SubmitMatchReward(bool bIsWinner, float MatchDuration, int32 EliminationOrder, const FString& MatchId = "");

    UFUNCTION(BlueprintCallable, Category = "Match")
    void SubmitMatchComplete(const FMatchCompleteRequest& MatchRequest);
    
    // === Getters ===
    UFUNCTION(BlueprintPure, Category = "Authentication")
    bool IsLoggedIn() const { return bIsLoggedIn; }
    
    UFUNCTION(BlueprintPure, Category = "Authentication")
    FUserProfile GetCurrentUserProfile() const { return CurrentUserProfile; }
    
    UFUNCTION(BlueprintPure, Category = "Authentication")
    FString GetAuthToken() const { return AuthToken; }

    UFUNCTION(BlueprintPure, Category = "Authentication")
    UEdsHttpService* GetHttpService() const { return HttpService; }
    
    // === Multiplayer Events ===
    // avatar upload notification (true on success)
    UPROPERTY(BlueprintAssignable, Category = "Authentication|Events")
    FOnAvatarUploadComplete OnAvatarUploadComplete;

    UPROPERTY(BlueprintAssignable, Category = "Multiplayer|Events")
    FOnSessionSearchCompleted OnSessionSearchCompleted;
    
    UPROPERTY(BlueprintAssignable, Category = "Multiplayer|Events")
    FOnCreateSessionSuccess OnCreateSessionSuccess;
    
    UPROPERTY(BlueprintAssignable, Category = "Multiplayer|Events")
    FOnCreateSessionFailed OnCreateSessionFailed;
    
    UPROPERTY(BlueprintAssignable, Category = "Multiplayer|Events")
    FOnJoinSessionFailed OnJoinSessionFailed;
    
    UPROPERTY(BlueprintAssignable, Category = "Multiplayer|Events")
    FOnSessionDestroyed OnSessionDestroyed;
    
    // === Authentication Events ===
    UPROPERTY(BlueprintAssignable, Category = "Authentication|Events")
    FOnAuthStateChanged OnAuthStateChanged;
    
    UPROPERTY(BlueprintAssignable, Category = "Profile|Events")
    FOnProfileUpdated OnProfileUpdated;

    UPROPERTY(BlueprintAssignable, Category = "Match|Events")
    FOnMatchRewardReceived OnMatchRewardReceived;

    UPROPERTY(BlueprintAssignable, Category = "Match|Events")
    FOnMatchCompleteSubmitted OnMatchCompleteSubmitted;
    
    // === Data for Blueprint ===
    UPROPERTY(BlueprintReadOnly, Category = "Multiplayer")
    TArray<FSessionInfo> SessionSearchResults;
    
    UPROPERTY(BlueprintReadOnly, Category = "Multiplayer")
    FString LastError;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multiplayer")
    FString DefaultMapPath;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multiplayer")
    int32 DefaultMaxPlayers = 4;
    
    // === Background Music ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BackgroundMusic")
    TArray<USoundCue*> MainMenuPlaylist;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BackgroundMusic")
    TArray<USoundCue*> GameplayPlaylist;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BackgroundMusic")
    TArray<USoundCue*> ResultPlaylist;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BackgroundMusic")
    float MenuMusicVolume = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BackgroundMusic")
    float GameplayMusicVolume = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BackgroundMusic")
    float ResultMusicVolume = 0.5f;

    // Legacy - kept for backward compatibility
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BackgroundMusic")
    float MusicVolume = 0.5f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BackgroundMusic")
    bool bShufflePlaylist = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BackgroundMusic")
    bool bAutoPlayOnLevelChange = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BackgroundMusic")
    EMusicState CurrentMusicState = EMusicState::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BackgroundMusic")
    EMusicState PendingMusicState = EMusicState::Menu;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BackgroundMusic")
    bool bMusicWasPlayingBeforeTravel = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BackgroundMusic")
    bool bIsInMatch = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BackgroundMusic")
    bool bMatchHasEnded = false;

protected:
    virtual void Init() override;
    virtual void Shutdown() override;
    virtual void OnStart() override;
    
private:
    bool HasNamedGameSession() const;
    void ContinueCreateSessionAfterDestroy();
    void ContinueJoinSessionAfterDestroy();
    void ReturnToMainMenuLevel();

    // === Multiplayer ===
    void OnCreateSessionComplete(FName SessionName, bool bSuccess);
    void OnFindSessionsComplete(bool bSuccess);
    void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
    void OnDestroySessionComplete(FName SessionName, bool bSuccess);
    
    void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
    void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);
    
    void HostGame();
    void TravelToServer(const FString& Address);
    
    IOnlineSessionPtr SessionInterface;
    TSharedPtr<FOnlineSessionSearch> SessionSearch;
    
    // Delegate Handles
    FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
    FDelegateHandle CreateSessionCompleteDelegateHandle;
    
    FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
    FDelegateHandle FindSessionsCompleteDelegateHandle;
    
    FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;
    FDelegateHandle JoinSessionCompleteDelegateHandle;
    
    FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;
    FDelegateHandle DestroySessionCompleteDelegateHandle;
    
    // Cached variables
    FString PendingSessionName;
    int32 PendingMaxPlayers;
    FBlueprintSessionResult PendingJoinSessionResult;
    bool bPendingCreateSessionAfterDestroy = false;
    bool bPendingJoinSessionAfterDestroy = false;
    bool bPendingReturnToMainMenu = false;
    bool bIsHosting = false;
    FString LobbyMapPath;
    
    // === Character Selection ===
    FName LocalCharacterSelection;
    
    // Map to store all players' character selections (persists through seamless travel)
    UPROPERTY()
    TMap<FString, FName> PlayerCharacterSelections;
    
    // === Authentication ===
    UPROPERTY()
    UEdsHttpService* HttpService;
    
    // Current user data
    FUserProfile CurrentUserProfile;
    FString AuthToken;
    FString CurrentUserId;
    bool bIsLoggedIn = false;
    
    // Helper methods
    void SetAuthState(bool bLoggedIn, const FString& Token = FString(), const FString& UserId = FString(), const FUserProfile& Profile = FUserProfile());
    
    // === Background Music ===
    UPROPERTY()
    UAudioComponent* MusicAudioComponent;
    
    int32 CurrentTrackIndex;
    
    TArray<USoundCue*>* CurrentPlaylist;
    
    void PlayNextTrack();
    void OnTrackFinished();
    void PlayFromPlaylist(TArray<USoundCue*>* Playlist);
    UWorld* GetMusicWorld() const;
};
