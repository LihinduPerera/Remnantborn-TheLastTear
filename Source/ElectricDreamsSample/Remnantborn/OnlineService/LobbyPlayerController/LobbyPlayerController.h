#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LobbyPlayerController.generated.h"

UCLASS()
class ELECTRICDREAMSSAMPLE_API ALobbyPlayerController : public APlayerController
{
    GENERATED_BODY()
    
public:
    virtual void BeginPlay() override;
    
    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void SetPlayerReady(bool bReady);
    
    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void StartMatchCountdown();
    
    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void CancelMatchCountdown();
    
    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void SetMaxPlayers(int32 MaxPlayers);
    
    UFUNCTION(BlueprintPure, Category = "Lobby")
    bool IsHost() const;
    
    UFUNCTION(BlueprintPure, Category = "Lobby")
    bool IsReady() const { return bIsReady; }
    
    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void CreateLobbyWidget();
    
    UFUNCTION(BlueprintPure, Category = "Lobby")
    class ULobbyWidget* GetLobbyWidget() const { return LobbyWidget; }
    
protected:
    UPROPERTY(ReplicatedUsing = OnRep_IsReady)
    bool bIsReady = false;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
    TSubclassOf<class ULobbyWidget> LobbyWidgetClass;
    
    UPROPERTY()
    class ULobbyWidget* LobbyWidget;
    
    UFUNCTION()
    void OnRep_IsReady();
    
private:
    void SetupInputMode();
    
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
