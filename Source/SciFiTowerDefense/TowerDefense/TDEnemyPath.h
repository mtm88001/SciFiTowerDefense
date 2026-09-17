// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TDEnemyPath.generated.h"

class UArrowComponent;
class USplineComponent;

/** Editable predefined route for tower-defense enemies. */
UCLASS(Blueprintable)
class SCIFITOWERDEFENSE_API ATDEnemyPath : public AActor
{
	GENERATED_BODY()

public:
	ATDEnemyPath();
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintPure, Category = "Tower Defense|Path")
	USplineComponent* GetPathSpline() const { return PathSpline; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USplineComponent> PathSpline;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UArrowComponent> StartArrow;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UArrowComponent> EndArrow;

private:
	void UpdateEndpointArrows();
};
