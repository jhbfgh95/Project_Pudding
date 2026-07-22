# 로컬 세션 API 안내

## 한 줄 요약

이번 단계에서 새 웹 API나 Steam Web API를 만든 것은 아닙니다. Unreal Engine의 **OnlineSubsystem 세션 API**를 C++에서 호출하고, 프로젝트에서 쓰기 쉽도록 `USessionSubsystem`으로 감싼 것입니다.

현재 설정은 `OnlineSubsystem=Null`입니다. 즉 Steam 없이 로컬 LAN 세션의 생성·검색·참가 요청 흐름을 학습하는 단계입니다.

## API는 두 층으로 나뉩니다

| 구분 | 위치 | 역할 |
|---|---|---|
| Unreal Engine API | `IOnlineSession` | 실제 세션 생성, 검색, 참가, 종료를 비동기로 요청합니다. |
| 프로젝트 API | `USessionSubsystem` | 엔진 API를 호출하고 결과 로그와 델리게이트를 관리합니다. |
| 테스트 명령 | `UServerTestGameInstance` | 에디터 콘솔에서 프로젝트 API를 쉽게 실행하게 합니다. |

```text
콘솔 명령
  ↓
UServerTestGameInstance
  ↓
USessionSubsystem
  ↓
IOnlineSession (Unreal OnlineSubsystem)
  ↓
NULL 서비스 (현재) → Steam 서비스 (다음 단계)
```

## 현재 프로젝트에서 제공하는 함수

### 테스트용 콘솔 명령

PIE 실행 중 콘솔(`~`)에 아래 명령을 입력합니다.

| 명령 | 호출되는 프로젝트 함수 | 의미 |
|---|---|---|
| `HostSession` | `CreateSession(4)` | 최대 4명인 로컬 방 생성을 요청합니다. |
| `HostSession 2` | `CreateSession(2)` | 최대 2명인 로컬 방 생성을 요청합니다. |
| `SearchSessions` | `FindSessions()` | 같은 LAN에서 광고 중인 방을 검색합니다. |
| `JoinFirstSession` | `JoinFirstFoundSession()` | 검색 결과의 첫 번째 방에 참가 요청을 보냅니다. |
| `LeaveSession` | `DestroySession()` | 현재 세션 종료를 요청합니다. |

### `USessionSubsystem`의 C++ 함수

```cpp
void CreateSession(int32 NumPublicConnections = 4);
void FindSessions(int32 MaxSearchResults = 100);
void JoinFirstFoundSession();
void DestroySession();
```

다른 C++ 클래스에서는 게임 인스턴스에서 서브시스템을 가져와 호출할 수 있습니다.

```cpp
USessionSubsystem* SessionSubsystem = GetGameInstance()->GetSubsystem<USessionSubsystem>();
SessionSubsystem->CreateSession(4);
```

## 내부에서 호출하는 Unreal Engine API

`USessionSubsystem`은 `IOnlineSession` 인터페이스의 아래 함수를 사용합니다.

| Unreal 함수 | 사용 시점 | 완료 확인 함수 |
|---|---|---|
| `CreateSession(...)` | 방 생성 요청 | `HandleCreateSessionComplete(...)` |
| `FindSessions(...)` | 방 목록 검색 요청 | `HandleFindSessionsComplete(...)` |
| `JoinSession(...)` | 선택한 방 참가 요청 | `HandleJoinSessionComplete(...)` |
| `DestroySession(...)` | 방 종료 요청 | `HandleDestroySessionComplete(...)` |

이 함수들은 **비동기 API**입니다. 예를 들어 `CreateSession()`의 반환값이 `true`여도 그것은 “요청을 시작했다”는 뜻입니다. 실제 성공·실패는 나중에 실행되는 완료 델리게이트에서 확정됩니다.

```text
CreateSession 호출
  ↓
요청 시작 여부 반환
  ↓ (잠시 뒤)
HandleCreateSessionComplete 호출
  ↓
성공 또는 실패 로그 기록
```

## 현재 단계의 범위와 제한

- `JoinFirstSession`은 세션 참가 요청의 성공 여부까지만 확인합니다.
- 아직 `GetResolvedConnectString()`과 `ClientTravel()`을 호출하지 않으므로, 참가 성공 뒤 호스트 맵으로 자동 이동하지는 않습니다.
- 검색 결과 중 첫 번째 항목만 선택합니다. 방 목록 UI나 방 이름 필터는 아직 없습니다.
- 현재 `Null` 서비스와 LAN 검색을 사용하므로 Steam 친구 초대, Steam 로비, 인터넷 기반 탐색은 제공하지 않습니다.

다음 작은 단계에서는 참가 성공 뒤 접속 주소를 얻고 `ClientTravel()`로 호스트에게 실제 접속하도록 확장합니다.

## 로그 확인 방법

`Output Log`에서 `LogServerTestSession`을 검색하면 다음과 비슷한 흐름을 확인할 수 있습니다.

```text
OnlineSubsystem: NULL
로컬 세션 생성을 요청합니다. 최대 인원: 4
세션 생성 결과: 성공 (GameSession)
로컬 세션 검색을 요청합니다.
세션 검색 결과: 성공, 1개
첫 번째 검색 결과에 참가를 요청합니다.
세션 참가 결과: 성공 (GameSession)
```

검색 결과가 0개라면 같은 네트워크인지, 호스트가 먼저 `HostSession`을 실행했는지, 호스트가 세션을 종료하지 않았는지를 확인해 주세요.

## Steam으로 전환할 때 달라지는 부분

세션을 사용하는 게임 코드의 호출 방식은 유지하고, 다음을 바꿉니다.

1. `OnlineSubsystemSteam` 플러그인과 빌드 모듈을 활성화합니다.
2. `DefaultEngine.ini`의 서비스와 NetDriver를 Steam용으로 설정합니다.
3. LAN 설정 대신 Steam 로비·Presence 설정을 적용합니다.
4. Steam 클라이언트와 서로 다른 계정 환경에서 생성·검색·참가를 검증합니다.

Steam 설정은 네트워크 드라이버 동작에 영향을 주므로, 별도 승인 후 다음 단계에서 적용합니다.
