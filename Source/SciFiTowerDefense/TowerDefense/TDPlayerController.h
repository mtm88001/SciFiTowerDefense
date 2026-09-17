// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/PlayerController.h"
#include "TDPlayerController.generated.h"

class AActor;
class ATDCameraPawn;
class ATDEnemyPath;
class ATDPlayerState;
class ATDTowerBase;
class ATDTowerPlacementPreview;
class UInputAction;
class UInputMappingContext;
class UTDHUDWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDTowerSelectionChangedSignature, TSubclassOf<ATDTowerBase>, NewTowerClass);

/** Tower-defense camera input and reusable free-form tower placement. */
UCLASS(Blueprintable)
class SCIFITOWERDEFENSE_API ATDPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ATDPlayerController();

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnPossess(APawn* InPawn) override;

	UFUNCTION(BlueprintCallable, Category = "Tower Defense|Placement")
	void EnterPlacementMode();

	UFUNCTION(BlueprintCallable, Category = "Tower Defense|Placement")
	void EnterRapidPlacementMode();

	/** Generic tower selection entry point shared by UI and keyboard shortcuts. */
	UFUNCTION(BlueprintCallable, Category = "Tower Defense|Placement")
	void SelectTowerForPlacement(TSubclassOf<ATDTowerBase> TowerClass);

	UFUNCTION(BlueprintCallable, Category = "Tower Defense|Placement")
	void ConfirmPlacement();

	UFUNCTION(BlueprintCallable, Category = "Tower Defense|Placement")
	void CancelPlacement();

	/** Default tower selected by the prototype's 1-key shortcut. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Tower Defense|Placement")
	TSubclassOf<ATDTowerBase> TowerClassToPlace;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Tower Defense|Placement")
	TSubclassOf<ATDTowerBase> RapidTowerClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Tower Defense|Placement")
	TSubclassOf<ATDTowerPlacementPreview> PlacementPreviewClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Tower Defense|Placement", meta = (ClampMin = "0.0", Units = "cm"))
	float PathExclusionRadius = 300.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Tower Defense|Placement")
	FName GameplayPlatformTag = TEXT("TD_BuildPlatform");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Tower Defense|Placement")
	FName NonBuildableTag = TEXT("TD_NoBuild");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Tower Defense|Placement")
	TEnumAsByte<ECollisionChannel> PlacementTraceChannel = ECC_Visibility;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Tower Defense|Placement", meta = (Units = "cm"))
	float PlacementHeightOffset = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Tower Defense|Placement|Debug")
	bool bShowPlacementDebug = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|Placement|Runtime")
	bool bPlacementModeActive = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|Placement|Runtime")
	bool bCurrentPlacementValid = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|Placement|Runtime")
	FVector CurrentPlacementLocation = FVector::ZeroVector;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|Placement|Runtime")
	float CurrentDistanceToPath = -1.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|Placement|Runtime")
	FString CurrentPlacementStatus = TEXT("Inactive");

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|Placement|Runtime")
	int32 PlacedTowerCount = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|Placement|Runtime")
	TSubclassOf<ATDTowerBase> SelectedTowerClass;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|Placement|Runtime")
	TObjectPtr<ATDTowerPlacementPreview> PlacementPreview;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|Placement|Runtime")
	TObjectPtr<ATDTowerBase> LastPlacedTower;

	UPROPERTY(BlueprintAssignable, Category = "Tower Defense|Placement")
	FTDTowerSelectionChangedSignature OnTowerSelectionChanged;

	/** Minimal gameplay HUD class. Assigned to WBP_TDHUD in BP_TDPlayerController. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tower Defense|HUD")
	TSubclassOf<UTDHUDWidget> HUDWidgetClass;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|HUD")
	TObjectPtr<UTDHUDWidget> TDHUDWidget;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tower Defense|Input")
	TObjectPtr<UInputMappingContext> CameraMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tower Defense|Input")
	TObjectPtr<UInputAction> PanAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tower Defense|Input")
	TObjectPtr<UInputAction> ZoomAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tower Defense|Input")
	TObjectPtr<UInputAction> DragAction;

private:
	void CreateGameplayHUD();
	void ApplyCursorInputMode();
	void ApplyDragInputMode();
	void HandleZoomIn();
	void HandleZoomOut();
	void BeginDragPan();
	void EndDragPan();
	void UpdatePlacementPreview();
	bool TraceGameplayPlatform(FHitResult& OutHit) const;
	bool EvaluatePlacement(const FHitResult& PlatformHit, FString& OutStatus);
	bool HasTowerOverlap(const FVector& Location, float CandidateRadius) const;
	bool HasBlockingOverlap(const FVector& Location, float CandidateRadius, AActor* PlatformActor) const;
	float GetSelectedTowerFootprintRadius() const;
	int32 GetSelectedTowerCost() const;
	ATDPlayerState* GetTDEconomy() const;
	bool IsPlacementDisabledByMatchState() const;
	ATDEnemyPath* ResolveEnemyPath();
	ATDCameraPawn* GetTDCameraPawn() const;
	void DrawPlacementVisualization() const;

	TWeakObjectPtr<ATDEnemyPath> CachedEnemyPath;
	bool bDragPanActive = false;
};
