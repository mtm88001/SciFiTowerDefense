// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TDGameModeBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDBaseHealthChangedSignature, int32, CurrentHealth, int32, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTDMatchOutcomeSignature);

/** Authoritative match state for the tower-defense prototype. */
UCLASS(Blueprintable)
class SCIFITOWERDEFENSE_API ATDGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATDGameModeBase();

	UFUNCTION(BlueprintCallable, Category = "Tower Defense|Match")
	void ApplyBaseDamage(int32 DamageAmount);

	UFUNCTION(BlueprintCallable, Category = "Tower Defense|Match")
	void DeclareVictory();

	/** Awards a valid combat-kill reward to the tower-defense PlayerState. */
	UFUNCTION(BlueprintCallable, Category = "Tower Defense|Economy")
	void AwardEnemyKillCredits(int32 RewardAmount);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Tower Defense|Match", meta = (ClampMin = "1"))
	int32 MaxBaseHealth = 20;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|Match")
	int32 CurrentBaseHealth = 20;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|Match")
	bool bGameOver = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|Match")
	bool bGameWon = false;

	UPROPERTY(BlueprintAssignable, Category = "Tower Defense|Match")
	FTDBaseHealthChangedSignature OnBaseHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Tower Defense|Match")
	FTDMatchOutcomeSignature OnGameOver;

	UPROPERTY(BlueprintAssignable, Category = "Tower Defense|Match")
	FTDMatchOutcomeSignature OnGameWon;

protected:
	virtual void BeginPlay() override;
};
