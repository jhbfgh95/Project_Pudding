// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PlayerCharacter.generated.h"

class UBoxComponent;
class UCameraComponent;
class UInventoryComponent;
class UPrimitiveComponent;
class USceneComponent;
class USpringArmComponent;
class UStaticMeshComponent;
struct FInventoryItem;

UCLASS()
class SERVERTEST_API APlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	APlayerCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/** 탑뷰 시점을 위한 카메라 붐입니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> Camera;

	/** 캐릭터 머리 위치에서 밟기 판정을 받는 충돌 영역입니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> HeadBounceCollider;

	/** 서버 권한으로 아이템 데이터를 보관하고 클라이언트에 복제하는 인벤토리입니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInventoryComponent> InventoryComponent;

	/** 인벤토리 칸별 아이템 메시를 붙일 위치입니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> ItemAttachSlot0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> ItemAttachSlot1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> ItemAttachSlot2;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ItemVisual0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ItemVisual1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ItemVisual2;

	/** 머리를 밟았을 때 적용할 고정 Z축 점프 힘입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Head Bounce")
	float HeadBounceJumpZVelocity = 1000.0f;

	/** 같은 머리 충돌 영역에서 연속으로 튕기는 것을 막는 시간입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Head Bounce", meta = (ClampMin = "0.0"))
	float HeadBounceCooldown = 0.15f;

	/** 차지하지 않고 점프 키를 바로 뗐을 때 적용할 최소 Z축 점프 힘입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jump", meta = (ClampMin = "0.0"))
	float MinJumpZVelocity = 650.0f;

	/** 최대 차지 시 적용할 최대 Z축 점프 힘입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jump", meta = (ClampMin = "0.0"))
	float MaxJumpZVelocity = 1500.0f;

	/** 점프 힘이 최대치에 도달하는 차지 시간입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jump", meta = (ClampMin = "0.1"))
	float MaxJumpChargeTime = 3.0f;

	/** 서버에서만 사용하는 마지막 머리 밟기 처리 시간입니다. */
	float LastHeadBounceTime = -BIG_NUMBER;

	/** 서버에서만 사용하는 점프 차지 상태입니다. */
	bool bIsChargingJump = false;
	float JumpChargeStartTime = 0.0f;

	UFUNCTION()
	void OnHeadBounceOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	bool CanTriggerHeadBounce(const APlayerCharacter* Jumper) const;
	void ApplyHeadBounce(APlayerCharacter* Jumper);

	UFUNCTION(Server, Reliable)
	void ServerBeginJumpCharge();

	UFUNCTION(Server, Reliable)
	void ServerEndJumpCharge();

	void ApplyChargedJump(float ChargeTime);

	UFUNCTION()
	void HandleInventoryChanged();

	UStaticMeshComponent* GetItemVisualComponent(int32 SlotIndex) const;
	void UpdateItemVisual(UStaticMeshComponent* ItemVisual, const FInventoryItem& Item);

public:	
	/** 고정 탑뷰 기준으로 X는 앞/뒤, Y는 좌/우 이동을 뜻합니다. */
	void Move(const FVector2D& MovementInput);
	void BeginJumpCharge();
	void EndJumpCharge();

	UInventoryComponent* GetInventoryComponent() const;
	USceneComponent* GetItemAttachSlot(int32 SlotIndex) const;
	void RefreshAttachedItemVisuals();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
