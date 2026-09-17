// Copyright Epic Games, Inc. All Rights Reserved.

#include "TowerDefense/TDTowerBase.h"

#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TowerDefense/TDEnemyBase.h"
#include "TowerDefense/TDProjectileBase.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float TargetScanIntervalSeconds = 0.25f;
}

ATDTowerBase::ATDTowerBase()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	BaseMesh->SetupAttachment(SceneRoot);
	BaseMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BaseMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 25.0f));
	BaseMesh->SetRelativeScale3D(FVector(0.8f, 0.8f, 0.5f));

	TurretPivot = CreateDefaultSubobject<USceneComponent>(TEXT("TurretPivot"));
	TurretPivot->SetupAttachment(SceneRoot);
	TurretPivot->SetRelativeLocation(FVector(0.0f, 0.0f, 65.0f));

	TurretMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TurretMesh"));
	TurretMesh->SetupAttachment(TurretPivot);
	TurretMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TurretMesh->SetRelativeScale3D(FVector(0.55f, 0.45f, 0.3f));

	GunPivot = CreateDefaultSubobject<USceneComponent>(TEXT("GunPivot"));
	GunPivot->SetupAttachment(TurretPivot);
	GunPivot->SetRelativeLocation(FVector(0.0f, 0.0f, 15.0f));

	GunMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GunMesh"));
	GunMesh->SetupAttachment(GunPivot);
	GunMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GunMesh->SetRelativeLocation(FVector(75.0f, 0.0f, 0.0f));
	GunMesh->SetRelativeScale3D(FVector(1.2f, 0.12f, 0.12f));

	SecondaryGunMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SecondaryGunMesh"));
	SecondaryGunMesh->SetupAttachment(GunPivot);
	SecondaryGunMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SecondaryGunMesh->SetVisibility(false);

	MuzzlePoint = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzlePoint"));
	MuzzlePoint->SetupAttachment(GunPivot);
	MuzzlePoint->SetRelativeLocation(FVector(135.0f, 0.0f, 0.0f));

	DetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionSphere"));
	DetectionSphere->SetupAttachment(SceneRoot);
	DetectionSphere->SetRelativeLocation(FVector(0.0f, 0.0f, 50.0f));
	DetectionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DetectionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	DetectionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	DetectionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	DetectionSphere->SetGenerateOverlapEvents(true);
	DetectionSphere->SetHiddenInGame(true);
	DetectionSphere->ShapeColor = FColor::Cyan;
	DetectionSphere->InitSphereRadius(DetectionRange);
	DetectionSphere->OnComponentBeginOverlap.AddDynamic(this, &ATDTowerBase::HandleDetectionBeginOverlap);
	DetectionSphere->OnComponentEndOverlap.AddDynamic(this, &ATDTowerBase::HandleDetectionEndOverlap);

	PlacementFootprint = CreateDefaultSubobject<USphereComponent>(TEXT("PlacementFootprint"));
	PlacementFootprint->SetupAttachment(SceneRoot);
	PlacementFootprint->SetRelativeLocation(FVector(0.0f, 0.0f, 50.0f));
	PlacementFootprint->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PlacementFootprint->SetCollisionObjectType(ECC_WorldDynamic);
	PlacementFootprint->SetCollisionResponseToAllChannels(ECR_Ignore);
	PlacementFootprint->SetCollisionResponseToChannel(ECC_Visibility, ECR_Overlap);
	PlacementFootprint->SetGenerateOverlapEvents(false);
	PlacementFootprint->SetHiddenInGame(true);
	PlacementFootprint->ShapeColor = FColor::Green;
	PlacementFootprint->bDrawOnlyIfSelected = true;
	PlacementFootprint->InitSphereRadius(TowerFootprintRadius);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		TurretMesh->SetStaticMesh(CubeMesh.Object);
		GunMesh->SetStaticMesh(CubeMesh.Object);
		SecondaryGunMesh->SetStaticMesh(CubeMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		BaseMesh->SetStaticMesh(CylinderMesh.Object);
	}
}

void ATDTowerBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	DetectionSphere->SetSphereRadius(DetectionRange, true);
	PlacementFootprint->SetSphereRadius(TowerFootprintRadius, true);
	RefreshPlaceholderAppearance();
}

void ATDTowerBase::BeginPlay()
{
	Super::BeginPlay();
	ProjectilesFired = 0;

	DetectionSphere->SetSphereRadius(DetectionRange, true);
	PlacementFootprint->SetSphereRadius(TowerFootprintRadius, true);
	RefreshPlaceholderAppearance();
	ScanForNearbyTargets();
	GetWorldTimerManager().SetTimer(
		TargetScanTimerHandle,
		this,
		&ATDTowerBase::ScanForNearbyTargets,
		TargetScanIntervalSeconds,
		true);

	if (FireRate > 0.0f)
	{
		GetWorldTimerManager().SetTimer(FireTimerHandle, this, &ATDTowerBase::TryFire, 1.0f / FireRate, true);
	}

	UE_LOG(LogTemp, Log, TEXT("TD Tower ready: %s | range: %.0f | fire rate: %.2f | projectile: %s"),
		*GetActorNameOrLabel(), DetectionRange, FireRate, ProjectileClass ? *ProjectileClass->GetName() : TEXT("None"));
}

void ATDTowerBase::RefreshPlaceholderAppearance()
{
	if (bUseRapidPlaceholderShape)
	{
		TurretMesh->SetRelativeScale3D(FVector(0.55f, 0.65f, 0.25f));
		GunMesh->SetRelativeLocation(FVector(65.0f, -18.0f, 0.0f));
		GunMesh->SetRelativeScale3D(FVector(0.9f, 0.08f, 0.08f));
		SecondaryGunMesh->SetRelativeLocation(FVector(65.0f, 18.0f, 0.0f));
		SecondaryGunMesh->SetRelativeScale3D(FVector(0.9f, 0.08f, 0.08f));
		SecondaryGunMesh->SetVisibility(true);
		MuzzlePoint->SetRelativeLocation(FVector(120.0f, -18.0f, 0.0f));
	}
	else if (BaseMesh->GetStaticMesh() == nullptr || BaseMesh->GetStaticMesh()->GetPathName().StartsWith(TEXT("/Engine/BasicShapes/")))
	{
		TurretMesh->SetRelativeScale3D(FVector(0.55f, 0.45f, 0.3f));
		GunMesh->SetRelativeLocation(FVector(75.0f, 0.0f, 0.0f));
		GunMesh->SetRelativeScale3D(FVector(1.2f, 0.12f, 0.12f));
		SecondaryGunMesh->SetVisibility(false);
		MuzzlePoint->SetRelativeLocation(FVector(135.0f, 0.0f, 0.0f));
	}
}

void ATDTowerBase::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	RotateWeaponTowardTarget(DeltaSeconds);
}

void ATDTowerBase::ScanForNearbyTargets()
{
	if (!IsValid(DetectionSphere) || !IsValid(GetWorld()))
	{
		return;
	}

	TArray<FOverlapResult> OverlapResults;
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TDTowerTargetScan), false, this);

	GetWorld()->OverlapMultiByObjectType(
		OverlapResults,
		DetectionSphere->GetComponentLocation(),
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(DetectionRange),
		QueryParams);

	for (const FOverlapResult& Overlap : OverlapResults)
	{
		if (ATDEnemyBase* Enemy = Cast<ATDEnemyBase>(Overlap.GetActor()); IsValid(Enemy) && Enemy->IsAlive())
		{
			CandidateTargets.AddUnique(Enemy);
		}
	}

	RefreshCurrentTarget();
}

ATDEnemyBase* ATDTowerBase::FindBestTarget()
{
	const FVector DetectionOrigin = DetectionSphere->GetComponentLocation();
	const float RangeSquared = FMath::Square(DetectionRange);

	CandidateTargets.RemoveAll([&](const TObjectPtr<ATDEnemyBase>& Enemy)
	{
		return !IsValid(Enemy) || !Enemy->IsAlive() || FVector::DistSquared(DetectionOrigin, Enemy->GetActorLocation()) > RangeSquared;
	});
	CandidateTargetCount = CandidateTargets.Num();

	ATDEnemyBase* BestTarget = nullptr;
	float BestDistanceSquared = TNumericLimits<float>::Max();
	for (ATDEnemyBase* Enemy : CandidateTargets)
	{
		const float DistanceSquared = FVector::DistSquared(DetectionOrigin, Enemy->GetActorLocation());
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestTarget = Enemy;
		}
	}

	return BestTarget;
}

void ATDTowerBase::TryFire()
{
	if (!IsValid(CurrentTarget) || !CurrentTarget->IsAlive() || !ProjectileClass || !IsValid(MuzzlePoint))
	{
		return;
	}

	const FVector DetectionOrigin = DetectionSphere->GetComponentLocation();
	if (FVector::DistSquared(DetectionOrigin, CurrentTarget->GetActorLocation()) > FMath::Square(DetectionRange))
	{
		RefreshCurrentTarget();
		return;
	}

	const FVector SpawnLocation = MuzzlePoint->GetComponentLocation();
	const FVector AimDirection = CurrentTarget->GetActorLocation() - SpawnLocation;
	if (AimDirection.IsNearlyZero())
	{
		return;
	}

	const FTransform SpawnTransform(AimDirection.Rotation(), SpawnLocation);
	ATDProjectileBase* Projectile = GetWorld()->SpawnActorDeferred<ATDProjectileBase>(
		ProjectileClass,
		SpawnTransform,
		this,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Projectile)
	{
		return;
	}

	Projectile->InitializeProjectile(CurrentTarget);
	UGameplayStatics::FinishSpawningActor(Projectile, SpawnTransform);
	++ProjectilesFired;
	UE_LOG(LogTemp, Log, TEXT("TD Tower %s fired projectile %d at: %s"),
		*GetActorNameOrLabel(), ProjectilesFired, *CurrentTarget->GetActorNameOrLabel());
}

void ATDTowerBase::HandleDetectionBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (ATDEnemyBase* Enemy = Cast<ATDEnemyBase>(OtherActor))
	{
		CandidateTargets.AddUnique(Enemy);
		RefreshCurrentTarget();
	}
}

void ATDTowerBase::HandleDetectionEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex)
{
	if (ATDEnemyBase* Enemy = Cast<ATDEnemyBase>(OtherActor))
	{
		CandidateTargets.Remove(Enemy);
		RefreshCurrentTarget();
	}
}

void ATDTowerBase::RefreshCurrentTarget()
{
	ATDEnemyBase* BestTarget = FindBestTarget();
	if (BestTarget == CurrentTarget)
	{
		return;
	}

	if (IsValid(CurrentTarget))
	{
		UE_LOG(LogTemp, Log, TEXT("TD Tower lost target: %s"), *CurrentTarget->GetActorNameOrLabel());
	}

	CurrentTarget = BestTarget;
	if (IsValid(CurrentTarget))
	{
		UE_LOG(LogTemp, Log, TEXT("TD Tower acquired target: %s"), *CurrentTarget->GetActorNameOrLabel());
	}
}

void ATDTowerBase::RotateWeaponTowardTarget(const float DeltaSeconds)
{
	if (!IsValid(CurrentTarget) || !IsValid(TurretPivot) || !IsValid(GunPivot))
	{
		return;
	}

	const FVector DirectionToTarget = CurrentTarget->GetActorLocation() - TurretPivot->GetComponentLocation();
	if (DirectionToTarget.IsNearlyZero())
	{
		return;
	}

	const float DesiredWorldYaw = DirectionToTarget.Rotation().Yaw;
	const float DesiredLocalYaw = FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw, DesiredWorldYaw) - BarrelYawOffset;
	const FRotator DesiredRotation(0.0f, DesiredLocalYaw, 0.0f);
	const FRotator NewRotation = FMath::RInterpConstantTo(
		TurretPivot->GetRelativeRotation(),
		DesiredRotation,
		DeltaSeconds,
		TurretRotationSpeed);

	TurretPivot->SetRelativeRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));

	const FVector GunDirection = CurrentTarget->GetActorLocation() - GunPivot->GetComponentLocation();
	if (GunDirection.IsNearlyZero())
	{
		return;
	}

	const float DesiredWorldPitch = GunDirection.Rotation().Pitch;
	const float ParentWorldPitch = TurretPivot->GetComponentRotation().Pitch;
	const float DesiredLocalPitch = FMath::Clamp(
		FMath::FindDeltaAngleDegrees(ParentWorldPitch, DesiredWorldPitch),
		MinimumBarrelPitch,
		MaximumBarrelPitch);
	const float NewPitch = FMath::FInterpConstantTo(
		GunPivot->GetRelativeRotation().Pitch,
		DesiredLocalPitch,
		DeltaSeconds,
		BarrelRotationSpeed);

	GunPivot->SetRelativeRotation(FRotator(NewPitch, BarrelYawOffset, 0.0f));
}
