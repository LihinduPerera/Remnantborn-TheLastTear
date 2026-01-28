#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InGameMenuWidget.generated.h"

UCLASS()
class ELECTRICDREAMSSAMPLE_API UInGameMenuWidget : public UUserWidget
{
	GENERATED_BODY()
    
public:
	virtual void NativeConstruct() override;
    
	UFUNCTION()
	void OnReturnToMenuClicked();
    
	UFUNCTION()
	void OnQuitGameClicked();
    
	UFUNCTION()
	void OnCancelClicked();
    
	UFUNCTION(BlueprintCallable)
	void ToggleMenu();
    
protected:
	UPROPERTY(meta = (BindWidget))
	class UButton* ReturnToMenuButton;
    
	UPROPERTY(meta = (BindWidget))
	class UButton* QuitGameButton;
    
	UPROPERTY(meta = (BindWidget))
	class UButton* CancelButton;
    
private:
	bool bIsMenuOpen = false;
};