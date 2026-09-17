// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "TDPlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDCreditsChangedSignature, int32, NewBalance);

/** Authoritative spendable-credit state for a tower-defense player. */
UCLASS(Blueprintable)
class SCIFITOWERDEFENSE_API ATDPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ATDPlayerState();

	UFUNCTION(BlueprintPure, Category = "Tower Defense|Economy")
	int32 GetCredits() const { return CurrentCredits; }

	UFUNCTION(BlueprintPure, Category = "Tower Defense|Economy")
	bool CanAfford(int32 Cost) const;

	/** Returns false without changing the balance when Amount is invalid or unaffordable. */
	UFUNCTION(BlueprintCallable, Category = "Tower Defense|Economy")
	bool SpendCredits(int32 Amount);

	/** Ignores zero and negative reward amounts. */
	UFUNCTION(BlueprintCallable, Category = "Tower Defense|Economy")
	void AddCredits(int32 Amount);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Tower Defense|Economy", meta = (ClampMin = "0"))
	int32 StartingCredits = 500;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|Economy")
	int32 CurrentCredits = 500;

	UPROPERTY(BlueprintAssignable, Category = "Tower Defense|Economy")
	FTDCreditsChangedSignature OnCreditsChanged;

protected:
	virtual void BeginPlay() override;
};
