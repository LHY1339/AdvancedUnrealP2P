#include "P2PSubsystem.h"

#include "SocketSubsystem.h"
#include "Common/UdpSocketBuilder.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "IpNetDriver.h"
#include "P2PLibrary.h"

//初始化
void UP2PSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	WasInit = true;
}

//Tick函数
void UP2PSubsystem::Tick(float DeltaTime)
{
	TickRecv(DeltaTime);
	TickTemp(DeltaTime);
	if (UKismetSystemLibrary::IsServer(GetWorld()) &&
		!UKismetSystemLibrary::IsStandalone(GetWorld()))
	{
		TickServer(DeltaTime);
	}
}

//是否执行Tick
bool UP2PSubsystem::IsTickable() const
{
	return WasInit;
}

//stat调试信息，返回默认值就行
TStatId UP2PSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UP2PSubsystem, STATGROUP_Tickables);
}

//设置Session.Password
void UP2PSubsystem::SetLocalSessionPassword(FString Password)
{
	LocalSession.Password = Password;
}

//设置Session.Property
void UP2PSubsystem::SetLocalSessionProperty(FString Property)
{
	LocalSession.Property = Property;
}

//设置目标服务器
void UP2PSubsystem::SetServer(FString IP, int Port)
{
	ServerIP = IP;
	ServerPort = Port;
}

//连接其他Session
void UP2PSubsystem::ConnectSession(FString IP, int Port, FString Password)
{
	//直接连接Session
	GetWorld()->GetFirstPlayerController()->ClientTravel(FString::Printf(TEXT("%s:%d"), *IP, Port), ETravelType::TRAVEL_Absolute);

	//添加TempMessage，告诉服务器让对方Ping我一下
	FTempMessage new_message;
	new_message.IP = ServerIP;
	new_message.Port = ServerPort;
	new_message.Message = FString::Printf(TEXT("@PingMe#%s#%d#%s#\0"), *IP, Port, *Password);
	TempMessageList.Add(new_message);
}

//刷新Session列表
void UP2PSubsystem::FlushSessionList()
{
	InitMessageSocket();
	SendMessage(MessageSocket, "@GetSession#", ServerIP, ServerPort);
}

//接收消息
void UP2PSubsystem::TickRecv(float DeltaTime)
{
	InitMessageSocket();
	uint8 Data[2048];
	int32 BytesReceived = 0;
	if (MessageSocket->Recv(Data, sizeof(Data), BytesReceived))
	{
		FString ReceivedString = FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(Data)));
		HandleMessage(ReceivedString);
	}
}

//缓存区，有序定时发送消息
void UP2PSubsystem::TickTemp(float DeltaTime)
{
	//定时器
	TickTempWaitTime += DeltaTime;
	if (TickTempWaitTime <= MaxTickTempWaitTime)
	{
		return;
	}
	TickTempWaitTime = 0.0f;

	//判断是否需要发送消息
	if (TempMessageList.Num() <= 0)
	{
		return;
	}
	InitListenSocket();
	if (!ListenSocket)
	{
		return;
	}

	//发送消息
	const FTempMessage temp = TempMessageList[0];
	SendMessage(ListenSocket, temp.Message, temp.IP, temp.Port);
	TempMessageList.RemoveAt(0);
}

//Session端逻辑，上传信息
void UP2PSubsystem::TickServer(float DeltaTime)
{
	//定时器
	TickServerWaitTime += DeltaTime;
	if (TickServerWaitTime <= MaxTickServerWaitTime)
	{
		return;
	}
	TickServerWaitTime = 0.0f;

	//MessageSocket在服务端注册
	InitMessageSocket();
	SendMessage(MessageSocket, "@MessageRegister#\0", ServerIP, ServerPort);

	//注册成功
	if (LocalSession.ID > 0)
	{
		//ListenSocket将基本信息发送到服务端
		InitListenSocket();
		const int session_id = LocalSession.ID; //ID
		const FString session_password = LocalSession.Password.IsEmpty() ? "false" : "true"; //是否使用Password
		const FString session_property = LocalSession.Property; //其他属性
		const FString send_message = FString::Printf(TEXT("@ListenRegister#%d#%s#%s#\0"), session_id, *session_password, *session_property);
		SendMessage(ListenSocket, send_message, ServerIP, ServerPort);
	}
}

//创建&&初始化MessageSocket
void UP2PSubsystem::InitMessageSocket()
{
	if (MessageSocket)
	{
		return;
	}
	MessageSocket = FUdpSocketBuilder(TEXT("MessageSocket"))
	                .AsReusable()
	                .WithBroadcast()
	                .WithSendBufferSize(512);

	TSharedRef<FInternetAddr> LocalAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	LocalAddr->SetAnyAddress();
	LocalAddr->SetPort(0);
	MessageSocket->Bind(*LocalAddr);
}

//查找ListenSocket
void UP2PSubsystem::InitListenSocket()
{
	TArray<FNamedNetDriver> drivers = GEngine->GetWorldContextFromWorldChecked(GetWorld()).ActiveNetDrivers;
	for (const FNamedNetDriver& DriverEntry : drivers)
	{
		UIpNetDriver* ip_net_driver = Cast<UIpNetDriver>(DriverEntry.NetDriver);
		if (ip_net_driver && ip_net_driver->GetSocket())
		{
			ListenSocket = ip_net_driver->GetSocket();
			return;
		}
	}
}

//发送消息
bool UP2PSubsystem::SendMessage(FSocket* InSocket, FString Message, FString TargetIP, int TargetPort)
{
	if (!InSocket || Message.IsEmpty())
	{
		return false;
	}

	FIPv4Address RemoteIP;
	FIPv4Address::Parse(TargetIP, RemoteIP);

	TSharedRef<FInternetAddr> RemoteAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	RemoteAddr->SetIp(RemoteIP.Value);
	RemoteAddr->SetPort(TargetPort);

	FTCHARToUTF8 Converted(*Message);

	TArray<uint8> Data;
	Data.Append((uint8*)Converted.Get(), Converted.Length());

	int32 BytesSent = 0;
	bool bSendSuccess = InSocket->SendTo(Data.GetData(), Data.Num(), BytesSent, *RemoteAddr);
	return bSendSuccess;
}

//分配接收的消息
void UP2PSubsystem::HandleMessage(FString Message)
{
	TArray<FString> param_list = UP2PLibrary::BreakString(Message, "#");
	if (param_list.Num() < 1)
	{
		return;
	}
	if (param_list[0] == "@ID")
	{
		CmdID(param_list, Message);
	}
	else if (param_list[0] == "@Ping")
	{
		CmdPing(param_list, Message);
	}
	else if (param_list[0] == "@Session")
	{
		CmdSession(param_list, Message);
	}
}

//设置ID
void UP2PSubsystem::CmdID(TArray<FString> ParamList, FString Message)
{
	if (ParamList.Num() < 2)
	{
		return;
	}
	LocalSession.ID = FCString::Atoi(*ParamList[1]);
}

//Ping
void UP2PSubsystem::CmdPing(TArray<FString> ParamList, FString Message)
{
	if (ParamList.Num() < 4)
	{
		return;
	}
	if (LocalSession.Password.IsEmpty() || LocalSession.Password == ParamList[3])
	{
		const FString ip = ParamList[1];
		const int port = FCString::Atoi(*ParamList[2]);
		InitListenSocket();
		SendMessage(ListenSocket, "@\0", ip, port);
	}
}

//设置Session列表
void UP2PSubsystem::CmdSession(TArray<FString> ParamList, FString Message)
{
	SessionList.Empty();
	for (int i = 0; i < ParamList.Num(); i++)
	{
		TArray<FString> session_param_list = UP2PLibrary::BreakString(ParamList[i], "/");
		if (session_param_list.Num() < 5)
		{
			continue;
		}
		FListSession new_session;
		new_session.IP = session_param_list[0];
		new_session.Port = FCString::Atoi(*session_param_list[1]);
		new_session.ID = FCString::Atoi(*session_param_list[2]);
		new_session.UsePassword = session_param_list[3] == "true";
		new_session.Property = session_param_list[4];
		SessionList.Add(new_session);
	}
	OnSessionListChange.Broadcast();
}
