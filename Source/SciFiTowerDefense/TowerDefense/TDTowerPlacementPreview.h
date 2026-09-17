// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TDTowerPlacementPreview.generated.h"

class UMaterialInterface;
class USceneComponent;
class UStaticMeshComponent;
class ATDTowerBase;

/** Lightweight, non-colliding visual used while choosing a tower location. */
UCLASS(Blueprintable)
class SCIFITOWERDEFENSE_API ATDTowerPlacementPreview : public AActor
{
	GENERATED_BODY()

public:
	ATDTowerPlacementPreview();

	UFUNCTION(BlueprintCallable, Category = "Tower Defense|Placement")
	void SetPlacementValid(bool bIsValid);

	/** Configures the placeholder silhouette from the selected tower class defaults. */
	UFUNCTION(BlueprintCallable, Category = "Tower Defense|Placement")
	void ConfigureForTower(TSubclassOf<ATDTowerBase> TowerClass);

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|Placement")
	bool bPlacementValid = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tower Defense|Placement")
	TObjectPtr<UMaterialInterface> ValidMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tower Defense|Placement")
	TObjectPtr<UMaterialInterface> InvalidMaterial;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> FootprintMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> TowerSilhouette;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> RapidBarrelSilhouetteA;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> RapidBarrelSilhouetteB;

private:
	void ApplyFeedbackMaterial();
};
