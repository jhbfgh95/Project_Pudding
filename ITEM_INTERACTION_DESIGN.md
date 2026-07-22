# 아이템·상호작용 프로토타입 기획

> 상태: 설계 합의 완료, 구현 전
>
> 이 문서는 아이템, 상호작용, 인벤토리, 점프 구현의 기준입니다. 구현 전에는 `AGENTS.md`의 사용자 동의 원칙을 따릅니다.

## 목표

- 레벨에 배치한 아이템을 플레이어가 획득합니다.
- 아이템 획득 결과가 Listen Server 호스트와 원격 클라이언트에 일관되게 보입니다.
- 플레이어는 최대 세 개의 아이템을 보유하고, 보유 아이템은 캐릭터에 시각적으로 표시됩니다.
- 차지 점프와 머리 밟기 점프를 서버 권한으로 구현합니다.
- 아이템 드롭 원형 UI는 C++ `UItemWidget`을 열기까지만 구현합니다.

## 현재 생성 대상 클래스

```text
PlayerCharacter
PlayerController
InventoryComponent (UActorComponent)
ItemActor
GameMode
```

- 기본 `GameState`를 사용합니다. 전역 게임 상태가 생길 때만 커스텀 `GameState`를 추가합니다.
- `ItemManagerSubsystem`은 지금 만들지 않습니다. 레벨에 아이템을 수동 배치하는 프로토타입에는 필수가 아닙니다.

## 네트워크 원칙

| 요소 | 클라이언트에서 즉시 처리 | 서버에서 최종 처리 | 복제 또는 공개 범위 |
|---|---|---|---|
| `PlayerCharacter` | 이동 입력, 상호작용 시도, 로컬 연출 | 이동 결과, 점프, 상호작용 실행 | 위치·이동·점프·캐릭터 상태 |
| `PlayerController` | 입력, 카메라, 소유자 UI | 서버 RPC 요청 전달 | 소유자 전용 |
| `InventoryComponent` | 획득·드롭 UI 반응 | 아이템 추가·제거·수량 변경 | 인벤토리 데이터 복제 |
| `ItemActor` | 근접 강조 표시 | 획득 가능 여부와 획득 완료 | 월드 존재·상태 복제 |
| `GameMode` | 없음 | 스폰·게임 규칙 | 직접 복제하지 않음 |
| `GameState` | 복제값 기반 UI 갱신 | `GameMode`가 변경 | 전역 공개 상태가 생길 때만 사용 |

- 게임 결과를 바꾸는 판정은 서버가 최종 권한을 가집니다.
- 클라이언트는 요청만 보내고, 서버가 유효성을 검증한 뒤 상태를 변경합니다.
- 서버 전용 객체인 `GameMode`와 향후 `ItemManagerSubsystem`은 직접 복제하지 않습니다. 결과 데이터는 `ItemActor`, `InventoryComponent`, 필요 시 `GameState`에 반영합니다.

## PlayerCharacter

### 역할

- 플레이어 Pawn 본체입니다.
- 이동, 점프, 충돌, 월드 아이템과의 거리 판정에 관여합니다.
- 탑뷰 카메라와 인벤토리, 아이템 부착 위치를 보유합니다.
- 다른 플레이어의 머리를 밟는 상황을 감지합니다.

### 보유 컴포넌트

```text
SpringArm
Camera
InventoryComponent
ItemAttachSlot0 (USceneComponent)
ItemAttachSlot1 (USceneComponent)
ItemAttachSlot2 (USceneComponent)
```

- SpringArm과 Camera는 탑뷰 약 45도 시점을 구성합니다.
- 각 `ItemAttachSlot`은 캐릭터에 부착하고, 위치·회전은 에디터에서 조정합니다.

### 아이템 슬롯 대응

```text
Inventory[0] -> ItemAttachSlot0
Inventory[1] -> ItemAttachSlot1
Inventory[2] -> ItemAttachSlot2
```

- 아이템을 획득하면 실제 월드 `ItemActor`는 제거합니다.
- 인벤토리 복제 데이터가 바뀌면 각 클라이언트가 대응하는 `ItemAttachSlot`에 시각용 메시를 만들거나 갱신합니다.
- 아이템 위치를 무작위로 결정하지 않고 슬롯별 SceneComponent 위치를 사용하여 모든 클라이언트가 같은 모습을 봅니다.
- 획득 시 ItemActor에 지정된 StaticMesh 참조를 인벤토리 슬롯에 함께 복사합니다. 이후 DataTable을 추가하면 이 메시 참조를 ItemId 기반 데이터 조회로 대체합니다.

## PlayerController

### 역할

- 플레이어 개인의 입력, 카메라, 소유자 전용 UI를 관리합니다.
- 입력을 받아 필요한 서버 RPC 요청을 보냅니다.

### 입력

```text
WASD  : 이동
Space : 점프 차지 시작 / 종료
E     : 가까운 아이템 획득 요청
Q     : 아이템 드롭 원형 UI 열기
```

- 이동 중 캐릭터는 마우스 포인터 방향을 바라보지 않습니다.
- `PlayerController`는 소유자 전용 객체입니다. 원격 플레이어 UI는 여기에서 처리하지 않습니다.
- 드롭 원형 UI는 PlayerController에서 지정한 `UItemWidget` 자식 위젯을 열기까지만 구현하고, 드래그를 통한 실제 드롭은 후속 기능입니다.
- `PlayerController`는 블루프린트에서 지정한 Input Mapping Context(IMC)와 Input Action(IA)을 사용합니다.

## 점프

### 차지 점프

- Space를 누르면 차지를 시작합니다.
- 최대 차지 시간은 3초입니다.
- Space를 떼면 서버가 차지 시간에 비례한 점프 힘을 적용합니다.
- 최대 점프 힘은 에디터에서 수정할 수 있는 값으로 둡니다. Unreal의 `JumpZVelocity` 계열 값을 기준으로 조정합니다.
- 차지 시작과 종료는 소유 Character의 Reliable Server RPC로 전달하며, 차지 시간은 서버 시간으로 계산합니다.

### 머리 밟기 점프

- 각 캐릭터는 머리 위치에 `HeadBounceCollider`(`UBoxComponent`)를 둡니다. 이 콜라이더는 이동을 막지 않는 Query Only / Pawn Overlap 충돌 영역입니다.
- 서버에서만 Overlap을 판정하며, 상대 캐릭터가 공중 상태이고 Z축 속도가 아래 방향일 때만 발동합니다.
- 다른 플레이어 머리를 밟는 순간 서버가 `HeadBounceJumpZVelocity`의 고정된 점프 힘을 즉시 적용합니다.
- 차지 상태나 차지량과 무관하며, 수평 이동 속도는 유지합니다.
- 같은 머리 콜라이더에서 반복 발동하지 않도록 서버 전용 `HeadBounceCooldown`을 적용합니다. 점프 힘과 쿨다운은 에디터에서 조정할 수 있습니다.

## InventoryComponent

### 역할

- `UActorComponent`를 상속합니다.
- `PlayerCharacter`에 부착합니다.
- 최대 세 칸의 아이템 데이터를 관리합니다.

### 규칙

- 아이템 추가와 제거는 서버만 수행합니다.
- 세 슬롯은 `EItemType::None`으로 비어 있음을 나타냅니다.
- 인벤토리 배열은 `ReplicatedUsing`으로 복제합니다.
- 서버 변경 직후와 클라이언트 `OnRep`에서 공통 인벤토리 변경 이벤트를 호출합니다.
- 이후 Character의 부착 메시와 UI가 인벤토리 변경 이벤트를 구독해 갱신합니다.
- 아이템을 실제 월드 액터 상태로 인벤토리에 보관하지 않고, 아이템 데이터를 저장합니다.

## ItemActor

### 역할

- 레벨에 배치되는 월드 아이템입니다.
- 아이템 종류와 획득 가능 상태, 월드 메시와 충돌을 보유합니다.
- 서버가 획득 가능 여부를 최종 판정합니다.
- `PickupRange`은 서버 거리 검사용 에디터 조절값이며, `PickupCollision`은 이후 근처 아이템 탐색과 강조 표시에 사용합니다.
- `TryPickup`은 RPC가 아닌 서버 전용 판정 함수입니다. 이후 PlayerController의 Server RPC가 호출합니다.

### 아이템 구분

- 프로토타입에서는 코드 기반 열거형 `EItemType`으로 다섯 가지 아이템 종류를 구분하고, 획득 당시 ItemActor의 StaticMesh 참조를 인벤토리에 함께 저장합니다.
- 이후 DataTable로 이름, 메시, 아이콘, 효과 등의 데이터를 연결합니다.
- 현재 열거형은 아이템 등급이 아니라 아이템 종류입니다.

### 획득 흐름

```text
클라이언트
  1. 가까운 ItemActor를 탐색하고 로컬 강조 표시
  2. E 입력으로 대상 ItemActor 획득 요청

서버
  3. ItemActor가 유효하고 아직 획득 가능한지 확인
  4. 플레이어와 아이템 간 거리가 허용 범위인지 확인
  5. 인벤토리가 세 칸 미만인지 확인
  6. InventoryComponent에 아이템 데이터 추가
  7. ItemActor 제거

모든 클라이언트
  8. ItemActor 제거와 인벤토리 복제 결과 수신
  9. PlayerCharacter의 부착 메시 갱신
```

- 충돌은 가까운 아이템 탐색 및 강조 표시에 사용할 수 있습니다.
- 실제 획득은 서버의 거리·상태·인벤토리 공간 검증을 모두 통과해야 합니다.
- 현재 E 입력은 플레이어 주변에서 가장 가까운 ItemActor를 후보로 고른 뒤 `PlayerController::ServerRequestPickup`으로 요청합니다.

## GameMode

- 서버 전용 게임 규칙을 담당합니다.
- `DefaultPawnClass`와 `PlayerControllerClass`를 지정합니다.
- 캐릭터 스폰 위치와 스폰 규칙을 결정합니다.

## 이후 필요해질 때 추가할 구조

### ItemManagerSubsystem (`UWorldSubsystem`)

다음 정책이 생길 때 추가합니다.

- 아이템 리스폰
- 랜덤 위치 아이템 스폰
- 버린 아이템의 월드 액터 생성
- 라운드 시작 시 월드 아이템 초기화
- 전체 아이템 일괄 생성 또는 제거

### 커스텀 GameState

다음처럼 모든 플레이어가 공유해야 하는 게임 전체 상태가 생기면 추가합니다.

- 게임 시작·종료 상태
- 라운드 번호
- 남은 시간
- 목표 점수
- 전역 이벤트 상태

아이템별 월드 상태와 개인 인벤토리 데이터는 `GameState`가 아니라 `ItemActor`와 `InventoryComponent`에서 관리합니다.

## 권장 구현 순서

1. `EItemType` 다섯 종류와 `InventoryComponent` 세 칸을 만듭니다.
2. 레벨 배치 `ItemActor` 한 종류를 서버 검증 후 획득하게 만듭니다.
3. 인벤토리 복제와 `ItemAttachSlot0~2` 시각 표시를 연결합니다.
4. Listen Server 호스트와 원격 클라이언트에서 획득 결과를 확인합니다.
5. Q 입력으로 드롭 원형 UI를 엽니다.
6. 차지 점프와 머리 밟기 점프를 추가합니다.
