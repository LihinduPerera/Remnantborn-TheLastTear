#include "SessionInfoWidget.h"
#include "SessionInfoObject.h"
#include "Components/ListViewBase.h"

void USessionInfoWidget::NativeConstruct()
{
	Super::NativeConstruct();
    
	// Initialize widget
	if (SessionNameText)
	{
		SessionNameText->SetText(FText::FromString(TEXT("Unknown Session")));
	}
    
	if (PlayerCountText)
	{
		PlayerCountText->SetText(FText::FromString(TEXT("0/0")));
	}
    
	if (PingText)
	{
		PingText->SetText(FText::FromString(TEXT("0 ms")));
	}
}

void USessionInfoWidget::SetItem(UObject* Item)
{
	if (USessionInfoObject* SessionInfo = Cast<USessionInfoObject>(Item))
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