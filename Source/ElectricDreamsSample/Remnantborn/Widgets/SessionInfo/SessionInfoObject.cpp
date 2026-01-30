#include "SessionInfoObject.h"

USessionInfoObject::USessionInfoObject()
{
	SessionName = TEXT("");
	PlayerCount = TEXT("");
	Ping = TEXT("");
	SessionIndex = -1;
}

void USessionInfoObject::Setup(const FString& Name, const FString& Players, const FString& PingStr, int32 Index)
{
	SessionName = Name;
	PlayerCount = Players;
	Ping = PingStr;
	SessionIndex = Index;
}