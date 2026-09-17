// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "TDCameraPawn.generated.h"

class UCameraComponent;
class UFloatingPawnMovement;
class USceneComponent;

/** Camera-only pawn used by the tower-defense prototype. */
UCLASS(Blueprintable)
class SCIFITOWERDEFENSE_API ATDCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	ATDCameraPawn();

	/** Applies camera-relative keyboard movement on the world XY plane. */
	UFUNCTION(BlueprintCallable, Category = "Tower Defense|Camera")
	void MoveCamera(const FVector2D& Input);

	/** Pans from a screen-space mouse delta using the current orthographic scale. */
	UFUNCTION(BlueprintCallable, Category = "Tower Defense|Camera")
	void PanByScreenDelta(const FVector2D& PixelDelta, float ViewportWidth);

	/** Applies multiplicative orthographic zoom. Positive values zoom inward. */
	UFUNCTION(BlueprintCallable, Category = "Tower Defense|Camera")
	void ApplyZoomInput(float Value);

	UFUNCTION(BlueprintPure, Category = "Tower Defense|Camera")
	UCameraComponent* GetCamera() const { return Camera; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UFloatingPawnMovement> FloatingPawnMovement;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tower Defense|Camera", meta = (ClampMin = "1.0"))
	float InitialOrthoWidth = 6000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tower Defense|Camera", meta = (ClampMin = "1.0"))
	float MinimumOrthoWidth = 4000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tower Defense|Camera", meta = (ClampMin = "1.0"))
	float MaximumOrthoWidth = 8000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tower Defense|Camera", meta = (ClampMin = "0.01"))
	float ZoomFactorPerStep = 0.9f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tower Defense|Camera", meta = (ClampMin = "0.01"))
	float DragPanMultiplier = 2.0f;
};
