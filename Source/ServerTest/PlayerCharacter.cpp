// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacter.h"

#include "Components/BoxComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InventoryComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogServerTestPlayerCharacter, Log, All);

// Sets default values
APlayerCharacter::APlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->SetRelativeRotation(FRotator(-50.0f, 0.0f, 0.0f));
	SpringArm->TargetArmLength = 2200.0f;
	SpringArm->bDoCollisionTest = false;
	SpringArm->bInheritYaw = false;
	SpringArm->bEnableCameraLag = false;
	SpringArm->bUsePawnControlRotation = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);
	Camera->SetFieldOfView(75.0f);
	Camera->bUsePawnControlRotation = false;

	HeadBounceCollider = CreateDefaultSubobject<UBoxComponent>(TEXT("HeadBounceCollider"));
	HeadBounceCollider->SetupAttachment(GetCapsuleComponent());
	HeadBounceCollider->SetRelativeLocation(FVector(0.0f, 0.0f, 75.0f));
	HeadBounceCollider->SetBoxExtent(FVector(35.0f, 35.0f, 25.0f));
	HeadBounceCollider->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HeadBounceCollider->SetCollisionResponseToAllChannels(ECR_Ignore);
	HeadBounceCollider->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	HeadBounceCollider->SetGenerateOverlapEvents(true);

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));

	ItemAttachSlot0 = CreateDefaultSubobject<USceneComponent>(TEXT("ItemAttachSlot0"));
	ItemAttachSlot0->SetupAttachment(RootComponent);
	ItemAttachSlot0->SetRelativeLocation(FVector(0.0f, -55.0f, 20.0f));

	ItemAttachSlot1 = CreateDefaultSubobject<USceneComponent>(TEXT("ItemAttachSlot1"));
	ItemAttachSlot1->SetupAttachment(RootComponent);
	ItemAttachSlot1->SetRelativeLocation(FVector(0.0f, 55.0f, 20.0f));

	ItemAttachSlot2 = CreateDefaultSubobject<USceneComponent>(TEXT("ItemAttachSlot2"));
	ItemAttachSlot2->SetupAttachment(RootComponent);
	ItemAttachSlot2->SetRelativeLocation(FVector(-45.0f, 0.0f, 20.0f));

	ItemVisual0 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemVisual0"));
	ItemVisual0->SetupAttachment(ItemAttachSlot0);
	ItemVisual0->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ItemVisual0->SetVisibility(false);

	ItemVisual1 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemVisual1"));
	ItemVisual1->SetupAttachment(ItemAttachSlot1);
	ItemVisual1->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ItemVisual1->SetVisibility(false);

	ItemVisual2 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemVisual2"));
	ItemVisual2->SetupAttachment(ItemAttachSlot2);
	ItemVisual2->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ItemVisual2->SetVisibility(false);

	// 마우스가 아닌 WASD 이동 방향을 바라보도록 설정합니다.
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
}

// Called when the game starts or when spawned
void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	HeadBounceCollider->OnComponentBeginOverlap.AddDynamic(this, &APlayerCharacter::OnHeadBounceOverlap);

	if (IsValid(InventoryComponent))
	{
		InventoryComponent->OnInventoryChanged.AddDynamic(this, &ThisClass::HandleInventoryChanged);
		RefreshAttachedItemVisuals();
	}
}

// Called every frame
void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void APlayerCharacter::Move(const FVector2D& MovementInput)
{
	if (!FMath::IsNearlyZero(MovementInput.X))
	{
		AddMovementInput(FVector::ForwardVector, MovementInput.X);
	}

	if (!FMath::IsNearlyZero(MovementInput.Y))
	{
		AddMovementInput(FVector::RightVector, MovementInput.Y);
	}
}

void APlayerCharacter::BeginJumpCharge()
{
	ServerBeginJumpCharge();
}

void APlayerCharacter::EndJumpCharge()
{
	ServerEndJumpCharge();
}

void APlayerCharacter::ServerBeginJumpCharge_Implementation()
{
	if (!GetCharacterMovement()->IsMovingOnGround())
	{
		return;
	}

	bIsChargingJump = true;
	JumpChargeStartTime = GetWorld()->GetTimeSeconds();
}

void APlayerCharacter::ServerEndJumpCharge_Implementation()
{
	if (!bIsChargingJump)
	{
		return;
	}

	bIsChargingJump = false;
	const float ChargeTime = FMath::Clamp(GetWorld()->GetTimeSeconds() - JumpChargeStartTime, 0.0f, MaxJumpChargeTime);
	ApplyChargedJump(ChargeTime);
}

void APlayerCharacter::ApplyChargedJump(float ChargeTime)
{
	const float ChargeAlpha = FMath::Clamp(ChargeTime / MaxJumpChargeTime, 0.0f, 1.0f);
	const float JumpVelocity = FMath::Lerp(MinJumpZVelocity, MaxJumpZVelocity, ChargeAlpha);

	// 서버가 최종 점프 속도를 적용하면 CharacterMovement가 결과를 복제합니다.
	LaunchCharacter(FVector(0.0f, 0.0f, JumpVelocity), false, true);
}

void APlayerCharacter::HandleInventoryChanged()
{
	RefreshAttachedItemVisuals();
}

UInventoryComponent* APlayerCharacter::GetInventoryComponent() const
{
	return InventoryComponent;
}

USceneComponent* APlayerCharacter::GetItemAttachSlot(int32 SlotIndex) const
{
	switch (SlotIndex)
	{
	case 0:
		return ItemAttachSlot0;
	case 1:
		return ItemAttachSlot1;
	case 2:
		return ItemAttachSlot2;
	default:
		return nullptr;
	}
}

void APlayerCharacter::RefreshAttachedItemVisuals()
{
	if (!IsValid(InventoryComponent))
	{
		return;
	}

	for (int32 SlotIndex = 0; SlotIndex < UInventoryComponent::MaxInventorySlots; ++SlotIndex)
	{
		UpdateItemVisual(GetItemVisualComponent(SlotIndex), InventoryComponent->GetItemAt(SlotIndex));
	}
}

UStaticMeshComponent* APlayerCharacter::GetItemVisualComponent(int32 SlotIndex) const
{
	switch (SlotIndex)
	{
	case 0:
		return ItemVisual0;
	case 1:
		return ItemVisual1;
	case 2:
		return ItemVisual2;
	default:
		return nullptr;
	}
}

void APlayerCharacter::UpdateItemVisual(UStaticMeshComponent* ItemVisual, const FInventoryItem& Item)
{
	if (!IsValid(ItemVisual))
	{
		return;
	}

	UStaticMesh* MeshToDisplay = Item.VisualMesh.Get();
	const bool bShouldShowItem = !Item.IsEmpty() && IsValid(MeshToDisplay);

	ItemVisual->SetStaticMesh(MeshToDisplay);
	ItemVisual->SetVisibility(bShouldShowItem, true);
}

void APlayerCharacter::OnHeadBounceOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	// 충돌 결과를 바꾸는 머리 밟기 판정은 서버에서만 수행합니다.
	if (!HasAuthority())
	{
		return;
	}

	APlayerCharacter* Jumper = Cast<APlayerCharacter>(OtherActor);
	if (!CanTriggerHeadBounce(Jumper))
	{
		return;
	}

	ApplyHeadBounce(Jumper);
}

bool APlayerCharacter::CanTriggerHeadBounce(const APlayerCharacter* Jumper) const
{
	if (!IsValid(Jumper) || Jumper == this)
	{
		return false;
	}

	const UCharacterMovementComponent* JumperMovement = Jumper->GetCharacterMovement();
	if (!IsValid(JumperMovement) || !JumperMovement->IsFalling())
	{
		return false;
	}

	// 위로 상승 중이거나 지면에 서 있는 상태에서는 밟기 점프가 발동하지 않습니다.
	if (Jumper->GetVelocity().Z >= 0.0f)
	{
		return false;
	}

	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	return World->GetTimeSeconds() - LastHeadBounceTime >= HeadBounceCooldown;
}

void APlayerCharacter::ApplyHeadBounce(APlayerCharacter* Jumper)
{
	LastHeadBounceTime = GetWorld()->GetTimeSeconds();

	// 수직 속도만 고정값으로 덮어쓰고, 수평 이동 속도는 유지합니다.
	Jumper->LaunchCharacter(FVector(0.0f, 0.0f, HeadBounceJumpZVelocity), false, true);
}

