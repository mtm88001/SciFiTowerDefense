// Copyright Epic Games, Inc. All Rights Reserved.

#include "TowerDefense/TDEnemyPath.h"

#include "Components/ArrowComponent.h"
#include "Components/SplineComponent.h"

ATDEnemyPath::ATDEnemyPath()
{
	PrimaryActorTick.bCanEverTick = false;

	PathSpline = CreateDefaultSubobject<USplineComponent>(TEXT("PathSpline"));
	SetRootComponent(PathSpline);
	PathSpline->SetClosedLoop(false, false);
	PathSpline->bDrawDebug = true;
	PathSpline->EditorUnselectedSplineSegmentColor = FLinearColor(0.0f, 0.8f, 1.0f);
	PathSpline->EditorSelectedSplineSegmentColor = FLinearColor(1.0f, 0.85f, 0.0f);

	PathSpline->ClearSplinePoints(false);
	const FVector RoutePoints[] = {
		FVector(-1700.0f, -750.0f, 5.0f),
		FVector(-700.0f, -750.0f, 5.0f),
		FVector(-700.0f, 450.0f, 5.0f),
		FVector(300.0f, 450.0f, 5.0f),
		FVector(300.0f, -450.0f, 5.0f),
		FVector(1600.0f, -450.0f, 5.0f)
	};
	for (const FVector& Point : RoutePoints)
	{
		PathSpline->AddSplinePoint(Point, ESplineCoordinateSpace::Local, false);
	}
	for (int32 PointIndex = 0; PointIndex < PathSpline->GetNumberOfSplinePoints(); ++PointIndex)
	{
		PathSpline->SetSplinePointType(PointIndex, ESplinePointType::Linear, false);
	}
	PathSpline->UpdateSpline();

	StartArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("Start"));
	StartArrow->SetupAttachment(PathSpline);
	StartArrow->SetArrowColor(FColor::Green);
	StartArrow->ArrowSize = 2.5f;
	StartArrow->SetHiddenInGame(true);

	EndArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("End"));
	EndArrow->SetupAttachment(PathSpline);
	EndArrow->SetArrowColor(FColor::Red);
	EndArrow->ArrowSize = 2.5f;
	EndArrow->SetHiddenInGame(true);
}

void ATDEnemyPath::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateEndpointArrows();
}

void ATDEnemyPath::UpdateEndpointArrows()
{
	const int32 PointCount = PathSpline->GetNumberOfSplinePoints();
	if (PointCount < 1)
	{
		StartArrow->SetVisibility(false);
		EndArrow->SetVisibility(false);
		return;
	}

	StartArrow->SetVisibility(true);
	EndArrow->SetVisibility(true);
	StartArrow->SetRelativeLocationAndRotation(
		PathSpline->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::Local),
		PathSpline->GetRotationAtSplinePoint(0, ESplineCoordinateSpace::Local));
	EndArrow->SetRelativeLocationAndRotation(
		PathSpline->GetLocationAtSplinePoint(PointCount - 1, ESplineCoordinateSpace::Local),
		PathSpline->GetRotationAtSplinePoint(PointCount - 1, ESplineCoordinateSpace::Local));
}
