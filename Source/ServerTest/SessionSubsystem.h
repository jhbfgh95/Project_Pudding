// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "SessionSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FServerTestSessionOperationComplete, bool /* bSucceeded */);

/** 방 생성, 검색, 참가, 종료의 비동기 흐름을 관리합니다. */
UCLASS()
class SERVERTEST_API USessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	void CreateSession(int32 NumPublicConnections = 4);
	void FindSessions(int32 MaxSearchResults = 100);
	void JoinFirstFoundSession();
	void DestroySession();
	FServerTestSessionOperationComplete OnCreateSessionComplete;
	FServerTestSessionOperationComplete OnFindSessionsComplete;
	FServerTestSessionOperationComplete OnJoinSessionComplete;
	FServerTestSessionOperationComplete OnDestroySessionComplete;

private:
	void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleFindSessionsComplete(bool bWasSuccessful);
	void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	IOnlineSessionPtr SessionInterface;
	TSharedPtr<FOnlineSessionSearch> SessionSearch;
	FDelegateHandle CreateSessionCompleteDelegateHandle;
	FDelegateHandle FindSessionsCompleteDelegateHandle;
	FDelegateHandle JoinSessionCompleteDelegateHandle;
	FDelegateHandle DestroySessionCompleteDelegateHandle;
};
