// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "ServerTestGameInstance.generated.h"

/** 콘솔 명령으로 로컬 세션 흐름을 확인하기 위한 게임 인스턴스입니다. */
UCLASS()
class SERVERTEST_API UServerTestGameInstance : public UGameInstance
{
	GENERATED_BODY()
public:
	UFUNCTION(Exec) void HostSession(int32 MaxPlayers = 4);
	UFUNCTION(Exec) void SearchSessions();
	UFUNCTION(Exec) void JoinFirstSession();
	UFUNCTION(Exec) void LeaveSession();

	/** 현재 적용된 NetDriver 정의와 월드의 실제 드라이버 클래스를 로그로 출력합니다. */
	UFUNCTION(Exec) void NetDriverStatus();
};
