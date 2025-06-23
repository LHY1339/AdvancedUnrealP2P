#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "P2PLibrary.generated.h"

UCLASS()
class UNREALP2P_API UP2PLibrary : public UObject
{
	GENERATED_BODY()

public:
	static TArray<FString> BreakString(FString BaseStr, FString Breaker);
};
