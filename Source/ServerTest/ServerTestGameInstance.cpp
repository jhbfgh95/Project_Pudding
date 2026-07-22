// Copyright Epic Games, Inc. All Rights Reserved.

#include "ServerTestGameInstance.h"
#include "Engine/Engine.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "SessionSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogServerTestNetwork, Log, All);

void UServerTestGameInstance::HostSession(int32 MaxPlayers) { GetSubsystem<USessionSubsystem>()->CreateSession(MaxPlayers); }
void UServerTestGameInstance::SearchSessions() { GetSubsystem<USessionSubsystem>()->FindSessions(); }
void UServerTestGameInstance::JoinFirstSession() { GetSubsystem<USessionSubsystem>()->JoinFirstFoundSession(); }
void UServerTestGameInstance::LeaveSession() { GetSubsystem<USessionSubsystem>()->DestroySession(); }

void UServerTestGameInstance::NetDriverStatus()
{
	if (GEngine == nullptr)
	{
		UE_LOG(LogServerTestNetwork, Error, TEXT("GEngine을 찾지 못했습니다."));
		return;
	}

	UE_LOG(LogServerTestNetwork, Log, TEXT("=== 적용된 NetDriverDefinitions (%d개) ==="), GEngine->NetDriverDefinitions.Num());
	for (const FNetDriverDefinition& Definition : GEngine->NetDriverDefinitions)
	{
		UE_LOG(LogServerTestNetwork, Log, TEXT("Def=%s, Primary=%s, Fallback=%s"),
			*Definition.DefName.ToString(),
			*Definition.DriverClassName.ToString(),
			*Definition.DriverClassNameFallback.ToString());
	}

	UWorld* World = GetWorld();
	UNetDriver* NetDriver = World != nullptr ? World->GetNetDriver() : nullptr;
	if (NetDriver == nullptr)
	{
		UE_LOG(LogServerTestNetwork, Warning, TEXT("현재 월드에 생성된 NetDriver가 없습니다."));
		return;
	}

	UE_LOG(LogServerTestNetwork, Log, TEXT("현재 월드 NetDriver: %s (Definition=%s)"),
		*NetDriver->GetClass()->GetPathName(),
		*NetDriver->GetNetDriverDefinition().ToString());
}
