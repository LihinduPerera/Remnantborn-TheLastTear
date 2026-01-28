#include "SessionInfoWidget.h"

void USessionInfoWidget::Setup(const FString& Name, const FString& Players, const FString& Ping)
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
}