#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemTypes.h"
#include "InventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SERVERTEST_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	static constexpr int32 MaxInventorySlots = 3;

	/** 아이템 데이터와 획득 당시의 메시를 함께 모든 클라이언트에 복제합니다. */
	UPROPERTY(ReplicatedUsing = OnRep_Items, VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TArray<FInventoryItem> Items;

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryChanged OnInventoryChanged;

	/** 빈 칸이 있으면 아이템 데이터를 넣습니다. 서버에서만 성공합니다. */
	bool TryAddItem(const FInventoryItem& Item);

	/** 지정한 슬롯의 아이템을 비웁니다. 실제 드롭 기능에서 사용합니다. */
	bool TryRemoveItemAt(int32 SlotIndex, FInventoryItem& OutItem);

	bool IsFull() const;
	int32 GetItemCount() const;
	FInventoryItem GetItemAt(int32 SlotIndex) const;
	bool IsValidSlot(int32 SlotIndex) const;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UFUNCTION()
	void OnRep_Items();

	void NotifyInventoryChanged();
};
