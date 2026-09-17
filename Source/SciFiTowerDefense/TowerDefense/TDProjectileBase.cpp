// Copyright Epic Games, Inc. All Rights Reserved.

#include "TowerDefense/TDProjectileBase.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "TowerDefense/TDEnemyBase.h"
#include "UObject/ConstructorHelpers.h"

ATDProjectileBase::ATDProjectileBase()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	SetRootComponent(CollisionSphere);
	CollisionSphere->InitSphereRadius(14.0f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionSphere->SetGenerateOverlapEvents(true);
	CollisionSphere->SetCanEverAffectNavigation(false);
	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ATDProjectileBase::HandleProjectileOverlap);

	ProjectileVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileVisual"));
	ProjectileVisual->SetupAttachment(CollisionSphere);
	ProjectileVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjectileVisual->SetRelativeScale3D(FVector(0.45f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		ProjectileVisual->SetStaticMesh(SphereMesh.Object);
	}

	if (bUseCylinderVisual)
	{
		if (UStaticMesh* CylinderMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")))
		{
			ProjectileVisual->SetStaticMesh(CylinderMesh);
			ProjectileVisual->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
			ProjectileVisual->SetRelativeScale3D(FVector(0.18f, 0.18f, 0.60f));
		}
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> EmissiveMaterial(
		TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"));
	if (EmissiveMaterial.Succeeded())
	{
		ProjectileVisual->SetMaterial(0, EmissiveMaterial.Object);
	}

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionSphere;
	ProjectileMovement->InitialSpeed = ProjectileSpeed;
	ProjectileMovement->MaxSpeed = ProjectileSpeed;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bInitialVelocityInLocalSpace = false;
	ProjectileMovement->HomingAccelerationMagnitude = HomingAcceleration;
}

void ATDProjectileBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ProjectileMovement->InitialSpeed = ProjectileSpeed;
	ProjectileMovement->MaxSpeed = ProjectileSpeed;
	ProjectileMovement->HomingAccelerationMagnitude = HomingAcceleration;
}

void ATDProjectileBase::InitializeProjectile(ATDEnemyBase* NewTarget)
{
	TargetEnemy = NewTarget;
}

void ATDProjectileBase::BeginPlay()
{
	Super::BeginPlay();

	bHasHit = false;
	SetLifeSpan(MaxLifetime);
	ProjectileMovement->InitialSpeed = ProjectileSpeed;
	ProjectileMovement->MaxSpeed = ProjectileSpeed;
	ProjectileMovement->HomingAccelerationMagnitude = HomingAcceleration;
	ProjectileMovement->Velocity = GetActorForwardVector() * ProjectileSpeed;

	if (UMaterialInstanceDynamic* ProjectileMaterial = ProjectileVisual->CreateAndSetMaterialInstanceDynamic(0))
	{
		ProjectileMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.0f, 0.05f, 0.01f));
	}

	if (bEnableHoming && IsValid(TargetEnemy) && IsValid(TargetEnemy->GetRootComponent()))
	{
		TargetEnemy->OnDestroyed.AddDynamic(this, &ATDProjectileBase::HandleTargetDestroyed);
		ProjectileMovement->HomingTargetComponent = TargetEnemy->GetRootComponent();
		ProjectileMovement->bIsHomingProjectile = true;
	}
	else if (!bEnableHoming && IsValid(TargetEnemy))
	{
		ProjectileMovement->bIsHomingProjectile = false;
		ProjectileMovement->HomingTargetComponent = nullptr;
	}
	else
	{
		Destroy();
	}
}

void ATDProjectileBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(TargetEnemy))
	{
		TargetEnemy->OnDestroyed.RemoveDynamic(this, &ATDProjectileBase::HandleTargetDestroyed);
	}

	Super::EndPlay(EndPlayReason);
}

void ATDProjectileBase::HandleProjectileOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (bHasHit)
	{
		return;
	}

	ATDEnemyBase* Enemy = Cast<ATDEnemyBase>(OtherActor);
	if (!IsValid(TargetEnemy))
	{
		Destroy();
		return;
	}

	if (Enemy != TargetEnemy || !Enemy->IsAlive())
	{
		return;
	}

	bHasHit = true;
	UE_LOG(LogTemp, Log, TEXT("TD Projectile hit enemy: %s for %.0f damage"), *Enemy->GetActorNameOrLabel(), Damage);
	Enemy->ApplyDamage(Damage);
	Destroy();
}

void ATDProjectileBase::HandleTargetDestroyed(AActor* DestroyedActor)
{
	if (DestroyedActor == TargetEnemy && !bHasHit)
	{
		UE_LOG(LogTemp, Log, TEXT("TD Projectile removed safely after target destroyed: %s"), *GetActorNameOrLabel());
		Destroy();
	}
}
