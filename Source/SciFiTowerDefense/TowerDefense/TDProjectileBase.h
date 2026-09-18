// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TDProjectileBase.generated.h"

class ATDEnemyBase;
class UProjectileMovementComponent;
class USphereComponent;
class UStaticMeshComponent;

/** Reusable tower projectile that can travel straight or home on its target. */
UCLASS(Blueprintable)
class SCIFITOWERDEFENSE_API ATDProjectileBase : public AActor
{
	GENERATED_BODY()

public:
	ATDProjectileBase();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Sets the intended enemy before deferred spawning is completed.
	 *  InLaunchDirection, when non-zero, is used for initial velocity/rotation
	 *  instead of the actor's spawn-time forward vector, which is not
	 *  reliably set yet when this runs during a deferred spawn. */
	UFUNCTION(BlueprintCallable, Category = "Tower Defense|Projectile")
	void InitializeProjectile(ATDEnemyBase* NewTarget, FVector InLaunchDirection = FVector::ZeroVector);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Projectile", meta = (ClampMin = "1.0", Units = "cm/s"))
	float ProjectileSpeed = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Projectile", meta = (ClampMin = "0.0"))
	float Damage = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Projectile", meta = (ClampMin = "0.1", Units = "s"))
	float MaxLifetime = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Projectile", meta = (ClampMin = "0.0", Units = "cm/s^2"))
	float HomingAcceleration = 10000.0f;

	/** When enabled, the projectile steers toward the assigned target. Disable for direct line-of-sight fire. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Projectile")
	bool bEnableHoming = true;

	/** Uses a small elongated cylinder for a bullet-like visual. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Projectile")
	bool bUseCylinderVisual = false;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ProjectileVisual;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

private:
	UFUNCTION()
	void HandleProjectileOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleTargetDestroyed(AActor* DestroyedActor);

	UPROPERTY(Transient)
	TObjectPtr<ATDEnemyBase> TargetEnemy;

	FVector LaunchDirection = FVector::ZeroVector;

	bool bHasHit = false;
};
