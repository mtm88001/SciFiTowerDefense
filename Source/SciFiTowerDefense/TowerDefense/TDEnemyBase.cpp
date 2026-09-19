// Copyright Epic Games, Inc. All Rights Reserved.

#include "TowerDefense/TDEnemyBase.h"

#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TowerDefense/TDEnemyHealthBarWidget.h"
#include "TowerDefense/TDEnemyPath.h"
#include "TowerDefense/TDGameModeBase.h"
#include "UObject/ConstructorHelpers.h"

ATDEnemyBase::ATDEnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessAI = EAutoPossessAI::Disabled;
	AIControllerClass = nullptr;

	GetCapsuleComponent()->InitCapsuleSize(25.0f, 50.0f);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Overlap);

	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->GravityScale = 0.0f;

	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetGenerateOverlapEvents(false);
	GetMesh()->SetCanEverAffectNavigation(false);
	GetMesh()->SetRelativeLocation(EnemyVisualRelativeLocation);
	GetMesh()->SetRelativeRotation(EnemyVisualRelativeRotation);
	GetMesh()->SetRelativeScale3D(EnemyVisualRelativeScale);

	PlaceholderVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderVisual"));
	PlaceholderVisual->SetupAttachment(GetCapsuleComponent());
	PlaceholderVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlaceholderVisual->SetRelativeScale3D(FVector(0.6f, 0.35f, 0.8f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		PlaceholderVisual->SetStaticMesh(CubeMesh.Object);
	}

	HealthBarWidgetClass = UTDEnemyHealthBarWidget::StaticClass();

	HealthBarComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarComponent"));
	HealthBarComponent->SetupAttachment(GetCapsuleComponent());
	HealthBarComponent->SetRelativeLocation(FVector(0.0f, 0.0f, HealthBarHeightOffset));
	HealthBarComponent->SetWidgetSpace(EWidgetSpace::Screen);
	HealthBarComponent->SetDrawSize(FVector2D(80.0f, 10.0f));
	HealthBarComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HealthBarComponent->SetWidgetClass(HealthBarWidgetClass);
}

void ATDEnemyBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshVisualState();
}

void ATDEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	DistanceAlongPath = 0.0f;
	bReachedPathEnd = false;
	bDead = false;
	CurrentHealth = FMath::Max(0.0f, MaxHealth);
	SetAnimationMovementState(0.0f);
	RefreshVisualState();
	UpdateHealthBarDisplay();

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
	}

	if (!ResolveEnemyPath())
	{
		UE_LOG(LogTemp, Error, TEXT("TD Enemy '%s' has no enemy path and cannot move"), *GetName());
		SetActorTickEnabled(false);
		return;
	}

	CachedSplineLength = EnemyPath->GetPathSpline()->GetSplineLength();
	UpdateTransformFromPath();
	UE_LOG(LogTemp, Log, TEXT("TD Enemy '%s' path assigned: %s | spline length: %.2f | movement speed: %.2f"),
		*GetName(), *EnemyPath->GetName(), CachedSplineLength, MovementSpeed);
}

void ATDEnemyBase::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bDead || bReachedPathEnd || !IsValid(EnemyPath) || MovementSpeed <= 0.0f)
	{
		SetAnimationMovementState(0.0f);
		return;
	}

	const FVector PreviousLocation = GetActorLocation();
	DistanceAlongPath = FMath::Min(DistanceAlongPath + (MovementSpeed * DeltaSeconds), CachedSplineLength);
	UpdateTransformFromPath();
	const float ActualSpeed = DeltaSeconds > UE_SMALL_NUMBER
		? FVector::Dist2D(PreviousLocation, GetActorLocation()) / DeltaSeconds
		: 0.0f;
	SetAnimationMovementState(ActualSpeed);

	if (DistanceAlongPath >= CachedSplineLength)
	{
		bReachedPathEnd = true;
		SetAnimationMovementState(0.0f);
		SetActorTickEnabled(false);
		UE_LOG(LogTemp, Log, TEXT("TD Enemy reached end of path: %s"), *GetName());
		OnReachedPathEnd();
		OnEnemyEscaped.Broadcast(this);
		if (ATDGameModeBase* GameMode = Cast<ATDGameModeBase>(UGameplayStatics::GetGameMode(this)))
		{
			GameMode->ApplyBaseDamage(EscapeDamage);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("TD Enemy '%s' could not find ATDGameModeBase for escape damage"), *GetName());
		}
		Destroy();
	}
}

void ATDEnemyBase::ApplyDamage(const float DamageAmount)
{
	if (bDead || DamageAmount <= 0.0f)
	{
		return;
	}

	const float AppliedDamage = FMath::Min(DamageAmount, CurrentHealth);
	CurrentHealth = FMath::Clamp(CurrentHealth - DamageAmount, 0.0f, MaxHealth);
	UE_LOG(LogTemp, Log, TEXT("TD Enemy %s took %.0f damage. Health: %.0f/%.0f"),
		*GetActorNameOrLabel(), AppliedDamage, CurrentHealth, MaxHealth);
	UpdateHealthBarDisplay();

	if (CurrentHealth <= 0.0f)
	{
		Die();
	}
}

void ATDEnemyBase::Die()
{
	if (bDead)
	{
		return;
	}

	bDead = true;
	bReachedPathEnd = true;
	SetAnimationMovementState(0.0f);
	SetActorTickEnabled(false);
	UE_LOG(LogTemp, Log, TEXT("TD Enemy died: %s"), *GetActorNameOrLabel());
	if (ATDGameModeBase* GameMode = Cast<ATDGameModeBase>(UGameplayStatics::GetGameMode(this)))
	{
		GameMode->AwardEnemyKillCredits(KillReward);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("TD Enemy '%s' could not find ATDGameModeBase for kill reward"), *GetName());
	}
	OnDeath();
	OnEnemyKilled.Broadcast(this);
	Destroy();
}

bool ATDEnemyBase::ResolveEnemyPath()
{
	if (IsValid(EnemyPath) && IsValid(EnemyPath->GetPathSpline()))
	{
		return true;
	}

	for (TActorIterator<ATDEnemyPath> It(GetWorld()); It; ++It)
	{
		EnemyPath = *It;
		UE_LOG(LogTemp, Warning, TEXT("TD Enemy '%s' used the single-path BeginPlay fallback: %s"), *GetName(), *EnemyPath->GetName());
		return IsValid(EnemyPath->GetPathSpline());
	}

	return false;
}

void ATDEnemyBase::RefreshVisualState()
{
	USkeletalMeshComponent* EnemyVisual = GetMesh();
	EnemyVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EnemyVisual->SetGenerateOverlapEvents(false);
	EnemyVisual->SetCanEverAffectNavigation(false);
	EnemyVisual->SetRelativeLocation(EnemyVisualRelativeLocation);
	EnemyVisual->SetRelativeRotation(EnemyVisualRelativeRotation);
	EnemyVisual->SetRelativeScale3D(EnemyVisualRelativeScale);

	const bool bHasSkeletalVisual = EnemyVisual->GetSkeletalMeshAsset() != nullptr;
	EnemyVisual->SetVisibility(bHasSkeletalVisual, true);
	EnemyVisual->SetHiddenInGame(!bHasSkeletalVisual);
	PlaceholderVisual->SetVisibility(!bHasSkeletalVisual, true);
	PlaceholderVisual->SetHiddenInGame(bHasSkeletalVisual);
}

void ATDEnemyBase::UpdateHealthBarDisplay()
{
	if (!CachedHealthBarWidget.IsValid() && IsValid(HealthBarComponent))
	{
		CachedHealthBarWidget = Cast<UTDEnemyHealthBarWidget>(HealthBarComponent->GetUserWidgetObject());
	}

	if (UTDEnemyHealthBarWidget* Widget = CachedHealthBarWidget.Get())
	{
		Widget->SetHealthPercent(MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f);
	}
}

void ATDEnemyBase::SetAnimationMovementState(const float Speed)
{
	MovementSpeedForAnimation = FMath::Max(0.0f, Speed);
	bIsMovingForAnimation = MovementSpeedForAnimation > 1.0f;
	const FVector AnimationMotion = bIsMovingForAnimation
		? GetActorForwardVector() * MovementSpeedForAnimation
		: FVector::ZeroVector;
	GetCharacterMovement()->Velocity = AnimationMotion;
}

void ATDEnemyBase::UpdateTransformFromPath()
{
	USplineComponent* Spline = EnemyPath->GetPathSpline();
	const FVector PathLocation = Spline->GetLocationAtDistanceAlongSpline(DistanceAlongPath, ESplineCoordinateSpace::World);
	const FVector PathDirection = Spline->GetDirectionAtDistanceAlongSpline(DistanceAlongPath, ESplineCoordinateSpace::World);
	const float PathYaw = PathDirection.Rotation().Yaw;

	SetActorLocationAndRotation(
		PathLocation + FVector(0.0f, 0.0f, PathHeightOffset),
		FRotator(0.0f, PathYaw, 0.0f),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
}
