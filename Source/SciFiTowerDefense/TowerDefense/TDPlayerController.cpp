// Copyright Epic Games, Inc. All Rights Reserved.

#include "TowerDefense/TDPlayerController.h"

#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SplineComponent.h"
#include "DrawDebugHelpers.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "TowerDefense/TDCameraPawn.h"
#include "TowerDefense/TDEnemyPath.h"
#include "TowerDefense/TDGameModeBase.h"
#include "TowerDefense/TDHUDWidget.h"
#include "TowerDefense/TDPlayerState.h"
#include "TowerDefense/TDTowerBase.h"
#include "TowerDefense/TDTowerPlacementPreview.h"

namespace
{
	constexpr float PlacementTraceDistance = 100000.0f;
	constexpr float PlacementProbeHeight = 50.0f;
}

ATDPlayerController::ATDPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	bEnableTouchEvents = false;
	bEnableTouchOverEvents = false;
	DefaultMouseCursor = EMouseCursor::Default;
	PlacementPreviewClass = ATDTowerPlacementPreview::StaticClass();
	HUDWidgetClass = UTDHUDWidget::StaticClass();
}

void ATDPlayerController::BeginPlay()
{
	Super::BeginPlay();

	ApplyCursorInputMode();
	ResolveEnemyPath();
	CreateGameplayHUD();

	if (IsLocalPlayerController() && CameraMappingContext)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(CameraMappingContext, 0);
			UE_LOG(LogTemp, Display, TEXT("TD input mapping active: %s"), *GetNameSafe(CameraMappingContext));
		}
	}
}

void ATDPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EnhancedInput)
	{
		UE_LOG(LogTemp, Error, TEXT("TD input setup failed: expected EnhancedInputComponent, got %s"), *GetNameSafe(InputComponent));
		return;
	}

	UE_LOG(LogTemp, Display, TEXT("TD input component active: %s"), *EnhancedInput->GetClass()->GetName());

	InputComponent->BindKey(EKeys::MouseScrollUp, IE_Pressed, this, &ATDPlayerController::HandleZoomIn);
	InputComponent->BindKey(EKeys::MouseScrollDown, IE_Pressed, this, &ATDPlayerController::HandleZoomOut);
	InputComponent->BindKey(EKeys::MiddleMouseButton, IE_Pressed, this, &ATDPlayerController::BeginDragPan);
	InputComponent->BindKey(EKeys::MiddleMouseButton, IE_Released, this, &ATDPlayerController::EndDragPan);
	InputComponent->BindKey(EKeys::One, IE_Pressed, this, &ATDPlayerController::EnterPlacementMode);
	InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &ATDPlayerController::EnterRapidPlacementMode);
	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ATDPlayerController::ConfirmPlacement);
	InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &ATDPlayerController::CancelPlacement);
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ATDPlayerController::CancelPlacement);
}

void ATDPlayerController::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (ATDCameraPawn* CameraPawn = GetTDCameraPawn())
	{
		FVector2D PanInput = FVector2D::ZeroVector;
		PanInput.X += IsInputKeyDown(EKeys::D) || IsInputKeyDown(EKeys::Right) ? 1.0f : 0.0f;
		PanInput.X -= IsInputKeyDown(EKeys::A) || IsInputKeyDown(EKeys::Left) ? 1.0f : 0.0f;
		PanInput.Y += IsInputKeyDown(EKeys::W) || IsInputKeyDown(EKeys::Up) ? 1.0f : 0.0f;
		PanInput.Y -= IsInputKeyDown(EKeys::S) || IsInputKeyDown(EKeys::Down) ? 1.0f : 0.0f;
		CameraPawn->MoveCamera(PanInput.GetSafeNormal());

		if (bDragPanActive)
		{
			float DeltaX = 0.0f;
			float DeltaY = 0.0f;
			GetInputMouseDelta(DeltaX, DeltaY);

			int32 ViewportX = 0;
			int32 ViewportY = 0;
			GetViewportSize(ViewportX, ViewportY);
			CameraPawn->PanByScreenDelta(FVector2D(DeltaX, DeltaY), static_cast<float>(ViewportX));
		}
	}

	if (bPlacementModeActive && IsPlacementDisabledByMatchState())
	{
		CancelPlacement();
	}
	else if (bPlacementModeActive)
	{
		UpdatePlacementPreview();
	}
}

void ATDPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	bDragPanActive = false;
}

void ATDPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(TDHUDWidget))
	{
		TDHUDWidget->RemoveFromParent();
	}
	TDHUDWidget = nullptr;

	if (IsValid(PlacementPreview))
	{
		PlacementPreview->Destroy();
	}
	PlacementPreview = nullptr;
	Super::EndPlay(EndPlayReason);
}

void ATDPlayerController::CreateGameplayHUD()
{
	if (!IsLocalPlayerController() || IsValid(TDHUDWidget) || !HUDWidgetClass)
	{
		return;
	}

	TDHUDWidget = CreateWidget<UTDHUDWidget>(this, HUDWidgetClass);
	if (IsValid(TDHUDWidget))
	{
		TDHUDWidget->AddToViewport(0);
		UE_LOG(LogTemp, Log, TEXT("TD HUD created: %s"), *GetNameSafe(TDHUDWidget));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("TD HUD creation failed for %s"), *GetNameSafe(HUDWidgetClass));
	}
}

void ATDPlayerController::EnterPlacementMode()
{
	SelectTowerForPlacement(TowerClassToPlace);
}

void ATDPlayerController::EnterRapidPlacementMode()
{
	SelectTowerForPlacement(RapidTowerClass);
}

void ATDPlayerController::SelectTowerForPlacement(const TSubclassOf<ATDTowerBase> TowerClass)
{
	if (IsPlacementDisabledByMatchState())
	{
		UE_LOG(LogTemp, Log, TEXT("TD Tower placement rejected: match has ended"));
		return;
	}

	if (!TowerClass || !PlacementPreviewClass || !GetWorld())
	{
		UE_LOG(LogTemp, Error, TEXT("TD Tower placement cannot start: TowerClassToPlace and PlacementPreviewClass are required"));
		return;
	}

	if (IsValid(PlacementPreview))
	{
		PlacementPreview->Destroy();
		PlacementPreview = nullptr;
	}

	SelectedTowerClass = TowerClass;
	bPlacementModeActive = true;
	bCurrentPlacementValid = false;
	CurrentPlacementStatus = TEXT("NoBuildPlatform");
	ApplyCursorInputMode();

	if (!IsValid(PlacementPreview))
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		PlacementPreview = GetWorld()->SpawnActor<ATDTowerPlacementPreview>(
			PlacementPreviewClass, FTransform::Identity, SpawnParams);
	}

	if (IsValid(PlacementPreview))
	{
		PlacementPreview->ConfigureForTower(SelectedTowerClass);
		PlacementPreview->SetActorHiddenInGame(true);
		UpdatePlacementPreview();
		OnTowerSelectionChanged.Broadcast(SelectedTowerClass);
		UE_LOG(LogTemp, Log, TEXT("TD Tower placement mode entered: %s"), *GetNameSafe(SelectedTowerClass));
	}
	else
	{
		bPlacementModeActive = false;
		SelectedTowerClass = nullptr;
		OnTowerSelectionChanged.Broadcast(nullptr);
	}
}

void ATDPlayerController::ConfirmPlacement()
{
	if (!bPlacementModeActive || !SelectedTowerClass || !GetWorld())
	{
		return;
	}
	if (IsPlacementDisabledByMatchState())
	{
		CancelPlacement();
		return;
	}

	UpdatePlacementPreview();
	if (!bCurrentPlacementValid)
	{
		if (CurrentPlacementStatus == TEXT("InsufficientCredits"))
		{
			UE_LOG(LogTemp, Log, TEXT("TD Tower placement rejected - insufficient credits"));
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("TD Tower placement rejected: %s"), *CurrentPlacementStatus);
		}
		return;
	}

	ATDPlayerState* Economy = GetTDEconomy();
	const int32 TowerCost = GetSelectedTowerCost();
	if (!IsValid(Economy) || !Economy->CanAfford(TowerCost))
	{
		bCurrentPlacementValid = false;
		CurrentPlacementStatus = TEXT("InsufficientCredits");
		if (IsValid(PlacementPreview))
		{
			PlacementPreview->SetPlacementValid(false);
		}
		UE_LOG(LogTemp, Log, TEXT("TD Tower placement rejected - insufficient credits"));
		return;
	}

	const FTransform SpawnTransform(FRotator::ZeroRotator, CurrentPlacementLocation);
	ATDTowerBase* Tower = GetWorld()->SpawnActorDeferred<ATDTowerBase>(
		SelectedTowerClass,
		SpawnTransform,
		this,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Tower)
	{
		UE_LOG(LogTemp, Error, TEXT("TD Tower placement failed to spawn %s"), *GetNameSafe(SelectedTowerClass));
		return;
	}

	Tower->Tags.AddUnique(TEXT("TD_PlayerPlaced"));
	UGameplayStatics::FinishSpawningActor(Tower, SpawnTransform);
	if (!Economy->SpendCredits(TowerCost))
	{
		Tower->Destroy();
		bCurrentPlacementValid = false;
		CurrentPlacementStatus = TEXT("InsufficientCredits");
		if (IsValid(PlacementPreview))
		{
			PlacementPreview->SetPlacementValid(false);
		}
		UE_LOG(LogTemp, Error, TEXT("TD Tower placement aborted because credits changed during spawn"));
		return;
	}
	LastPlacedTower = Tower;
	++PlacedTowerCount;
	UE_LOG(LogTemp, Log, TEXT("TD Tower placed. Cost: %d. Credits remaining: %d. Location: X=%.1f, Y=%.1f, Z=%.1f"),
		TowerCost, Economy->GetCredits(), CurrentPlacementLocation.X, CurrentPlacementLocation.Y, CurrentPlacementLocation.Z);

	UpdatePlacementPreview();
}

void ATDPlayerController::CancelPlacement()
{
	if (!bPlacementModeActive && !IsValid(PlacementPreview))
	{
		return;
	}

	bPlacementModeActive = false;
	bCurrentPlacementValid = false;
	CurrentPlacementStatus = TEXT("Inactive");
	CurrentDistanceToPath = -1.0f;
	SelectedTowerClass = nullptr;
	OnTowerSelectionChanged.Broadcast(nullptr);
	ApplyCursorInputMode();

	if (IsValid(PlacementPreview))
	{
		PlacementPreview->Destroy();
	}
	PlacementPreview = nullptr;
	UE_LOG(LogTemp, Log, TEXT("TD Tower placement cancelled"));
}

void ATDPlayerController::HandleZoomIn()
{
	if (ATDCameraPawn* CameraPawn = GetTDCameraPawn())
	{
		CameraPawn->ApplyZoomInput(1.0f);
	}
}

void ATDPlayerController::HandleZoomOut()
{
	if (ATDCameraPawn* CameraPawn = GetTDCameraPawn())
	{
		CameraPawn->ApplyZoomInput(-1.0f);
	}
}

void ATDPlayerController::BeginDragPan()
{
	bDragPanActive = true;
	ApplyDragInputMode();
}

void ATDPlayerController::EndDragPan()
{
	bDragPanActive = false;
	ApplyCursorInputMode();
}

void ATDPlayerController::ApplyCursorInputMode()
{
	bShowMouseCursor = true;

	if (UWorld* World = GetWorld())
	{
		if (UGameViewportClient* ViewportClient = World->GetGameViewport())
		{
			ViewportClient->SetMouseCaptureMode(EMouseCaptureMode::NoCapture);
			ViewportClient->SetMouseLockMode(EMouseLockMode::DoNotLock);
		}
	}

	FInputModeGameAndUI CursorInputMode;
	CursorInputMode.SetHideCursorDuringCapture(false);
	CursorInputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(CursorInputMode);
}

void ATDPlayerController::ApplyDragInputMode()
{
	bShowMouseCursor = false;
	if (UWorld* World = GetWorld())
	{
		if (UGameViewportClient* ViewportClient = World->GetGameViewport())
		{
			ViewportClient->SetMouseCaptureMode(EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown);
		}
	}
	SetInputMode(FInputModeGameOnly());
}

void ATDPlayerController::UpdatePlacementPreview()
{
	if (!IsValid(PlacementPreview))
	{
		return;
	}

	FHitResult PlatformHit;
	if (!TraceGameplayPlatform(PlatformHit))
	{
		bCurrentPlacementValid = false;
		CurrentPlacementStatus = TEXT("NoBuildPlatform");
		CurrentDistanceToPath = -1.0f;
		PlacementPreview->SetPlacementValid(false);
		PlacementPreview->SetActorHiddenInGame(true);
		return;
	}

	CurrentPlacementLocation = PlatformHit.ImpactPoint + FVector(0.0f, 0.0f, PlacementHeightOffset);
	FString PlacementStatus;
	bCurrentPlacementValid = EvaluatePlacement(PlatformHit, PlacementStatus);
	CurrentPlacementStatus = MoveTemp(PlacementStatus);
	PlacementPreview->SetActorLocationAndRotation(CurrentPlacementLocation, FRotator::ZeroRotator);
	PlacementPreview->SetPlacementValid(bCurrentPlacementValid);
	PlacementPreview->SetActorHiddenInGame(false);
	DrawPlacementVisualization();
}

bool ATDPlayerController::TraceGameplayPlatform(FHitResult& OutHit) const
{
	if (!GetWorld())
	{
		return false;
	}

	FVector WorldOrigin;
	FVector WorldDirection;
	if (!DeprojectMousePositionToWorld(WorldOrigin, WorldDirection))
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TDTowerPlacementTrace), true);
	if (IsValid(PlacementPreview))
	{
		QueryParams.AddIgnoredActor(PlacementPreview);
	}
	if (const APawn* ControlledPawn = GetPawn())
	{
		QueryParams.AddIgnoredActor(ControlledPawn);
	}

	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		OutHit,
		WorldOrigin,
		WorldOrigin + (WorldDirection * PlacementTraceDistance),
		PlacementTraceChannel,
		QueryParams);

	return bHit && IsValid(OutHit.GetActor()) && OutHit.GetActor()->ActorHasTag(GameplayPlatformTag);
}

bool ATDPlayerController::EvaluatePlacement(const FHitResult& PlatformHit, FString& OutStatus)
{
	AActor* PlatformActor = PlatformHit.GetActor();
	if (!IsValid(PlatformActor) || !PlatformActor->ActorHasTag(GameplayPlatformTag))
	{
		OutStatus = TEXT("NoBuildPlatform");
		return false;
	}

	const float CandidateRadius = GetSelectedTowerFootprintRadius();
	FVector BoundsOrigin;
	FVector BoundsExtent;
	PlatformActor->GetActorBounds(true, BoundsOrigin, BoundsExtent, true);
	if (FMath::Abs(CurrentPlacementLocation.X - BoundsOrigin.X) + CandidateRadius > BoundsExtent.X
		|| FMath::Abs(CurrentPlacementLocation.Y - BoundsOrigin.Y) + CandidateRadius > BoundsExtent.Y)
	{
		OutStatus = TEXT("PlatformEdge");
		return false;
	}

	ATDEnemyPath* EnemyPath = ResolveEnemyPath();
	if (!IsValid(EnemyPath) || !IsValid(EnemyPath->GetPathSpline()))
	{
		CurrentDistanceToPath = -1.0f;
		OutStatus = TEXT("NoEnemyPath");
		return false;
	}

	const FVector ClosestPathPoint = EnemyPath->GetPathSpline()->FindLocationClosestToWorldLocation(
		CurrentPlacementLocation, ESplineCoordinateSpace::World);
	CurrentDistanceToPath = FVector2D::Distance(
		FVector2D(CurrentPlacementLocation.X, CurrentPlacementLocation.Y),
		FVector2D(ClosestPathPoint.X, ClosestPathPoint.Y));
	if (CurrentDistanceToPath < PathExclusionRadius)
	{
		OutStatus = TEXT("EnemyPath");
		return false;
	}

	if (HasTowerOverlap(CurrentPlacementLocation, CandidateRadius))
	{
		OutStatus = TEXT("TowerOverlap");
		return false;
	}

	if (HasBlockingOverlap(CurrentPlacementLocation, CandidateRadius, PlatformActor))
	{
		OutStatus = TEXT("BlockingGeometry");
		return false;
	}

	ATDPlayerState* Economy = GetTDEconomy();
	if (!IsValid(Economy))
	{
		OutStatus = TEXT("NoPlayerState");
		return false;
	}
	if (!Economy->CanAfford(GetSelectedTowerCost()))
	{
		OutStatus = TEXT("InsufficientCredits");
		return false;
	}

	OutStatus = TEXT("Valid");
	return true;
}

bool ATDPlayerController::HasTowerOverlap(const FVector& Location, const float CandidateRadius) const
{
	if (!GetWorld())
	{
		return true;
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams ObjectQuery;
	ObjectQuery.AddObjectTypesToQuery(ECC_WorldDynamic);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TDTowerPlacementTowerOverlap), false);
	if (IsValid(PlacementPreview))
	{
		QueryParams.AddIgnoredActor(PlacementPreview);
	}

	GetWorld()->OverlapMultiByObjectType(
		Overlaps,
		Location + FVector(0.0f, 0.0f, PlacementProbeHeight),
		FQuat::Identity,
		ObjectQuery,
		FCollisionShape::MakeSphere(CandidateRadius),
		QueryParams);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		if (const ATDTowerBase* ExistingTower = Cast<ATDTowerBase>(Overlap.GetActor()))
		{
			const float RequiredSpacing = CandidateRadius + ExistingTower->TowerFootprintRadius;
			const float HorizontalDistance = FVector2D::Distance(
				FVector2D(Location.X, Location.Y),
				FVector2D(ExistingTower->GetActorLocation().X, ExistingTower->GetActorLocation().Y));
			if (HorizontalDistance < RequiredSpacing)
			{
				return true;
			}
		}
	}

	return false;
}

bool ATDPlayerController::HasBlockingOverlap(
	const FVector& Location,
	const float CandidateRadius,
	AActor* PlatformActor) const
{
	if (!GetWorld())
	{
		return true;
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams ObjectQuery;
	ObjectQuery.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQuery.AddObjectTypesToQuery(ECC_WorldDynamic);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TDTowerPlacementBlockingOverlap), false);
	QueryParams.AddIgnoredActor(PlatformActor);
	if (IsValid(PlacementPreview))
	{
		QueryParams.AddIgnoredActor(PlacementPreview);
	}
	if (const APawn* ControlledPawn = GetPawn())
	{
		QueryParams.AddIgnoredActor(ControlledPawn);
	}

	GetWorld()->OverlapMultiByObjectType(
		Overlaps,
		Location + FVector(0.0f, 0.0f, PlacementProbeHeight),
		FQuat::Identity,
		ObjectQuery,
		FCollisionShape::MakeSphere(CandidateRadius),
		QueryParams);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* OtherActor = Overlap.GetActor();
		if (!IsValid(OtherActor) || OtherActor == PlatformActor || OtherActor->IsA<ATDTowerBase>() || OtherActor->IsA<ATDEnemyPath>())
		{
			continue;
		}

		if (OtherActor->ActorHasTag(NonBuildableTag))
		{
			return true;
		}

		const UPrimitiveComponent* Primitive = Overlap.GetComponent();
		if (Primitive && Primitive->GetCollisionEnabled() != ECollisionEnabled::NoCollision
			&& Primitive->GetCollisionResponseToChannel(PlacementTraceChannel) == ECR_Block)
		{
			return true;
		}
	}

	return false;
}

float ATDPlayerController::GetSelectedTowerFootprintRadius() const
{
	if (SelectedTowerClass)
	{
		if (const ATDTowerBase* TowerDefaults = SelectedTowerClass->GetDefaultObject<ATDTowerBase>())
		{
			return FMath::Max(1.0f, TowerDefaults->TowerFootprintRadius);
		}
	}
	return 125.0f;
}

int32 ATDPlayerController::GetSelectedTowerCost() const
{
	if (SelectedTowerClass)
	{
		if (const ATDTowerBase* TowerDefaults = SelectedTowerClass->GetDefaultObject<ATDTowerBase>())
		{
			return FMath::Max(0, TowerDefaults->TowerCost);
		}
	}
	return 0;
}

ATDPlayerState* ATDPlayerController::GetTDEconomy() const
{
	return GetPlayerState<ATDPlayerState>();
}

bool ATDPlayerController::IsPlacementDisabledByMatchState() const
{
	const ATDGameModeBase* GameMode = Cast<ATDGameModeBase>(UGameplayStatics::GetGameMode(this));
	return IsValid(GameMode) && (GameMode->bGameOver || GameMode->bGameWon);
}

ATDEnemyPath* ATDPlayerController::ResolveEnemyPath()
{
	if (CachedEnemyPath.IsValid())
	{
		return CachedEnemyPath.Get();
	}

	for (TActorIterator<ATDEnemyPath> It(GetWorld()); It; ++It)
	{
		CachedEnemyPath = *It;
		return *It;
	}

	return nullptr;
}

ATDCameraPawn* ATDPlayerController::GetTDCameraPawn() const
{
	return Cast<ATDCameraPawn>(GetPawn());
}

void ATDPlayerController::DrawPlacementVisualization() const
{
	if (!bShowPlacementDebug || !GetWorld())
	{
		return;
	}

	const FColor PlacementColor = bCurrentPlacementValid ? FColor::Green : FColor::Red;
	DrawDebugCircle(
		GetWorld(),
		CurrentPlacementLocation + FVector(0.0f, 0.0f, 2.0f),
		GetSelectedTowerFootprintRadius(),
		48,
		PlacementColor,
		false,
		0.0f,
		0,
		3.0f,
		FVector::ForwardVector,
		FVector::RightVector,
		false);

	if (ATDEnemyPath* EnemyPath = CachedEnemyPath.Get())
	{
		const FVector ClosestPathPoint = EnemyPath->GetPathSpline()->FindLocationClosestToWorldLocation(
			CurrentPlacementLocation, ESplineCoordinateSpace::World);
		DrawDebugLine(GetWorld(), CurrentPlacementLocation, ClosestPathPoint, PlacementColor, false, 0.0f, 0, 2.0f);
	}
}
