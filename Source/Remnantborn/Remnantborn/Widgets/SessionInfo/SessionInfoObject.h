#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "SessionInfoObject.generated.h"

UCLASS(BlueprintType)
class REMNANTBORN_API USessionInfoObject : public UObject
{
	GENERATED_BODY()
    
public:
	UPROPERTY(BlueprintReadWrite, Category = "Session Info")
	FString SessionName;
    
	UPROPERTY(BlueprintReadWrite, Category = "Session Info")
	FString PlayerCount;
    
	UPROPERTY(BlueprintReadWrite, Category = "Session Info")
	FString Ping;
    
	UPROPERTY(BlueprintReadWrite, Category = "Session Info")
	int32 SessionIndex;
    
	// Constructor
	USessionInfoObject();
    
	// Setup function
	UFUNCTION(BlueprintCallable)
	void Setup(const FString& Name, const FString& Players, const FString& PingStr, int32 Index);
};