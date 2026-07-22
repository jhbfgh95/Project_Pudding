#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemTypes.h"
#include "ItemActor.generated.h"

class APlayerCharacter;
class UStaticMeshComponent;
class USphereComponent;

UCLASS()
class SERVERTEST_API AItemActor : public AActor
{
	GENERATED_BODY()

public:
	AItemActor();

	/** 현재 ItemActor가 나타내는 프로토타입 아이템 종류입니다. */
	UPROPERTY(EditInstanceOnly, Replicated, BlueprintReadOnly, Category = "Item")
	EItemType ItemType = EItemType::Item1;

	/** 서버에서만 바꾸는 획득 가능 상태입니다. */
	UPROPERTY(ReplicatedUsing = OnRep_IsAvailable, VisibleAnywhere, BlueprintReadOnly, Category = "Item")
	bool bIsAvailable = true;

	/** 서버가 플레이어와의 거리를 검사할 때 사용하는 최대 거리입니다. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "0.0"))
	float PickupRange = 200.0f;

	/** 아이템 시각 요소입니다. 메시와 머티리얼은 에디터에서 지정합니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ItemMesh;

	/** 근처 아이템 탐색과 강조 표시에 사용할 충돌 영역입니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> PickupCollision;

	/** 서버에서 호출합니다. 거리, 상태, 인벤토리 공간을 모두 통과하면 아이템을 획득합니다. */
	bool TryPickup(APlayerCharacter* Character);

	bool CanBePickedUpBy(const APlayerCharacter* Character) const;
	EItemType GetItemType() const;
	float GetPickupRange() const;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION()
	void OnRep_IsAvailable();

	void UpdateItemAvailabilityVisual();
};
