// Copyright Epic Games, Inc. All Rights Reserved.

#include "SessionSubsystem.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "GameMapsSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"

DEFINE_LOG_CATEGORY_STATIC(LogServerTestSession, Log, All);

namespace ServerTestSession
{
	// Steam의 공용 테스트 App ID(480)에서 이 프로젝트의 방만 찾기 위한 식별값입니다.
	const FName SearchKey(TEXT("SERVERTEST_SESSION"));
	const FString SearchValue(TEXT("SERVERTEST_V1"));
}

void USessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get())
	{
		SessionInterface = OnlineSubsystem->GetSessionInterface();
		UE_LOG(LogServerTestSession, Log, TEXT("OnlineSubsystem: %s"), *OnlineSubsystem->GetSubsystemName().ToString());
	}
	if (!SessionInterface.IsValid()) UE_LOG(LogServerTestSession, Error, TEXT("세션 인터페이스를 가져오지 못했습니다."));
}

void USessionSubsystem::Deinitialize()
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
	}
	Super::Deinitialize();
}

void USessionSubsystem::CreateSession(int32 NumPublicConnections)
{
	if (!SessionInterface.IsValid()) { OnCreateSessionComplete.Broadcast(false); return; }
	if (SessionInterface->GetNamedSession(NAME_GameSession) != nullptr)
	{
		UE_LOG(LogServerTestSession, Warning, TEXT("이미 세션이 있습니다. LeaveSession 후 다시 시도해 주세요."));
		OnCreateSessionComplete.Broadcast(false); return;
	}
	FOnlineSessionSettings Settings;
	Settings.bIsLANMatch = false;
	Settings.NumPublicConnections = NumPublicConnections;
	Settings.bShouldAdvertise = true;
	Settings.bAllowJoinInProgress = true;
	Settings.bAllowJoinViaPresence = true;
	Settings.bUsesPresence = true;
	Settings.bUseLobbiesIfAvailable = true;
	Settings.Set(ServerTestSession::SearchKey, ServerTestSession::SearchValue, EOnlineDataAdvertisementType::ViaOnlineService);
	CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleCreateSessionComplete));
	UE_LOG(LogServerTestSession, Log, TEXT("Steam 세션 생성을 요청합니다. 최대 인원: %d"), NumPublicConnections);
	if (!SessionInterface->CreateSession(0, NAME_GameSession, Settings))
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		UE_LOG(LogServerTestSession, Error, TEXT("세션 생성 요청을 시작하지 못했습니다.")); OnCreateSessionComplete.Broadcast(false);
	}
}

void USessionSubsystem::FindSessions(int32 MaxSearchResults)
{
	if (!SessionInterface.IsValid()) { OnFindSessionsComplete.Broadcast(false); return; }
	SessionSearch = MakeShared<FOnlineSessionSearch>();
	SessionSearch->bIsLanQuery = false;
	SessionSearch->MaxSearchResults = MaxSearchResults;
	// Steam에서는 공개 Game Server 목록이 아닌 Lobby 목록을 검색합니다.
	SessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	SessionSearch->QuerySettings.Set(ServerTestSession::SearchKey, ServerTestSession::SearchValue, EOnlineComparisonOp::Equals);
	FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::HandleFindSessionsComplete));
	UE_LOG(LogServerTestSession, Log, TEXT("Steam 세션 검색을 요청합니다. 프로젝트 전용 필터: %s"), *ServerTestSession::SearchValue);
	if (!SessionInterface->FindSessions(0, SessionSearch.ToSharedRef()))
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
		UE_LOG(LogServerTestSession, Error, TEXT("세션 검색 요청을 시작하지 못했습니다.")); OnFindSessionsComplete.Broadcast(false);
	}
}

void USessionSubsystem::JoinFirstFoundSession()
{
	if (!SessionInterface.IsValid() || !SessionSearch.IsValid() || SessionSearch->SearchResults.IsEmpty())
	{
		UE_LOG(LogServerTestSession, Warning, TEXT("참가할 검색 결과가 없습니다. 먼저 SearchSessions를 호출해 주세요.")); OnJoinSessionComplete.Broadcast(false); return;
	}
	JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleJoinSessionComplete));
	UE_LOG(LogServerTestSession, Log, TEXT("첫 번째 검색 결과에 참가를 요청합니다."));
	if (!SessionInterface->JoinSession(0, NAME_GameSession, SessionSearch->SearchResults[0]))
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		UE_LOG(LogServerTestSession, Error, TEXT("세션 참가 요청을 시작하지 못했습니다.")); OnJoinSessionComplete.Broadcast(false);
	}
}

void USessionSubsystem::DestroySession()
{
	if (!SessionInterface.IsValid() || SessionInterface->GetNamedSession(NAME_GameSession) == nullptr)
	{
		UE_LOG(LogServerTestSession, Warning, TEXT("종료할 세션이 없습니다.")); OnDestroySessionComplete.Broadcast(false); return;
	}
	DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleDestroySessionComplete));
	UE_LOG(LogServerTestSession, Log, TEXT("세션 종료를 요청합니다."));
	if (!SessionInterface->DestroySession(NAME_GameSession))
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
		UE_LOG(LogServerTestSession, Error, TEXT("세션 종료 요청을 시작하지 못했습니다.")); OnDestroySessionComplete.Broadcast(false);
	}
}

void USessionSubsystem::HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
	UE_LOG(LogServerTestSession, Log, TEXT("세션 생성 결과: %s (%s)"), bWasSuccessful ? TEXT("성공") : TEXT("실패"), *SessionName.ToString());
	OnCreateSessionComplete.Broadcast(bWasSuccessful);

	if (!bWasSuccessful)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		UE_LOG(LogServerTestSession, Error, TEXT("Listen Server를 시작할 월드를 찾지 못했습니다."));
		return;
	}

	const FString MapPath = UGameMapsSettings::GetGameDefaultMap();
	UE_LOG(LogServerTestSession, Log, TEXT("Listen Server로 맵을 엽니다: %s?listen"), *MapPath);
	UGameplayStatics::OpenLevel(World, FName(*MapPath), true, TEXT("listen"));
}
void USessionSubsystem::HandleFindSessionsComplete(bool bWasSuccessful)
{
	SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
	const int32 FoundCount = bWasSuccessful && SessionSearch.IsValid() ? SessionSearch->SearchResults.Num() : 0;
	UE_LOG(LogServerTestSession, Log, TEXT("세션 검색 결과: %s, %d개"), bWasSuccessful ? TEXT("성공") : TEXT("실패"), FoundCount); OnFindSessionsComplete.Broadcast(bWasSuccessful);
}
void USessionSubsystem::HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
	const bool bWasSuccessful = Result == EOnJoinSessionCompleteResult::Success;
	UE_LOG(LogServerTestSession, Log, TEXT("세션 참가 결과: %s (%s)"), bWasSuccessful ? TEXT("성공") : TEXT("실패"), *SessionName.ToString());
	OnJoinSessionComplete.Broadcast(bWasSuccessful);

	if (!bWasSuccessful)
	{
		return;
	}

	FString ConnectString;
	if (!SessionInterface->GetResolvedConnectString(SessionName, ConnectString))
	{
		UE_LOG(LogServerTestSession, Error, TEXT("세션 접속 주소를 해석하지 못했습니다."));
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	APlayerController* PlayerController = GameInstance != nullptr ? GameInstance->GetFirstLocalPlayerController() : nullptr;
	if (PlayerController == nullptr)
	{
		UE_LOG(LogServerTestSession, Error, TEXT("ClientTravel을 호출할 로컬 PlayerController를 찾지 못했습니다."));
		return;
	}

	UE_LOG(LogServerTestSession, Log, TEXT("호스트로 이동합니다: %s"), *ConnectString);
	PlayerController->ClientTravel(ConnectString, TRAVEL_Absolute);
}
void USessionSubsystem::HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
	UE_LOG(LogServerTestSession, Log, TEXT("세션 종료 결과: %s (%s)"), bWasSuccessful ? TEXT("성공") : TEXT("실패"), *SessionName.ToString()); OnDestroySessionComplete.Broadcast(bWasSuccessful);
}
