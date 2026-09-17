// Copyright Epic Games, Inc. All Rights Reserved.

#include "TowerDefense/TDWaveSpawner.h"

#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "TowerDefense/TDEnemyBase.h"
#include "TowerDefense/TDEnemyPath.h"
#include "TowerDefense/TDGameModeBase.h"

ATDWaveSpawner::ATDWaveSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);
}

void ATDWaveSpawner::BeginPlay()
{
	Super::BeginPlay();

	CurrentWave = 0;
	CurrentWaveEnemiesScheduled = 0;
	EnemiesSpawned = 0;
	EnemiesAlive = 0;
	EnemiesKilled = 0;
	EnemiesEscaped = 0;
	EnemiesRemaining = 0;
	TotalEnemiesSpawned = 0;
	TotalEnemiesKilled = 0;
	TotalEnemiesEscaped = 0;
	ActiveEnemies.Reset();
	bCurrentWaveCompleted = false;
	bStopped = false;

	TDGameMode = Cast<ATDGameModeBase>(UGameplayStatics::GetGameMode(this));
	if (!IsValid(TDGameMode))
	{
		UE_LOG(LogTemp, Error, TEXT("TD Waves cannot start: active GameMode is not ATDGameModeBase"));
		bStopped = true;
		return;
	}
	TDGameMode->OnGameOver.AddDynamic(this, &ATDWaveSpawner::HandleGameOver);

	if (!EnemyClass || !IsValid(EnemyPath) || !IsValid(EnemyPath->GetPathSpline()))
	{
		UE_LOG(LogTemp, Error, TEXT("TD Waves cannot start: %s requires EnemyClass and EnemyPath"), *GetActorNameOrLabel());
		bStopped = true;
		return;
	}

	if (InitialSpawnDelay <= 0.0f)
	{
		StartNextWave();
	}
	else
	{
		GetWorldTimerManager().SetTimer(NextWaveTimerHandle, this, &ATDWaveSpawner::StartNextWave, InitialSpawnDelay, false);
	}
}

void ATDWaveSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
	GetWorldTimerManager().ClearTimer(NextWaveTimerHandle);
	GetWorldTimerManager().ClearTimer(WaveResolutionTimerHandle);

	if (IsValid(TDGameMode))
	{
		TDGameMode->OnGameOver.RemoveDynamic(this, &ATDWaveSpawner::HandleGameOver);
	}

	for (ATDEnemyBase* Enemy : ActiveEnemies)
	{
		if (IsValid(Enemy))
		{
			Enemy->OnEnemyKilled.RemoveDynamic(this, &ATDWaveSpawner::HandleEnemyKilled);
			Enemy->OnEnemyEscaped.RemoveDynamic(this, &ATDWaveSpawner::HandleEnemyEscaped);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void ATDWaveSpawner::StartNextWave()
{
	if (bStopped || !IsValid(TDGameMode) || TDGameMode->bGameOver || TDGameMode->bGameWon || CurrentWave >= TotalWaves)
	{
		return;
	}

	++CurrentWave;
	CurrentWaveEnemiesScheduled = FMath::Max(1, BaseEnemiesPerWave + ((CurrentWave - 1) * EnemiesAddedPerWave));
	EnemiesSpawned = 0;
	EnemiesAlive = 0;
	EnemiesKilled = 0;
	EnemiesEscaped = 0;
	EnemiesRemaining = CurrentWaveEnemiesScheduled;
	ActiveEnemies.Reset();
	bCurrentWaveCompleted = false;

	UE_LOG(LogTemp, Log, TEXT("TD Wave %d started. Scheduled: %d"), CurrentWave, CurrentWaveEnemiesScheduled);
	OnWaveStarted.Broadcast(CurrentWave);
	OnEnemiesRemainingChanged.Broadcast(EnemiesRemaining);
	SpawnNextEnemy();
}

void ATDWaveSpawner::SpawnNextEnemy()
{
	if (bStopped || !IsValid(TDGameMode) || TDGameMode->bGameOver || TDGameMode->bGameWon)
	{
		return;
	}

	if (EnemiesSpawned >= CurrentWaveEnemiesScheduled || !EnemyClass || !IsValid(EnemyPath))
	{
		TryCompleteWave();
		return;
	}

	USplineComponent* PathSpline = EnemyPath->GetPathSpline();
	if (!IsValid(PathSpline))
	{
		UE_LOG(LogTemp, Error, TEXT("TD Wave stopped: assigned path has no spline"));
		bStopped = true;
		return;
	}

	const ATDEnemyBase* EnemyDefaults = EnemyClass->GetDefaultObject<ATDEnemyBase>();
	const FVector SpawnLocation = PathSpline->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World)
		+ FVector(0.0f, 0.0f, EnemyDefaults ? EnemyDefaults->PathHeightOffset : 50.0f);
	const FRotator SpawnRotation = PathSpline->GetRotationAtSplinePoint(0, ESplineCoordinateSpace::World);
	const FTransform SpawnTransform(SpawnRotation, SpawnLocation);

	ATDEnemyBase* Enemy = GetWorld()->SpawnActorDeferred<ATDEnemyBase>(
		EnemyClass,
		SpawnTransform,
		this,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Enemy)
	{
		UE_LOG(LogTemp, Error, TEXT("TD Wave %d failed to spawn enemy %d/%d"),
			CurrentWave, EnemiesSpawned + 1, CurrentWaveEnemiesScheduled);
		bStopped = true;
		return;
	}

	Enemy->EnemyPath = EnemyPath;
	Enemy->OnEnemyKilled.AddDynamic(this, &ATDWaveSpawner::HandleEnemyKilled);
	Enemy->OnEnemyEscaped.AddDynamic(this, &ATDWaveSpawner::HandleEnemyEscaped);

	++EnemiesSpawned;
	++EnemiesAlive;
	++TotalEnemiesSpawned;
	ActiveEnemies.Add(Enemy);
	UE_LOG(LogTemp, Log, TEXT("TD Wave %d spawning enemy %d/%d"),
		CurrentWave, EnemiesSpawned, CurrentWaveEnemiesScheduled);
	UGameplayStatics::FinishSpawningActor(Enemy, SpawnTransform);

	if (EnemiesSpawned < CurrentWaveEnemiesScheduled)
	{
		ScheduleNextSpawn();
	}
	else
	{
		TryCompleteWave();
	}
}

void ATDWaveSpawner::ScheduleNextSpawn()
{
	if (!bStopped)
	{
		GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &ATDWaveSpawner::SpawnNextEnemy,
			FMath::Max(SpawnInterval, 0.01f), false);
	}
}

void ATDWaveSpawner::HandleEnemyKilled(ATDEnemyBase* Enemy)
{
	ResolveEnemy(Enemy, true);
}

void ATDWaveSpawner::HandleEnemyEscaped(ATDEnemyBase* Enemy)
{
	ResolveEnemy(Enemy, false);
}

void ATDWaveSpawner::ResolveEnemy(ATDEnemyBase* Enemy, const bool bWasKilled)
{
	if (!IsValid(Enemy) || ActiveEnemies.RemoveSingle(Enemy) == 0)
	{
		return;
	}

	EnemiesAlive = FMath::Max(0, EnemiesAlive - 1);
	EnemiesRemaining = FMath::Max(0, CurrentWaveEnemiesScheduled - EnemiesKilled - EnemiesEscaped - 1);
	if (bWasKilled)
	{
		++EnemiesKilled;
		++TotalEnemiesKilled;
		UE_LOG(LogTemp, Log, TEXT("TD Wave %d enemy killed. Alive: %d"), CurrentWave, EnemiesAlive);
	}
	else
	{
		++EnemiesEscaped;
		++TotalEnemiesEscaped;
		UE_LOG(LogTemp, Log, TEXT("TD Wave %d enemy escaped. Alive: %d"), CurrentWave, EnemiesAlive);
	}
	OnEnemiesRemainingChanged.Broadcast(EnemiesRemaining);

	TryCompleteWave();
}

void ATDWaveSpawner::TryCompleteWave()
{
	if (bStopped || bCurrentWaveCompleted || EnemiesSpawned != CurrentWaveEnemiesScheduled || EnemiesAlive != 0
		|| GetWorldTimerManager().IsTimerActive(WaveResolutionTimerHandle))
	{
		return;
	}

	// Escape accounting broadcasts before base damage. Deferring completion ensures that damage
	// and a possible game-over event happen before a final-wave victory is evaluated.
	GetWorldTimerManager().SetTimer(WaveResolutionTimerHandle, this, &ATDWaveSpawner::CompleteCurrentWave, 0.001f, false);
}

void ATDWaveSpawner::CompleteCurrentWave()
{
	if (bStopped || bCurrentWaveCompleted || !IsValid(TDGameMode) || TDGameMode->bGameOver)
	{
		return;
	}

	bCurrentWaveCompleted = true;
	UE_LOG(LogTemp, Log, TEXT("TD Wave %d complete. Spawned: %d, Killed: %d, Escaped: %d"),
		CurrentWave, EnemiesSpawned, EnemiesKilled, EnemiesEscaped);
	OnWaveCompleted.Broadcast(CurrentWave);

	if (CurrentWave >= TotalWaves)
	{
		TDGameMode->DeclareVictory();
	}
	else
	{
		GetWorldTimerManager().SetTimer(NextWaveTimerHandle, this, &ATDWaveSpawner::StartNextWave,
			FMath::Max(TimeBetweenWaves, 0.0f), false);
	}
}

void ATDWaveSpawner::HandleGameOver()
{
	if (bStopped)
	{
		return;
	}

	bStopped = true;
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
	GetWorldTimerManager().ClearTimer(NextWaveTimerHandle);
	GetWorldTimerManager().ClearTimer(WaveResolutionTimerHandle);
	UE_LOG(LogTemp, Log, TEXT("TD Wave spawner halted for game over"));
}
