#include "InventoryComponent.h"

#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogServerTestInventory, Log, All);

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	Items.Init(FInventoryItem(), MaxInventorySlots);
}

bool UInventoryComponent::TryAddItem(const FInventoryItem& Item)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		UE_LOG(LogServerTestInventory, Warning, TEXT("TryAddItem은 서버에서만 호출할 수 있습니다."));
		return false;
	}

	if (Item.IsEmpty() || IsFull())
	{
		return false;
	}

	for (int32 SlotIndex = 0; SlotIndex < Items.Num(); ++SlotIndex)
	{
		if (Items[SlotIndex].IsEmpty())
		{
			Items[SlotIndex] = Item;
			NotifyInventoryChanged();
			return true;
		}
	}

	return false;
}

bool UInventoryComponent::TryRemoveItemAt(int32 SlotIndex, FInventoryItem& OutItem)
{
	OutItem = FInventoryItem();

	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		UE_LOG(LogServerTestInventory, Warning, TEXT("TryRemoveItemAt은 서버에서만 호출할 수 있습니다."));
		return false;
	}

	if (!IsValidSlot(SlotIndex) || Items[SlotIndex].IsEmpty())
	{
		return false;
	}

	OutItem = Items[SlotIndex];
	Items[SlotIndex] = FInventoryItem();
	NotifyInventoryChanged();
	return true;
}

bool UInventoryComponent::IsFull() const
{
	for (const FInventoryItem& Item : Items)
	{
		if (Item.IsEmpty())
		{
			return false;
		}
	}

	return true;
}

int32 UInventoryComponent::GetItemCount() const
{
	int32 ItemCount = 0;
	for (const FInventoryItem& Item : Items)
	{
		if (!Item.IsEmpty())
		{
			++ItemCount;
		}
	}

	return ItemCount;
}

FInventoryItem UInventoryComponent::GetItemAt(int32 SlotIndex) const
{
	return IsValidSlot(SlotIndex) ? Items[SlotIndex] : FInventoryItem();
}

bool UInventoryComponent::IsValidSlot(int32 SlotIndex) const
{
	return Items.IsValidIndex(SlotIndex);
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UInventoryComponent, Items);
}

void UInventoryComponent::OnRep_Items()
{
	NotifyInventoryChanged();
}

void UInventoryComponent::NotifyInventoryChanged()
{
	OnInventoryChanged.Broadcast();
}
