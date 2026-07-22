// Fill out your copyright notice in the Description page of Project Settings.


#include "GGGameMode.h"

#include "GGPlayerController.h"
#include "PlayerCharacter.h"

AGGGameMode::AGGGameMode()
{
	// GameMode는 서버에서만 존재하며, 접속 플레이어마다 이 Pawn과 Controller를 생성합니다.
	DefaultPawnClass = APlayerCharacter::StaticClass();
	PlayerControllerClass = AGGPlayerController::StaticClass();
}
