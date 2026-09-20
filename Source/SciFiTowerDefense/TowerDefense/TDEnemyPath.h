// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TDEnemyPath.generated.h"

class UArrowComponent;
class USplineComponent;
class USplineMeshComponent;
class UStaticMesh;
class UMaterialInterface;

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

	/** Rebuilds the spline-mesh ribbon from the current spline shape. Runs automatically on
	 *  construction, and can also be triggered manually after editing spline points. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Tower Defense|Path|Visual")
	void RebuildPathVisual();

	/** Mesh stretched between spline points to build the path ribbon. Should be a flat plane
	 *  with its long axis along local X and its UVs spanning 0-1 along X and Y. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Path|Visual")
	TObjectPtr<UStaticMesh> PathSegmentMesh;

	/** Material applied to each path segment. A per-segment dynamic instance overrides its
	 *  UVLengthScale/UVLengthOffset scalar parameters so the texture tiles continuously. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Path|Visual")
	TObjectPtr<UMaterialInterface> PathVisualMaterial;

	/** World-space width of the rendered path ribbon, across the direction of travel. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Path|Visual", meta = (ClampMin = "1.0", Units = "cm"))
	float PathWidth = 300.0f;

	/** World-space length of each spline-mesh chunk. Kept short relative to the curve's radius
	 *  so tight turns still read as smoothly curved rather than faceted straight segments. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Path|Visual", meta = (ClampMin = "10.0", Units = "cm"))
	float PathVisualStepLength = 100.0f;

	/** World-space distance one full repeat of the path texture should span along the direction
	 *  of travel. Decoupled from PathVisualStepLength so texture scale doesn't dictate curve
	 *  smoothness (or vice versa); tune to match the texture's real aspect ratio at PathWidth. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Path|Visual", meta = (ClampMin = "10.0", Units = "cm"))
	float PathTileWorldLength = 800.0f;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USplineComponent> PathSpline;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UArrowComponent> StartArrow;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UArrowComponent> EndArrow;

private:
	void UpdateEndpointArrows();
	void ClearPathVisual();

	UPROPERTY(Transient)
	TArray<TObjectPtr<USplineMeshComponent>> PathVisualSegments;
};
