// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TDWaveSpawner.generated.h"

class ATDEnemyBase;
class ATDEnemyPath;
class ATDGameModeBase;
class USceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDWaveEventSignature, int32, WaveNumber);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDEnemiesRemainingChangedSignature, int32, EnemiesRemaining);

/** Reusable sequential-wave spawner that owns and tracks its spawned enemies. */
UCLASS(Blueprintable)
class SCIFITOWERDEFENSE_API ATDWaveSpawner : public AActor
{
	GENERATED_BODY()

public:
	ATDWaveSpawner();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Wave")
	TSubclassOf<ATDEnemyBase> EnemyClass;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Tower Defense|Wave")
	TObjectPtr<ATDEnemyPath> EnemyPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Wave", meta = (ClampMin = "1"))
	int32 TotalWaves = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Wave", meta = (ClampMin = "1"))
	int32 BaseEnemiesPerWave = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Wave", meta = (ClampMin = "0"))
	int32 EnemiesAddedPerWave = 2;

	/** Optional second enemy type spawned alongside the primary each wave (e.g. a slow, tanky variant). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Wave|Secondary")
	TSubclassOf<ATDEnemyBase> SecondaryEnemyClass;

	/** First wave (1-based) at which SecondaryEnemyClass starts appearing. Ignored if SecondaryEnemyClass is unset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Wave|Secondary", meta = (ClampMin = "1"))
	int32 SecondaryFirstWave = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Wave|Secondary", meta = (ClampMin = "0"))
	int32 SecondaryBaseEnemiesPerWave = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Wave|Secondary", meta = (ClampMin = "0"))
	int32 SecondaryEnemiesAddedPerWave = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Wave", meta = (ClampMin = "0.01", Units = "s"))
	float SpawnInterval = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Wave", meta = (ClampMin = "0.0", Units = "s"))
	float InitialSpawnDelay = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Wave", meta = (ClampMin = "0.0", Units = "s"))
	float TimeBetweenWaves = 5.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|Wave|Runtime")
	int32 CurrentWave = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|Wave|Runtime")
	int32 CurrentWaveEnemiesScheduled = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|Wave|Runtime")
	int32 EnemiesSpawned = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|Wave|Runtime")
	int32 EnemiesAlive = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|Wave|Runtime")
	int32 EnemiesKilled = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|Wave|Runtime")
	int32 EnemiesEscaped = 0;

	/** Scheduled enemies in the current wave that have not yet died or escaped. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|Wave|Runtime")
	int32 EnemiesRemaining = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|Wave|Runtime|Totals")
	int32 TotalEnemiesSpawned = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|Wave|Runtime|Totals")
	int32 TotalEnemiesKilled = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|Wave|Runtime|Totals")
	int32 TotalEnemiesEscaped = 0;

	UPROPERTY(BlueprintAssignable, Category = "Tower Defense|Wave")
	FTDWaveEventSignature OnWaveStarted;

	UPROPERTY(BlueprintAssignable, Category = "Tower Defense|Wave")
	FTDWaveEventSignature OnWaveCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Tower Defense|Wave")
	FTDEnemiesRemainingChangedSignature OnEnemiesRemainingChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

private:
	void StartNextWave();
	void SpawnNextEnemy();
	void ScheduleNextSpawn();
	void ResolveEnemy(ATDEnemyBase* Enemy, bool bWasKilled);
	void TryCompleteWave();
	void CompleteCurrentWave();

	UFUNCTION()
	void HandleEnemyKilled(ATDEnemyBase* Enemy);

	UFUNCTION()
	void HandleEnemyEscaped(ATDEnemyBase* Enemy);

	UFUNCTION()
	void HandleGameOver();

	UPROPERTY(Transient)
	TArray<TObjectPtr<ATDEnemyBase>> ActiveEnemies;

	UPROPERTY(Transient)
	TObjectPtr<ATDGameModeBase> TDGameMode;

	FTimerHandle SpawnTimerHandle;
	FTimerHandle NextWaveTimerHandle;
	FTimerHandle WaveResolutionTimerHandle;
	bool bCurrentWaveCompleted = false;
	bool bStopped = false;

	int32 CurrentWavePrimaryScheduled = 0;
	int32 CurrentWaveSecondaryScheduled = 0;
	int32 PrimarySpawned = 0;
	int32 SecondarySpawned = 0;
};
