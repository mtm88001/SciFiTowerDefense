// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TDEnemyBase.generated.h"

class ATDEnemyPath;
class ATDEnemyBase;
class USkeletalMeshComponent;
class UStaticMeshComponent;
class UWidgetComponent;
class UTDEnemyHealthBarWidget;
class UUserWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDEnemyLifecycleSignature, ATDEnemyBase*, Enemy);

/** Reusable tower-defense enemy that moves deterministically along an assigned path spline. */
UCLASS(Blueprintable)
class SCIFITOWERDEFENSE_API ATDEnemyBase : public ACharacter
{
	GENERATED_BODY()

public:
	ATDEnemyBase();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	/** Reusable presentation mesh. Gameplay collision remains on the inherited capsule. */
	UFUNCTION(BlueprintPure, Category = "Tower Defense|Visual")
	USkeletalMeshComponent* GetEnemyVisual() const { return GetMesh(); }

	/** Applies reusable tower-defense damage. Zero and negative values are ignored. */
	UFUNCTION(BlueprintCallable, Category = "Tower Defense|Health")
	void ApplyDamage(float DamageAmount);

	UFUNCTION(BlueprintPure, Category = "Tower Defense|Health")
	bool IsAlive() const { return !bDead && CurrentHealth > 0.0f; }

	/** Authoritative route for this enemy. Assign this on placed instances when possible. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Tower Defense|Path", meta = (ExposeOnSpawn = "true"))
	TObjectPtr<ATDEnemyPath> EnemyPath;

	/** Travel speed along the path in Unreal units per second. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Movement", meta = (ClampMin = "0.0", Units = "cm/s"))
	float MovementSpeed = 300.0f;

	/** Vertical offset from the spline, keeping the capsule and visual above the platform. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Movement", meta = (ClampMin = "0.0", Units = "cm"))
	float PathHeightOffset = 50.0f;

	/** Current distance traveled along the assigned path. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|Movement")
	float DistanceAlongPath = 0.0f;

	/** Actual spline travel speed exposed for animation state and blend-space driving. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|Animation")
	float MovementSpeedForAnimation = 0.0f;

	/** True while the enemy is advancing along its spline. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|Animation")
	bool bIsMovingForAnimation = false;

	/** Mesh-only offset; changing this never changes the gameplay actor or capsule transform. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Visual")
	FVector EnemyVisualRelativeLocation = FVector(0.0f, 0.0f, -50.0f);

	/** Mesh-only orientation correction for imported characters. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Visual")
	FRotator EnemyVisualRelativeRotation = FRotator(0.0f, -90.0f, 0.0f);

	/** Mesh-only scale correction; the enemy actor remains at unit scale. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Visual", meta = (ClampMin = "0.01"))
	FVector EnemyVisualRelativeScale = FVector(0.6f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Health", meta = (ClampMin = "1.0"))
	float MaxHealth = 100.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|Health")
	float CurrentHealth = 100.0f;

	/** Height above the capsule origin where the health banner is drawn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Health", meta = (Units = "cm"))
	float HealthBarHeightOffset = 58.0f;

	/** On-screen pixel size of the health banner, sized to this enemy's on-screen footprint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Health")
	FVector2D HealthBarDrawSize = FVector2D(26.0f, 4.0f);

	/** Damage dealt to the defended base when this enemy reaches the path endpoint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Base", meta = (ClampMin = "0"))
	int32 EscapeDamage = 1;

	/** Credits awarded exactly once when this enemy dies from combat. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Defense|Economy", meta = (ClampMin = "0"))
	int32 KillReward = 25;

	/** Called once when the enemy reaches the path endpoint. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tower Defense|Path")
	void OnReachedPathEnd();

	/** Notification hook for future death visuals. The prototype destroys immediately afterward. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tower Defense|Health")
	void OnDeath();

	/** Broadcast once when this enemy is killed. */
	UPROPERTY(BlueprintAssignable, Category = "Tower Defense|Lifecycle")
	FTDEnemyLifecycleSignature OnEnemyKilled;

	/** Broadcast once when this enemy reaches the path endpoint. */
	UPROPERTY(BlueprintAssignable, Category = "Tower Defense|Lifecycle")
	FTDEnemyLifecycleSignature OnEnemyEscaped;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PlaceholderVisual;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> HealthBarComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tower Defense|Health")
	TSubclassOf<UUserWidget> HealthBarWidgetClass;

private:
	bool ResolveEnemyPath();
	void RefreshVisualState();
	void SetAnimationMovementState(float Speed);
	void UpdateTransformFromPath();
	void UpdateHealthBarDisplay();
	void Die();

	bool bReachedPathEnd = false;
	bool bDead = false;
	float CachedSplineLength = 0.0f;
	TWeakObjectPtr<UTDEnemyHealthBarWidget> CachedHealthBarWidget;
};
