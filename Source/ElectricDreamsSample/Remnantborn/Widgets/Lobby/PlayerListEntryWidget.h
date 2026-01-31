#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerListEntryWidget.generated.h"

UCLASS()
class ELECTRICDREAMSSAMPLE_API UPlayerListEntryWidget : public UUserWidget
{
    GENERATED_BODY()
    
public:
    UFUNCTION(BlueprintCallable)
    void SetPlayerInfo(const FString& PlayerName, bool bIsReady);
    
protected:
    UPROPERTY(meta = (BindWidget))
    class UTextBlock* PlayerNameText;
    
    UPROPERTY(meta = (BindWidget))
    class UTextBlock* ReadyStatusText;
};
