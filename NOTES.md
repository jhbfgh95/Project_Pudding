# ServerTest 학습·작업 노트

이 문서는 구현 과정을 이해하고 다시 재현하기 위한 기록입니다. 기능을 구현하기 전에는 계획을 적고, 사용자 동의 후 구현을 시작합니다.

## 프로젝트 기준

- 엔진: Unreal Engine (버전 확인 필요)
- 네트워크: Listen Server
- 온라인 서비스: Steam 연동 예정
- 구현 방식: 그래픽·머티리얼 등 에셋 작업을 제외하고 약 90% C++

## 학습 진행 방식

실제 프로토타입 기능을 구현할 때는 한 번에 완성하지 않고, 아래의 작은 단위로 진행합니다.

1. 구현할 기능과 Unreal 개념(Replication, RPC, Subsystem 등)을 짧게 설명합니다.
2. 사용자에게 직접 구현해 볼 부분과 Codex가 구현을 도울 부분을 구분합니다.
3. 사용자가 코드를 작성하면 코드 흐름과 오류를 함께 검토합니다. 필요하면 사용자 승인 뒤 보완합니다.
4. Listen Server 호스트와 원격 클라이언트 양쪽에서 테스트합니다.
5. 변경 내용, 네트워크 흐름, 테스트 결과, 다음 학습 항목을 이 문서에 기록합니다.

### 다음 학습 후보

| 순서 | 주제 | 먼저 익힐 개념 | 상태 |
|---|---|---|---|
| 1 | Steam 세션 전환 | OnlineSubsystem 제공자, 설정 파일, Steam 테스트 환경 | 대기 |
| 2 | 아이템 획득 | Server RPC, Replication, OnRep, 서버 권한 | 대기 |
| 3 | 점프 동기화 | CharacterMovementComponent, 서버 권한 이동 | 대기 |
| 4 | 간이 전투 | 피해 요청, Replicated 체력, OnRep UI 갱신 | 대기 |

## 진행 상태

| 기능 | 상태 | 다음 단계 | 메모 |
|---|---|---|---|
| Steam 연동 | 시작 전 | 플러그인·설정 구조 설명 | |
| 방 생성/참가 | 1단계 구현 | 로컬(NULL) 세션 생성·검색·참가 로그 확인 | Steam 전환 전의 API 흐름 검증 |
| 아이템 획득 동기화 | 시작 전 | 서버 권한·Replication 설계 | |
| 점프 동기화 | 시작 전 | CharacterMovement 확인 | |
| 간이 전투 | 시작 전 | 체력·피해 흐름 설계 | |

## 구현 전 확인 기록

각 기능은 아래 양식으로 제안하고 사용자 동의를 받은 뒤 구현합니다.

### 기능명

- 목표:
- 구현 방식:
- 핵심 개념:
- 수정 예정 파일:
- 테스트 방법:
- 사용자 동의: 대기 / 승인 / 보류

## 학습 메모

### Listen Server

플레이어 한 명이 서버와 클라이언트를 동시에 맡는 방식입니다. 호스트가 게임을 열고, 다른 플레이어가 그 호스트에게 접속합니다. 게임 규칙에 영향을 주는 최종 상태는 호스트 서버가 결정해야 합니다.

### Replication

서버가 가진 변수 값을 접속한 클라이언트들에게 복제하는 기능입니다. 예를 들어 아이템을 누가 획득했는지는 서버에서 변경하고, 해당 결과를 모든 클라이언트가 받아야 합니다.

### RPC

서버와 클라이언트 사이에서 함수를 호출하는 방법입니다. 클라이언트가 아이템 획득을 요청하면 Server RPC로 서버에 요청하고, 서버가 판정한 결과를 Replication 또는 Client/NetMulticast RPC로 보여줄 수 있습니다.

### 1단계: 세션 시스템

- 목표: Steam 없이 세션 생성, 검색, 참가, 종료 API의 비동기 흐름을 확인합니다.
- 구현 방식: `USessionSubsystem`이 `UGameInstanceSubsystem`으로서 `IOnlineSession`을 관리합니다. 로컬 검증에는 `OnlineSubsystem=Null`과 LAN 검색을 사용했고, 이후 Steam으로 전환했습니다.
- 핵심 개념: OnlineSubsystem API는 요청 직후 완료되지 않습니다. 완료 델리게이트에서 결과를 받아야 합니다.
- 수정 파일: `ServerTest.Build.cs`, `DefaultEngine.ini`, `SessionSubsystem.h/.cpp`, `ServerTestGameInstance.h/.cpp`, `NOTES.md`
- 테스트 방법: Output Log에서 `LogServerTestSession`을 필터링하고 `HostSession`, `SearchSessions`, `JoinFirstSession`, `LeaveSession` 콘솔 명령을 실행합니다.
- 사용자 동의: 승인 (2026-07-18)

### 2단계: Steam Online Subsystem 전환

- 목표: 같은 세션 코드가 Steam 세션과 Steam NetDriver를 사용하도록 전환합니다.
- 구현 방식: `OnlineSubsystemSteam` 플러그인·동적 모듈을 활성화하고, `DefaultPlatformService=Steam`, Steam NetDriver, 개발용 App ID 480, 세션 초기화 옵션을 설정합니다. 세션에는 프로젝트 전용 검색 키를 광고하고 검색에도 같은 키를 적용합니다.
- 보완: Steam Lobby 생성 방식에서는 `SEARCH_LOBBIES=true` 검색 조건을 추가합니다. 이 조건이 없으면 App ID 480의 공용 Game Server 목록을 검색하여 다른 빌드가 제외되고 결과가 0개가 될 수 있습니다.
- 보완: 기본 `IpNetDriver` 정의를 `ClearArray`로 교체한 뒤 SteamNetDriver를 `GameNetDriver`로 등록합니다. 중복 정의가 남으면 Steam 주소(`steam.<SteamId>`)를 IP 드라이버가 DNS 이름으로 해석해 `AddressResolutionFailed`가 발생합니다.
- 보완: 패키징 실행 시 `UGameEngine` 설정 섹션이 우선 적용되는 경우에도 SteamNetDriver를 선택하도록 같은 NetDriver 정의를 `[/Script/Engine.GameEngine]`에도 명시합니다.
- 진단: `NetDriverStatus` 콘솔 명령은 실행 중 적용된 `NetDriverDefinitions`와 현재 월드의 실제 NetDriver 클래스를 출력합니다. Steam P2P 주소를 `IpNetDriver`가 처리하는 원인을 확인할 때 사용합니다.
- 보완: UE 5.7의 Steam P2P 드라이버는 `OnlineSubsystemSteam.SteamNetDriver`가 아니라 Steam Sockets 플러그인의 `SteamSockets.SteamSocketsNetDriver`입니다. Steam Sockets 플러그인을 활성화하고 이 드라이버·연결 클래스로 전환했습니다.
- 핵심 개념: OnlineSubsystem 인터페이스는 유지되고, 설정된 제공자만 NULL에서 Steam으로 교체됩니다. App ID 480은 공용 테스트 ID이므로 검색 필터가 필요합니다.
- 수정 파일: `ServerTest.uproject`, `ServerTest.Build.cs`, `DefaultEngine.ini`, `SessionSubsystem.cpp`, `NOTES.md`
- 테스트 방법: Steam 클라이언트 실행 및 서로 다른 Steam 계정 두 개로 별도 프로세스를 실행합니다. `OnlineSubsystem: STEAM` 로그, 세션 생성·검색·참가·접속을 차례로 확인합니다.
- 사용자 동의: 승인 (2026-07-19)

### 3단계: 머리 밟기 점프 기반

- 목표: 공중에서 하강 중인 플레이어가 다른 플레이어의 머리를 밟으면 고정된 힘으로 다시 점프하게 합니다.
- 구현 방식: `APlayerCharacter`에 `HeadBounceCollider`(`UBoxComponent`)를 추가합니다. Overlap 이벤트는 서버에서만 판정하며, 상대가 공중 상태이고 Z축 속도가 아래 방향일 때만 `LaunchCharacter`로 고정된 Z축 속도를 적용합니다. 동일 콜라이더에서 반복 발동하지 않도록 서버 전용 쿨다운 시간을 사용합니다.
- 핵심 개념: 쿼리 전용 Overlap 콜라이더는 이동 충돌을 막지 않고 조건 판정에만 사용합니다. `LaunchCharacter`를 서버에서 실행하면 결과 이동이 캐릭터 이동 복제를 통해 클라이언트에 전달됩니다.
- 수정 파일: `PlayerCharacter.h/.cpp`, `NOTES.md`
- 테스트 방법: Listen Server 호스트와 원격 클라이언트를 실행합니다. 한 플레이어가 다른 플레이어의 머리 위로 하강할 때만 고정 높이로 튕기는지, 지상·상승 중 접촉에서는 발동하지 않는지, 짧은 시간 반복 접촉에서 쿨다운이 적용되는지 확인합니다.
- 사용자 동의: 승인 (2026-07-22)

### 4단계: PlayerController 입력과 차지 점프

- 목표: `AGGPlayerController`가 WASD, Space, E, Q 입력을 받아 소유한 `APlayerCharacter`에 전달하고, 서버 권한 차지 점프과 드롭 UI 토글 기반을 만듭니다.
- 구현 방식: `AGGPlayerController`가 블루프린트에서 지정한 `InputMappingContext`와 `InputAction`을 사용합니다. BeginPlay에서 로컬 플레이어의 Enhanced Input Subsystem에 IMC를 등록하고, SetupInputComponent에서 각 IA를 바인딩합니다. 이동은 CharacterMovement의 기본 네트워크 이동 복제를 사용합니다. 점프 차지 시작·종료는 소유 Character의 Reliable Server RPC로 보내고, 서버 시간 기준으로 0~3초를 계산한 뒤 `LaunchCharacter`를 적용합니다.
- UI: Q는 `BP_GGPlayerController`에서 지정한 `TSubclassOf<UItemWidget>`을 `CreateWidget`으로 생성·표시·숨김 처리합니다. `UUserWidget`은 ActorComponent처럼 Default Subobject로 만들 수 없으며, 로컬 PlayerController가 실행 중에 생성해 Viewport에 추가해야 합니다.
- 보류: E 입력은 Controller에 연결했지만, 실제 아이템 거리 검증과 인벤토리 추가는 `ItemActor`와 `InventoryComponent` 구현 단계에서 서버 요청으로 연결합니다.
- 수정 파일: `PlayerCharacter.h/.cpp`, `GGPlayerController.h/.cpp`, `DefaultInput.ini`, `ServerTest.Build.cs`, `NOTES.md`
- 테스트 방법: 기본 GameMode와 PlayerController를 블루프린트에서 지정한 뒤, WASD 이동과 Space 누름/뗌에 따른 점프 높이를 확인합니다. Q를 눌러 지정한 위젯이 열리고 다시 Q를 눌러 닫히는지 확인합니다.
- 사용자 동의: 승인 (2026-07-22)

### 5단계: 복제 인벤토리 기반

- 목표: 플레이어가 최대 세 종류의 아이템을 보유할 수 있는 서버 권한 인벤토리 기반을 만듭니다.
- 구현 방식: `EItemType`에 빈 슬롯을 뜻하는 `None`과 다섯 가지 프로토타입 아이템 종류를 정의합니다. `UInventoryComponent`는 길이가 세 개인 배열을 복제하며, `TryAddItem`과 `TryRemoveItemAt`은 소유 Actor가 서버 권한일 때만 값을 변경합니다.
- 핵심 개념: `ReplicatedUsing`과 `OnRep_Items`로 클라이언트가 서버의 인벤토리 변경을 받습니다. 서버는 OnRep을 받지 않으므로, 서버 변경 직후와 클라이언트 OnRep 양쪽에서 공통 `OnInventoryChanged` 이벤트를 호출합니다.
- 수정 파일: `ItemTypes.h`, `InventoryComponent.h/.cpp`, `NOTES.md`
- 테스트 방법: 이후 `PlayerCharacter`에 컴포넌트를 부착한 뒤, 서버에서 `TryAddItem`과 `TryRemoveItemAt`을 호출합니다. Listen Server 호스트와 원격 클라이언트에서 세 슬롯 값과 `OnInventoryChanged` 이벤트가 동일하게 바뀌는지 확인합니다.
- 사용자 동의: 승인 (2026-07-22)

### 6단계: Character 인벤토리·부착 슬롯 연결

- 목표: 복제 인벤토리를 플레이어 캐릭터에 부착하고, 세 인벤토리 칸에 대응하는 아이템 표시 위치를 만듭니다.
- 구현 방식: `APlayerCharacter` 생성자에서 `UInventoryComponent`와 `ItemAttachSlot0~2`(`USceneComponent`)를 만듭니다. BeginPlay에서 인벤토리 변경 이벤트를 구독하고, 서버 변경 또는 클라이언트 OnRep 수신 후 `RefreshAttachedItemVisuals`를 호출합니다.
- 현재 범위: DataTable과 시각용 아이템 메시가 없으므로 실제 메시 부착은 하지 않고, 복제된 세 슬롯 값을 로그로 확인합니다. 각 슬롯의 상대 위치는 에디터에서 조정할 수 있습니다.
- 수정 파일: `PlayerCharacter.h/.cpp`, `NOTES.md`
- 테스트 방법: 이후 서버에서 인벤토리 값을 변경했을 때 호스트와 원격 클라이언트의 로그에 같은 세 슬롯 값이 표시되는지 확인합니다.
- 사용자 동의: 승인 (2026-07-22)

### 7단계: 서버 권한 ItemActor 기반

- 목표: 레벨에 배치한 아이템이 서버의 거리·상태·인벤토리 공간 검사를 통과한 경우에만 획득되도록 만듭니다.
- 구현 방식: `AItemActor`는 `EItemType`, 획득 가능 상태, 서버 거리 검사 범위, `ItemMesh`, `PickupCollision`을 가집니다. 서버 전용 `TryPickup`이 `CanBePickedUpBy`로 유효성·거리·인벤토리 여유를 검사한 뒤 `InventoryComponent::TryAddItem`을 호출하고 성공하면 Actor를 제거합니다.
- 핵심 개념: ItemActor는 복제 Actor이며, 서버가 Destroy하면 모든 클라이언트의 월드에서도 사라집니다. `bIsAvailable`은 제거 전 상태 변경과 이후 리스폰 기능을 위한 복제 안전장치입니다.
- 수정 파일: `ItemActor.h/.cpp`, `NOTES.md`
- 테스트 방법: 다음 단계의 Server RPC 연결 뒤, 호스트와 원격 클라이언트에서 아이템에 가까울 때만 획득되는지, 가득 찬 인벤토리에서는 남아 있는지, 한 플레이어가 획득하면 모두의 월드에서 사라지는지 확인합니다.
- 사용자 동의: 승인 (2026-07-22)

### 8단계: E 입력 서버 획득 요청 연결

- 목표: E 입력이 가까운 ItemActor를 대상으로 서버 권한 획득 요청을 보내도록 연결합니다.
- 구현 방식: 로컬 `PlayerController`는 현재 Character와 ItemActor의 거리를 비교해 획득 범위 안에서 가장 가까운 후보를 찾습니다. `ServerRequestPickup`은 Reliable Server RPC이며, 서버 Controller가 소유 Character와 대상 ItemActor를 다시 가져와 `TryPickup`을 호출합니다.
- 핵심 개념: 클라이언트가 보낸 Actor 참조는 신뢰하지 않습니다. `ItemActor::TryPickup`이 서버에서 다시 거리·아이템 상태·인벤토리 공간을 검사하므로, 원격 클라이언트도 서버 검증을 우회할 수 없습니다.
- 수정 파일: `GGPlayerController.h/.cpp`, `ItemActor.h/.cpp`, `NOTES.md`
- 테스트 방법: 레벨에 ItemActor를 배치하고 ItemType·PickupRange·메시를 설정합니다. 호스트와 원격 클라이언트가 각각 E를 눌러 가까운 아이템만 획득하는지, 세 칸이 찬 상태에서 아이템이 남는지, 한쪽 획득 뒤 양쪽에서 아이템이 사라지는지 확인합니다.
- 사용자 동의: 승인 (2026-07-22)

### 9단계: 탑뷰 카메라·부착 아이템 시각화

- 목표: PlayerCharacter에 탑뷰 카메라를 구성하고, 복제 인벤토리의 세 슬롯을 캐릭터에 붙은 메시로 표시합니다.
- 구현 방식: `SpringArm`과 `Camera`를 C++ 생성자로 만들고, -50도·2200 길이·75 FOV의 고정 탑뷰 값을 적용합니다. 인벤토리 슬롯은 `EItemType`과 획득 당시 ItemActor에 지정된 `UStaticMesh` 참조를 함께 복제합니다. `ItemAttachSlot0~2` 아래의 `ItemVisual0~2`는 그 복제 메시를 설정하거나 숨깁니다.
- 핵심 개념: 월드 ItemActor를 제거한 뒤에도 인벤토리에 복제된 메시 참조가 남으므로, 호스트와 원격 클라이언트 모두 같은 아이템 모양을 표시합니다. 이후 DataTable을 도입하면 메시 참조 대신 ItemId로 데이터를 조회하도록 확장할 수 있습니다.
- 수정 파일: `PlayerCharacter.h/.cpp`, `NOTES.md`
- 테스트 방법: 레벨에 배치한 ItemActor의 `ItemMesh`에 메시를 지정합니다. 호스트와 원격 클라이언트에서 아이템 획득 후 동일한 부착 슬롯에 같은 메시가 보이는지 확인합니다.
- 사용자 동의: 승인 (2026-07-22)

### 10단계: GameMode 기본 플레이어 클래스 지정

- 목표: GameMode가 접속 플레이어를 위해 프로젝트의 Character와 PlayerController를 자동 생성하도록 설정합니다.
- 구현 방식: `AGGGameMode` 생성자에서 `DefaultPawnClass`에 `APlayerCharacter`, `PlayerControllerClass`에 `AGGPlayerController`를 지정합니다.
- 핵심 개념: GameMode는 서버에만 존재합니다. 클라이언트는 GameMode를 직접 보유하지 않으며, 서버가 결정한 Pawn과 Controller 생성 결과를 네트워크를 통해 받습니다.
- 수정 파일: `GGGameMode.h/.cpp`, `NOTES.md`
- 테스트 방법: 적용된 GameMode로 PIE Listen Server를 실행합니다. 호스트와 클라이언트가 모두 `APlayerCharacter`를 Pawn으로, `AGGPlayerController`를 Controller로 생성하는지 확인합니다.
- 사용자 동의: 승인 (2026-07-22)

### 1-1단계: 세션 참가 후 실제 접속

- 목표: 세션 API의 참가 성공을 실제 Listen Server 접속으로 연결합니다.
- 구현 방식: 호스트는 세션 생성 완료 뒤 기본 맵을 `?listen`으로 열고, 클라이언트는 `GetResolvedConnectString`으로 얻은 주소에 `ClientTravel`합니다.
- 핵심 개념: 세션 참가와 맵 이동은 별도 작업입니다. 접속 주소는 OnlineSubsystem이 해석하며, 클라이언트가 임의로 만들지 않습니다.
- 수정 파일: `SessionSubsystem.cpp`, `NOTES.md`
- 테스트 방법: 별도 프로세스의 호스트에서 `HostSession`, 클라이언트에서 `SearchSessions` 후 `JoinFirstSession`을 실행합니다. 클라이언트가 호스트 맵으로 이동하는지 양쪽 로그에서 확인합니다.
- 사용자 동의: 승인 (2026-07-19)

## 테스트 기록

| 날짜 | 기능 | 환경 | 결과 | 관찰한 내용 |
|---|---|---|---|---|
| 2026-07-19 | 로컬 세션 API | 별도 프로세스 / NULL | 성공 | `HostSession` → `SearchSessions` → `JoinFirstSession` 성공 확인 |
| 2026-07-19 | 세션 참가 후 접속 | 별도 프로세스 / NULL | 성공 | 호스트 주소 해석 후 `ClientTravel`로 호스트 Listen Server 맵 접속 확인 |
| 2026-07-19 | Steam 세션 전환 | 별도 프로세스 / Steam | 미확인 | Steam 클라이언트와 서로 다른 계정으로 확인 필요 |

## 이슈 및 질문

| 날짜 | 내용 | 재현 절차 | 원인/가설 | 다음 확인 |
|---|---|---|---|---|
| | | | | |

## 자주 확인할 항목

- PIE에서 플레이어 수를 2 이상으로 설정한 뒤 호스트와 클라이언트 양쪽 결과를 비교합니다.
- `Output Log`에서 서버와 클라이언트 로그를 구분하여 확인합니다.
- Steam 연동 전에는 로컬 세션으로 흐름을 확인하고, 이후 Steam 환경에서 다시 확인합니다.
