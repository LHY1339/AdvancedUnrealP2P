#pragma once

#include "CoreMinimal.h"
#include "P2PObject.generated.h"

//列表Session结构体
USTRUCT(BlueprintType)
struct FListSession
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="FListSession")
	FString IP = "";

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="FListSession")
	int Port = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="FListSession")
	int ID = -1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="FListSession")
	bool UsePassword = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="FListSession")
	FString Property = "";

	bool operator==(const FListSession& Other);
};

//本地Session结构体
USTRUCT(BlueprintType)
struct FLocalSession
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="FLocalSession")
	int ID = -1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="FLocalSession")
	FString Password = "";

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="FLocalSession")
	FString Property = "";
};

//临时消息结构体
USTRUCT()
struct FTempMessage
{
	GENERATED_BODY()

	UPROPERTY()
	FString IP;

	UPROPERTY()
	int Port;

	UPROPERTY()
	FString Message = "";
};
