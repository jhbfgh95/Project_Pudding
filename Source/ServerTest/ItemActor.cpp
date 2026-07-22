#include "ItemActor.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "PlayerCharacter.h"

DEFINE_LOG_CATEGORY_STATIC(LogServerTestItem, Log, All);

AItemActor::AItemActor()
{
	bReplicates = true;
	SetReplicateMovement(false);

	PickupCollision = CreateDefaultSubobject<USphereComponent>(TEXT("PickupCollision"));
	PickupCollision->InitSphereRadius(PickupRange);
	PickupCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	PickupCollision->SetGenerateOverlapEvents(true);
	RootComponent = PickupCollision;

	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	ItemMesh->SetupAttachment(PickupCollision);
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AItemActor::BeginPlay()
{
	Super::BeginPlay();
	UpdateItemAvailabilityVisual();
}

void AItemActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	PickupCollision->SetSphereRadius(PickupRange);
}

bool AItemActor::TryPickup(APlayerCharacter* Character)
{
	if (!HasAuthority() || !CanBePickedUpBy(Character))
	{
		return false;
	}

	UInventoryComponent* InventoryComponent = Character->GetInventoryComponent();
	FInventoryItem InventoryItem;
	InventoryItem.ItemType = ItemType;
	InventoryItem.VisualMesh = ItemMesh->GetStaticMesh();

	if (!IsValid(InventoryComponent) || !InventoryComponent->TryAddItem(InventoryItem))
	{
		return false;
	}

	bIsAvailable = false;
	UpdateItemAvailabilityVisual();
	ForceNetUpdate();

	UE_LOG(LogServerTestItem, Log, TEXT("아이템 획득 성공: %s"), *GetName());
	Destroy();
	return true;
}

bool AItemActor::CanBePickedUpBy(const APlayerCharacter* Character) const
{
	if (!IsValid(Character) || !bIsAvailable || ItemType == EItemType::None)
	{
		return false;
	}

	const UInventoryComponent* InventoryComponent = Character->GetInventoryComponent();
	if (!IsValid(InventoryComponent) || InventoryComponent->IsFull())
	{
		return false;
	}

	return FVector::DistSquared(Character->GetActorLocation(), GetActorLocation()) <= FMath::Square(PickupRange);
}

EItemType AItemActor::GetItemType() const
{
	return ItemType;
}

float AItemActor::GetPickupRange() const
{
	return PickupRange;
}

void AItemActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AItemActor, ItemType);
	DOREPLIFETIME(AItemActor, bIsAvailable);
}

void AItemActor::OnRep_IsAvailable()
{
	UpdateItemAvailabilityVisual();
}

void AItemActor::UpdateItemAvailabilityVisual()
{
	ItemMesh->SetVisibility(bIsAvailable, true);
	PickupCollision->SetCollisionEnabled(bIsAvailable ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}
