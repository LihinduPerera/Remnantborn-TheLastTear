#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "SessionInfoWidget.generated.h"

class USessionInfoObject;

UCLASS()
class ELECTRICDREAMSSAMPLE_API USessionInfoWidget : public UUserWidget
{
	GENERATED_BODY()
    
public:
	// Native construct
	virtual void NativeConstruct() override;
    
	// Called when the list item is set
	UFUNCTION(BlueprintCallable)
	void SetItem(UObject* Item);
    
	// Called to update the widget with data
	UFUNCTION(BlueprintCallable)
	void UpdateWidget(const FString& Name, const FString& Players, const FString& Ping, int32 Index);
    
	// Get the session index
	UFUNCTION(BlueprintCallable)
	int32 GetSessionIndex() const { return SessionIndex; }
    
protected:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* SessionNameText;
    
	UPROPERTY(meta = (BindWidget))
	UTextBlock* PlayerCountText;
    
	UPROPERTY(meta = (BindWidget))
	UTextBlock* PingText;
    
private:
	int32 SessionIndex = -1;
};