// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GGPlayerController.generated.h"

class APlayerCharacter;
class AItemActor;
class UInputAction;
class UInputMappingContext;
class UItemWidget;

UCLASS()
class SERVERTEST_API AGGPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	/** 블루프린트에서 지정할 플레이어 입력 매핑 컨텍스트입니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpChargeAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> ToggleDropInventoryAction;

	/** BP_GGPlayerController에서 지정할 드롭 인벤토리 위젯 클래스입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UItemWidget> ItemWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UItemWidget> DropInventoryWidget;

private:
	APlayerCharacter* GetControlledPlayerCharacter() const;
	AItemActor* FindClosestInteractableItem() const;

	void Move(const struct FInputActionValue& Value);
	void BeginJumpCharge();
	void EndJumpCharge();
	void HandleInteract();
	void ToggleDropInventoryUI();

	UFUNCTION(Server, Reliable)
	void ServerRequestPickup(AItemActor* TargetItem);
};
