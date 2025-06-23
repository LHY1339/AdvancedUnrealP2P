#pragma once

#include "CoreMinimal.h"
#include "P2PObject.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "P2PSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSessionListChange);

UCLASS()
class UNREALP2P_API UP2PSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	//重写UGameInstanceSubsystem的函数
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	//重写FTickableGameObject的函数
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

	//公开函数
	UFUNCTION(BlueprintCallable, Category="P2P")
	void SetLocalSessionPassword(FString Password);

	UFUNCTION(BlueprintCallable, Category="P2P")
	void SetLocalSessionProperty(FString Property);

	UFUNCTION(BlueprintCallable, Category="P2P")
	void SetServer(FString IP, int Port);

	UFUNCTION(BlueprintCallable, Category="P2P")
	void ConnectSession(FString IP, int Port, FString Password);

	UFUNCTION(BlueprintCallable, Category="P2P")
	void FlushSessionList();

private:
	//Tick相关函数
	void TickRecv(float DeltaTime);
	void TickTemp(float DeltaTime);
	void TickServer(float DeltaTime);

	//Socket相关函数
	void InitMessageSocket();
	void InitListenSocket();
	bool SendMessage(FSocket* InSocket, FString Message, FString TargetIP, int TargetPort);

	//接收函数
	void HandleMessage(FString Message);
	void CmdID(TArray<FString> ParamList, FString Message);
	void CmdPing(TArray<FString> ParamList, FString Message);
	void CmdSession(TArray<FString> ParamList, FString Message);

public:
	FSocket* ListenSocket;
	FSocket* MessageSocket;

	int MaxTickServerWaitTime = 1.0f;
	int MaxTickTempWaitTime = 1.0f;

	FString ServerIP = "";
	int ServerPort = 0;

	UPROPERTY(BlueprintReadOnly, Category="P2P")
	FLocalSession LocalSession;

	UPROPERTY(BlueprintReadOnly, Category="P2P")
	TArray<FListSession> SessionList;

	UPROPERTY(BlueprintAssignable, Category="P2P")
	FOnSessionListChange OnSessionListChange;

private:
	bool WasInit = false;

	float TickServerWaitTime = 0.0f;
	float TickTempWaitTime = 0.0f;

	TArray<FTempMessage> TempMessageList;
};
