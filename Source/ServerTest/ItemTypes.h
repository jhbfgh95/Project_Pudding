#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMesh.h"
#include "ItemTypes.generated.h"

/** 프로토타입 단계에서 코드로 구분하는 아이템 종류입니다. */
UENUM(BlueprintType)
enum class EItemType : uint8
{
	None UMETA(DisplayName = "None"),
	Item1 UMETA(DisplayName = "Item 1"),
	Item2 UMETA(DisplayName = "Item 2"),
	Item3 UMETA(DisplayName = "Item 3"),
	Item4 UMETA(DisplayName = "Item 4"),
	Item5 UMETA(DisplayName = "Item 5")
};

/** 인벤토리 한 칸에 저장되는 아이템 데이터입니다. */
USTRUCT(BlueprintType)
struct FInventoryItem
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
	EItemType ItemType = EItemType::None;

	/** 획득 당시 ItemActor에 지정된 메시입니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UStaticMesh> VisualMesh = nullptr;

	bool IsEmpty() const
	{
		return ItemType == EItemType::None;
	}
};
