// Copyright Epic Games, Inc. All Rights Reserved.

#include "TowerDefense/TDGameModeBase.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TowerDefense/TDPlayerState.h"

ATDGameModeBase::ATDGameModeBase()
{
	MaxBaseHealth = 20;
	CurrentBaseHealth = MaxBaseHealth;
	PlayerStateClass = ATDPlayerState::StaticClass();
}

void ATDGameModeBase::BeginPlay()
{
	Super::BeginPlay();
	MaxBaseHealth = FMath::Max(1, MaxBaseHealth);
	CurrentBaseHealth = MaxBaseHealth;
	bGameOver = false;
	bGameWon = false;
}

void ATDGameModeBase::ApplyBaseDamage(const int32 DamageAmount)
{
	if (DamageAmount <= 0 || bGameOver || bGameWon)
	{
		return;
	}

	const int32 PreviousHealth = CurrentBaseHealth;
	CurrentBaseHealth = FMath::Clamp(CurrentBaseHealth - DamageAmount, 0, MaxBaseHealth);
	if (CurrentBaseHealth == PreviousHealth)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("TD Base damaged by %d. Health: %d/%d"),
		PreviousHealth - CurrentBaseHealth, CurrentBaseHealth, MaxBaseHealth);
	OnBaseHealthChanged.Broadcast(CurrentBaseHealth, MaxBaseHealth);

	if (CurrentBaseHealth == 0)
	{
		bGameOver = true;
		bGameWon = false;
		UE_LOG(LogTemp, Log, TEXT("TD GAME OVER"));
		OnGameOver.Broadcast();
	}
}

void ATDGameModeBase::DeclareVictory()
{
	if (bGameOver || bGameWon || CurrentBaseHealth <= 0)
	{
		return;
	}

	bGameWon = true;
	UE_LOG(LogTemp, Log, TEXT("TD VICTORY - All waves completed"));
	OnGameWon.Broadcast();
}

void ATDGameModeBase::AwardEnemyKillCredits(const int32 RewardAmount)
{
	if (RewardAmount <= 0 || bGameOver || bGameWon || !GetWorld())
	{
		return;
	}

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	ATDPlayerState* Economy = PlayerController ? PlayerController->GetPlayerState<ATDPlayerState>() : nullptr;
	if (!IsValid(Economy))
	{
		UE_LOG(LogTemp, Error, TEXT("TD kill reward could not find ATDPlayerState"));
		return;
	}

	Economy->AddCredits(RewardAmount);
	UE_LOG(LogTemp, Log, TEXT("TD Enemy kill reward: %d. Credits: %d"), RewardAmount, Economy->GetCredits());
}
