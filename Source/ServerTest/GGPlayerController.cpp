// Fill out your copyright notice in the Description page of Project Settings.


#include "GGPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "ItemWidget.h"
#include "ItemActor.h"
#include "PlayerCharacter.h"

DEFINE_LOG_CATEGORY_STATIC(LogServerTestPlayer, Log, All);

void AGGPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	if (!IsValid(DefaultMappingContext))
	{
		UE_LOG(LogServerTestPlayer, Warning, TEXT("DefaultMappingContext가 지정되지 않았습니다."));
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		InputSubsystem->AddMappingContext(DefaultMappingContext, 0);
	}
}

void AGGPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (!IsValid(EnhancedInputComponent))
	{
		UE_LOG(LogServerTestPlayer, Error, TEXT("Enhanced Input Component를 찾지 못했습니다."));
		return;
	}

	if (IsValid(MoveAction))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Move);
	}

	if (IsValid(JumpChargeAction))
	{
		EnhancedInputComponent->BindAction(JumpChargeAction, ETriggerEvent::Started, this, &ThisClass::BeginJumpCharge);
		EnhancedInputComponent->BindAction(JumpChargeAction, ETriggerEvent::Completed, this, &ThisClass::EndJumpCharge);
		EnhancedInputComponent->BindAction(JumpChargeAction, ETriggerEvent::Canceled, this, &ThisClass::EndJumpCharge);
	}

	if (IsValid(InteractAction))
	{
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &ThisClass::HandleInteract);
	}

	if (IsValid(ToggleDropInventoryAction))
	{
		EnhancedInputComponent->BindAction(ToggleDropInventoryAction, ETriggerEvent::Started, this, &ThisClass::ToggleDropInventoryUI);
	}
}

APlayerCharacter* AGGPlayerController::GetControlledPlayerCharacter() const
{
	return Cast<APlayerCharacter>(GetPawn());
}

AItemActor* AGGPlayerController::FindClosestInteractableItem() const
{
	const APlayerCharacter* PlayerCharacter = GetControlledPlayerCharacter();
	UWorld* World = GetWorld();
	if (!IsValid(PlayerCharacter) || !IsValid(World))
	{
		return nullptr;
	}

	AItemActor* ClosestItem = nullptr;
	float ClosestDistanceSquared = TNumericLimits<float>::Max();

	for (TActorIterator<AItemActor> It(World); It; ++It)
	{
		AItemActor* ItemActor = *It;
		if (!IsValid(ItemActor) || !ItemActor->bIsAvailable)
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(PlayerCharacter->GetActorLocation(), ItemActor->GetActorLocation());
		if (DistanceSquared > FMath::Square(ItemActor->GetPickupRange()) || DistanceSquared >= ClosestDistanceSquared)
		{
			continue;
		}

		ClosestItem = ItemActor;
		ClosestDistanceSquared = DistanceSquared;
	}

	return ClosestItem;
}

void AGGPlayerController::Move(const FInputActionValue& Value)
{
	const FVector2D MovementInput = Value.Get<FVector2D>();

	if (APlayerCharacter* PlayerCharacter = GetControlledPlayerCharacter())
	{
		PlayerCharacter->Move(MovementInput);
	}
}

void AGGPlayerController::BeginJumpCharge()
{
	if (APlayerCharacter* PlayerCharacter = GetControlledPlayerCharacter())
	{
		PlayerCharacter->BeginJumpCharge();
	}
}

void AGGPlayerController::EndJumpCharge()
{
	if (APlayerCharacter* PlayerCharacter = GetControlledPlayerCharacter())
	{
		PlayerCharacter->EndJumpCharge();
	}
}

void AGGPlayerController::HandleInteract()
{
	AItemActor* TargetItem = FindClosestInteractableItem();
	if (!IsValid(TargetItem))
	{
		UE_LOG(LogServerTestPlayer, Verbose, TEXT("획득 범위 안에 아이템이 없습니다."));
		return;
	}

	ServerRequestPickup(TargetItem);
}

void AGGPlayerController::ServerRequestPickup_Implementation(AItemActor* TargetItem)
{
	APlayerCharacter* PlayerCharacter = GetControlledPlayerCharacter();
	if (!IsValid(PlayerCharacter) || !IsValid(TargetItem))
	{
		return;
	}

	if (!TargetItem->TryPickup(PlayerCharacter))
	{
		UE_LOG(LogServerTestPlayer, Verbose, TEXT("서버 아이템 획득 요청이 거부되었습니다."));
	}
}

void AGGPlayerController::ToggleDropInventoryUI()
{
	if (!IsLocalController())
	{
		return;
	}

	if (IsValid(DropInventoryWidget) && DropInventoryWidget->IsInViewport())
	{
		DropInventoryWidget->RemoveFromParent();
		bShowMouseCursor = false;

		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
		return;
	}

	if (!IsValid(DropInventoryWidget))
	{
		if (!IsValid(ItemWidgetClass))
		{
			UE_LOG(LogServerTestPlayer, Warning, TEXT("ItemWidgetClass가 지정되지 않아 드롭 UI를 열 수 없습니다."));
			return;
		}

		DropInventoryWidget = CreateWidget<UItemWidget>(this, ItemWidgetClass);
	}

	if (!IsValid(DropInventoryWidget))
	{
		UE_LOG(LogServerTestPlayer, Error, TEXT("ItemWidget을 생성하지 못했습니다."));
		return;
	}

	DropInventoryWidget->AddToViewport();
	bShowMouseCursor = true;

	FInputModeGameAndUI InputMode;
	SetInputMode(InputMode);
}
