// Copyright Epic Games, Inc. All Rights Reserved.

#include "TowerDefense/TDTowerPlacementPreview.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "TowerDefense/TDTowerBase.h"
#include "UObject/ConstructorHelpers.h"

ATDTowerPlacementPreview::ATDTowerPlacementPreview()
{
	PrimaryActorTick.bCanEverTick = false;
	SetActorEnableCollision(false);
	Tags.Add(TEXT("TD_PlacementPreview"));

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);

	FootprintMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FootprintMesh"));
	FootprintMesh->SetupAttachment(SceneRoot);
	FootprintMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FootprintMesh->SetGenerateOverlapEvents(false);
	FootprintMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 4.0f));
	FootprintMesh->SetRelativeScale3D(FVector(2.5f, 2.5f, 0.08f));

	TowerSilhouette = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TowerSilhouette"));
	TowerSilhouette->SetupAttachment(SceneRoot);
	TowerSilhouette->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TowerSilhouette->SetGenerateOverlapEvents(false);
	TowerSilhouette->SetRelativeLocation(FVector(0.0f, 0.0f, 50.0f));
	TowerSilhouette->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.0f));

	RapidBarrelSilhouetteA = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RapidBarrelSilhouetteA"));
	RapidBarrelSilhouetteA->SetupAttachment(SceneRoot);
	RapidBarrelSilhouetteA->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RapidBarrelSilhouetteA->SetGenerateOverlapEvents(false);
	RapidBarrelSilhouetteA->SetRelativeLocation(FVector(65.0f, -18.0f, 82.0f));
	RapidBarrelSilhouetteA->SetRelativeScale3D(FVector(0.9f, 0.08f, 0.08f));
	RapidBarrelSilhouetteA->SetVisibility(false);

	RapidBarrelSilhouetteB = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RapidBarrelSilhouetteB"));
	RapidBarrelSilhouetteB->SetupAttachment(SceneRoot);
	RapidBarrelSilhouetteB->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RapidBarrelSilhouetteB->SetGenerateOverlapEvents(false);
	RapidBarrelSilhouetteB->SetRelativeLocation(FVector(65.0f, 18.0f, 82.0f));
	RapidBarrelSilhouetteB->SetRelativeScale3D(FVector(0.9f, 0.08f, 0.08f));
	RapidBarrelSilhouetteB->SetVisibility(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		FootprintMesh->SetStaticMesh(CylinderMesh.Object);
		TowerSilhouette->SetStaticMesh(CylinderMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		RapidBarrelSilhouetteA->SetStaticMesh(CubeMesh.Object);
		RapidBarrelSilhouetteB->SetStaticMesh(CubeMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ValidMaterialFinder(
		TEXT("/Game/TowerDefense/Towers/Placement/M_TDPlacementValid.M_TDPlacementValid"));
	if (ValidMaterialFinder.Succeeded())
	{
		ValidMaterial = ValidMaterialFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> InvalidMaterialFinder(
		TEXT("/Game/TowerDefense/Towers/Placement/M_TDPlacementInvalid.M_TDPlacementInvalid"));
	if (InvalidMaterialFinder.Succeeded())
	{
		InvalidMaterial = InvalidMaterialFinder.Object;
	}

	ApplyFeedbackMaterial();
}

void ATDTowerPlacementPreview::ConfigureForTower(const TSubclassOf<ATDTowerBase> TowerClass)
{
	const ATDTowerBase* TowerDefaults = TowerClass ? TowerClass->GetDefaultObject<ATDTowerBase>() : nullptr;
	const bool bRapidShape = TowerDefaults && TowerDefaults->bUseRapidPlaceholderShape;
	RapidBarrelSilhouetteA->SetVisibility(bRapidShape);
	RapidBarrelSilhouetteB->SetVisibility(bRapidShape);
	TowerSilhouette->SetRelativeScale3D(bRapidShape ? FVector(0.8f, 0.95f, 0.85f) : FVector(0.8f, 0.8f, 1.0f));
	ApplyFeedbackMaterial();
}

void ATDTowerPlacementPreview::SetPlacementValid(const bool bIsValid)
{
	if (bPlacementValid == bIsValid)
	{
		return;
	}

	bPlacementValid = bIsValid;
	ApplyFeedbackMaterial();
}

void ATDTowerPlacementPreview::ApplyFeedbackMaterial()
{
	UMaterialInterface* FeedbackMaterial = bPlacementValid ? ValidMaterial : InvalidMaterial;
	if (!FeedbackMaterial)
	{
		return;
	}

	FootprintMesh->SetMaterial(0, FeedbackMaterial);
	TowerSilhouette->SetMaterial(0, FeedbackMaterial);
	RapidBarrelSilhouetteA->SetMaterial(0, FeedbackMaterial);
	RapidBarrelSilhouetteB->SetMaterial(0, FeedbackMaterial);
}
