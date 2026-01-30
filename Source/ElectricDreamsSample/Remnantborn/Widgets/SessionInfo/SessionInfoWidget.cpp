#include "SessionInfoWidget.h"
#include "SessionInfoObject.h"
#include "Components/ListViewBase.h"

void USessionInfoWidget::NativeConstruct()
{
	Super::NativeConstruct();
    
	// Don't initialize with default text - wait for data to be set
	if (SessionNameText)
	{
		SessionNameText->SetText(FText::FromString(TEXT("Loading...")));
	}
    
	if (PlayerCountText)
	{
		PlayerCountText->SetText(FText::FromString(TEXT("-")));
	}
    
	if (PingText)
	{
		PingText->SetText(FText::FromString(TEXT("-")));
	}
}

void USessionInfoWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	if (USessionInfoObject* SessionInfo = Cast<USessionInfoObject>(ListItemObject))
	{
		UpdateWidget(SessionInfo->SessionName, SessionInfo->PlayerCount, SessionInfo->Ping, SessionInfo->SessionIndex);
	}
}

void USessionInfoWidget::UpdateWidget(const FString& Name, const FString& Players, const FString& Ping, int32 Index)
{
	if (SessionNameText)
	{
		SessionNameText->SetText(FText::FromString(Name));
	}
    
	if (PlayerCountText)
	{
		PlayerCountText->SetText(FText::FromString(Players));
	}
    
	if (PingText)
	{
		PingText->SetText(FText::FromString(Ping));
	}
    
	SessionIndex = Index;
}