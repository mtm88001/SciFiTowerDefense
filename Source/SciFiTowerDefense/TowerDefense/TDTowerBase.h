// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TDTowerBase.generated.h"

class ATDEnemyBase;
class ATDProjectileBase;
class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;

/** Stationary tower foundation with overlap-driven enemy targeting and a yawing turret. */
UCLASS(Blueprintable)
class SCIFITOWERDEFENSE_API ATDTowerBase : public AActor
{
	GENERATED_BODY()

public:
	ATDTowerBase();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	/** Radius used by the detection sphere to collect tower-defense enemies. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Targeting", meta = (ClampMin = "0.0", Units = "cm"))
	float DetectionRange = 1000.0f;

	/** Maximum horizontal turret turn rate in degrees per second. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Targeting", meta = (ClampMin = "0.0", Units = "deg/s"))
	float TurretRotationSpeed = 180.0f;

	/** Maximum vertical barrel turn rate in degrees per second. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Targeting", meta = (ClampMin = "0.0", Units = "deg/s"))
	float BarrelRotationSpeed = 120.0f;

	/** Fixed yaw between the turret housing and the barrel mount. Tracking compensates for this offset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Targeting", meta = (ClampMin = "-180.0", ClampMax = "180.0", Units = "deg"))
	float BarrelYawOffset = 0.0f;

	/** Lowest pitch angle the barrel can use while tracking a target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Targeting", meta = (ClampMin = "-89.0", ClampMax = "89.0", Units = "deg"))
	float MinimumBarrelPitch = -25.0f;

	/** Highest pitch angle the barrel can use while tracking a target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Targeting", meta = (ClampMin = "-89.0", ClampMax = "89.0", Units = "deg"))
	float MaximumBarrelPitch = 35.0f;

	/** Nearest valid tower-defense enemy currently inside the detection sphere. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|Targeting")
	TObjectPtr<ATDEnemyBase> CurrentTarget;

	/** Number of valid enemies currently considered by nearest-target selection. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|Targeting")
	int32 CandidateTargetCount = 0;

	/** Number of projectiles fired per second while a valid target is available. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Combat", meta = (ClampMin = "0.01", Units = "Hz"))
	float FireRate = 1.0f;

	/** Projectile spawned from MuzzlePoint when the tower fires. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Combat")
	TSubclassOf<ATDProjectileBase> ProjectileClass;

	/** Runtime shot count used to compare tower cadence during PIE verification. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|Combat")
	int32 ProjectilesFired = 0;

	/** Horizontal build footprint used to keep placed towers from overlapping. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Placement", meta = (ClampMin = "1.0", Units = "cm"))
	float TowerFootprintRadius = 125.0f;

	/** Purchase price supplied by this tower type to the placement economy. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Economy", meta = (ClampMin = "0"))
	int32 TowerCost = 100;

	/** Enables the prototype twin-barrel silhouette used by the Rapid Tower subclass. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Visual")
	bool bUseRapidPlaceholderShape = false;

	UFUNCTION(BlueprintPure, Category = "Tower Defense|Targeting")
	ATDEnemyBase* FindBestTarget();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BaseMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> TurretPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> TurretMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> GunPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> GunMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> SecondaryGunMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> MuzzlePoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> DetectionSphere;

	/** Editor-visible placement footprint. Hidden during play and never blocks gameplay. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> PlacementFootprint;

private:
	UFUNCTION()
	void HandleDetectionBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleDetectionEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex);

	void RefreshCurrentTarget();
	void RefreshPlaceholderAppearance();
	void ScanForNearbyTargets();
	void RotateWeaponTowardTarget(float DeltaSeconds);
	void TryFire();

	UPROPERTY(Transient)
	TArray<TObjectPtr<ATDEnemyBase>> CandidateTargets;

	FTimerHandle FireTimerHandle;
	FTimerHandle TargetScanTimerHandle;
};
