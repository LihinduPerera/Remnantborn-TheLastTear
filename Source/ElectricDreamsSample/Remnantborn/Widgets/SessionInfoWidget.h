#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "SessionInfoWidget.generated.h"

UCLASS()
class ELECTRICDREAMSSAMPLE_API USessionInfoWidget : public UUserWidget
{
	GENERATED_BODY()
    
public:
	UFUNCTION(BlueprintCallable)
	void Setup(const FString& Name, const FString& Players, const FString& Ping);
    
protected:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* SessionNameText;
    
	UPROPERTY(meta = (BindWidget))
	UTextBlock* PlayerCountText;
    
	UPROPERTY(meta = (BindWidget))
	UTextBlock* PingText;
};