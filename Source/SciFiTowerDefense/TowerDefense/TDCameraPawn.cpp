// Copyright Epic Games, Inc. All Rights Reserved.

#include "TowerDefense/TDCameraPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/FloatingPawnMovement.h"

ATDCameraPawn::ATDCameraPawn()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SceneRoot);
	Camera->SetRelativeLocation(FVector(-4000.0f, 4000.0f, 4000.0f));
	Camera->SetRelativeRotation(FRotator(-35.264f, -45.0f, 0.0f));
	Camera->ProjectionMode = ECameraProjectionMode::Orthographic;
	Camera->OrthoWidth = InitialOrthoWidth;
	Camera->AutoPlaneShift = 1.0f;
	Camera->bUpdateOrthoPlanes = false;

	FloatingPawnMovement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("FloatingPawnMovement"));
	FloatingPawnMovement->UpdatedComponent = SceneRoot;
	FloatingPawnMovement->MaxSpeed = 2500.0f;
	FloatingPawnMovement->Acceleration = 8000.0f;
	FloatingPawnMovement->Deceleration = 8000.0f;
	FloatingPawnMovement->bConstrainToPlane = true;
	FloatingPawnMovement->SetPlaneConstraintNormal(FVector::UpVector);
	FloatingPawnMovement->SetPlaneConstraintOrigin(FVector::ZeroVector);
}

void ATDCameraPawn::MoveCamera(const FVector2D& Input)
{
	if (Input.IsNearlyZero())
	{
		return;
	}

	const FRotator PlanarRotation(0.0f, Camera->GetComponentRotation().Yaw, 0.0f);
	const FVector Forward = PlanarRotation.RotateVector(FVector::ForwardVector);
	const FVector Right = PlanarRotation.RotateVector(FVector::RightVector);
	AddMovementInput(Forward, Input.Y);
	AddMovementInput(Right, Input.X);
}

void ATDCameraPawn::PanByScreenDelta(const FVector2D& PixelDelta, const float ViewportWidth)
{
	if (PixelDelta.IsNearlyZero() || ViewportWidth <= 0.0f)
	{
		return;
	}

	const FRotator PlanarRotation(0.0f, Camera->GetComponentRotation().Yaw, 0.0f);
	const FVector Forward = PlanarRotation.RotateVector(FVector::ForwardVector);
	const FVector Right = PlanarRotation.RotateVector(FVector::RightVector);
	const float WorldUnitsPerPixel = Camera->OrthoWidth / ViewportWidth;
	const FVector Offset = ((-Right * PixelDelta.X) + (Forward * PixelDelta.Y)) * WorldUnitsPerPixel * DragPanMultiplier;
	AddActorWorldOffset(Offset, false);
}

void ATDCameraPawn::ApplyZoomInput(const float Value)
{
	if (FMath::IsNearlyZero(Value))
	{
		return;
	}

	const float NewWidth = Camera->OrthoWidth * FMath::Pow(ZoomFactorPerStep, Value);
	Camera->SetOrthoWidth(FMath::Clamp(NewWidth, MinimumOrthoWidth, MaximumOrthoWidth));
}
