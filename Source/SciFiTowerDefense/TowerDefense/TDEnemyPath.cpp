// Copyright Epic Games, Inc. All Rights Reserved.

#include "TowerDefense/TDEnemyPath.h"

#include "Components/ArrowComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

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
	RebuildPathVisual();
}

void ATDEnemyPath::ClearPathVisual()
{
	for (USplineMeshComponent* Segment : PathVisualSegments)
	{
		if (IsValid(Segment))
		{
			Segment->DestroyComponent();
		}
	}
	PathVisualSegments.Reset();
}

void ATDEnemyPath::RebuildPathVisual()
{
	ClearPathVisual();

	if (!PathSegmentMesh || !PathVisualMaterial || PathVisualStepLength <= 0.0f || PathTileWorldLength <= 0.0f)
	{
		return;
	}

	const float SplineLength = PathSpline->GetSplineLength();
	if (SplineLength <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float NativeWidth = PathSegmentMesh->GetBounds().BoxExtent.Y * 2.0f;
	if (NativeWidth <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	const float WidthScale = PathWidth / NativeWidth;

	const int32 NumSegments = FMath::Max(1, FMath::RoundToInt(SplineLength / PathVisualStepLength));
	const float ActualStepLength = SplineLength / NumSegments;

	for (int32 SegmentIndex = 0; SegmentIndex < NumSegments; ++SegmentIndex)
	{
		const float StartDistance = SegmentIndex * ActualStepLength;
		const float EndDistance = (SegmentIndex + 1) * ActualStepLength;

		USplineMeshComponent* Segment = NewObject<USplineMeshComponent>(this, NAME_None, RF_Transactional);
		Segment->SetMobility(EComponentMobility::Movable);
		Segment->SetupAttachment(PathSpline);
		Segment->SetStaticMesh(PathSegmentMesh);
		Segment->SetForwardAxis(ESplineMeshAxis::X, false);
		Segment->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Segment->SetCastShadow(false);

		const FVector StartPos = PathSpline->GetLocationAtDistanceAlongSpline(StartDistance, ESplineCoordinateSpace::Local);
		const FVector StartTangent = PathSpline->GetTangentAtDistanceAlongSpline(StartDistance, ESplineCoordinateSpace::Local).GetSafeNormal() * ActualStepLength;
		const FVector EndPos = PathSpline->GetLocationAtDistanceAlongSpline(EndDistance, ESplineCoordinateSpace::Local);
		const FVector EndTangent = PathSpline->GetTangentAtDistanceAlongSpline(EndDistance, ESplineCoordinateSpace::Local).GetSafeNormal() * ActualStepLength;
		Segment->SetStartAndEnd(StartPos, StartTangent, EndPos, EndTangent, false);
		Segment->SetStartScale(FVector2D(WidthScale, WidthScale), false);
		Segment->SetEndScale(FVector2D(WidthScale, WidthScale), false);

		UMaterialInstanceDynamic* SegmentMaterial = UMaterialInstanceDynamic::Create(PathVisualMaterial, this);
		SegmentMaterial->SetScalarParameterValue(TEXT("UVLengthScale"), ActualStepLength / PathTileWorldLength);
		SegmentMaterial->SetScalarParameterValue(TEXT("UVLengthOffset"), StartDistance / PathTileWorldLength);
		Segment->SetMaterial(0, SegmentMaterial);

		Segment->RegisterComponent();
		AddInstanceComponent(Segment);
		PathVisualSegments.Add(Segment);
	}
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
