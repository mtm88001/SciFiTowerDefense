// Copyright Epic Games, Inc. All Rights Reserved.

#include "TowerDefense/TDHUDWidget.h"

#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Styling/CoreStyle.h"
#include "TowerDefense/TDGameModeBase.h"
#include "TowerDefense/TDPlayerState.h"
#include "TowerDefense/TDPlayerController.h"
#include "TowerDefense/TDTowerBase.h"
#include "TowerDefense/TDWaveSpawner.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

TSharedRef<SWidget> UTDHUDWidget::RebuildWidget()
{
	const FTextBlockStyle& StatusStyle = FCoreStyle::Get().GetWidgetStyle<FTextBlockStyle>("NormalText");

	return SNew(SOverlay)
		.Visibility(EVisibility::SelfHitTestInvisible)
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Top)
		.Padding(FMargin(18.0f))
		[
			SNew(SBorder)
			.Visibility(EVisibility::HitTestInvisible)
			.BorderBackgroundColor(FLinearColor(0.015f, 0.025f, 0.045f, 0.82f))
			.Padding(FMargin(14.0f, 8.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(12.0f, 3.0f)
				[
					SAssignNew(CreditsTextBlock, STextBlock).TextStyle(&StatusStyle).Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(12.0f, 3.0f)
				[
					SAssignNew(BaseHealthTextBlock, STextBlock).TextStyle(&StatusStyle).Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(12.0f, 3.0f)
				[
					SAssignNew(WaveTextBlock, STextBlock).TextStyle(&StatusStyle).Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(12.0f, 3.0f)
				[
					SAssignNew(EnemiesTextBlock, STextBlock).TextStyle(&StatusStyle).Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
				]
			]
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.Visibility_Lambda([this]() { return bTerminalVisible ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			.BorderBackgroundColor(FLinearColor(0.01f, 0.01f, 0.015f, 0.88f))
			.Padding(FMargin(42.0f, 24.0f))
			[
				SAssignNew(TerminalTextBlock, STextBlock)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 56))
				.ColorAndOpacity(FLinearColor(0.95f, 0.88f, 0.25f, 1.0f))
			]
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(18.0f))
		[
			SNew(SBorder)
			.Visibility(EVisibility::SelfHitTestInvisible)
			.BorderBackgroundColor(FLinearColor(0.015f, 0.025f, 0.045f, 0.82f))
			.Padding(FMargin(10.0f, 7.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(6.0f)
				[
					SAssignNew(StandardTowerButton, SButton)
					.IsEnabled_Lambda([this]() { return bStandardAffordable; })
					.ButtonColorAndOpacity_Lambda([this]()
					{
						return FSlateColor(bStandardSelected
							? FLinearColor(0.12f, 0.5f, 0.9f, 1.0f)
							: FLinearColor(0.22f, 0.25f, 0.3f, 1.0f));
					})
					.OnClicked(FOnClicked::CreateUObject(this, &UTDHUDWidget::HandleStandardButtonClicked))
					[
						SNew(STextBlock)
						.Justification(ETextJustify::Center)
						.Text_Lambda([this]()
						{
							return FText::FromString(FString::Printf(TEXT("Standard Tower\n$%d"),
								IsValid(TDPlayerController) ? GetTowerCost(TDPlayerController->TowerClassToPlace) : 100));
						})
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(6.0f)
				[
					SAssignNew(RapidTowerButton, SButton)
					.IsEnabled_Lambda([this]() { return bRapidAffordable; })
					.ButtonColorAndOpacity_Lambda([this]()
					{
						return FSlateColor(bRapidSelected
							? FLinearColor(0.75f, 0.2f, 0.12f, 1.0f)
							: FLinearColor(0.22f, 0.25f, 0.3f, 1.0f));
					})
					.OnClicked(FOnClicked::CreateUObject(this, &UTDHUDWidget::HandleRapidButtonClicked))
					[
						SNew(STextBlock)
						.Justification(ETextJustify::Center)
						.Text_Lambda([this]()
						{
							return FText::FromString(FString::Printf(TEXT("Rapid Tower\n$%d"),
								IsValid(TDPlayerController) ? GetTowerCost(TDPlayerController->RapidTowerClass) : 175));
						})
					]
				]
			]
		];
}

void UTDHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	BindToGameplayState();
	RefreshAllValues();
}

void UTDHUDWidget::NativeDestruct()
{
	UnbindFromGameplayState();
	Super::NativeDestruct();
}

void UTDHUDWidget::BindToGameplayState()
{
	APlayerController* Owner = GetOwningPlayer();
	TDPlayerController = Cast<ATDPlayerController>(Owner);
	TDPlayerState = Owner ? Owner->GetPlayerState<ATDPlayerState>() : nullptr;
	TDGameMode = Cast<ATDGameModeBase>(UGameplayStatics::GetGameMode(this));

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ATDWaveSpawner> It(World); It; ++It)
		{
			TDWaveSpawner = *It;
			break;
		}
	}

	if (IsValid(TDPlayerState))
	{
		TDPlayerState->OnCreditsChanged.AddUniqueDynamic(this, &UTDHUDWidget::HandleCreditsChanged);
	}
	if (IsValid(TDGameMode))
	{
		TDGameMode->OnBaseHealthChanged.AddUniqueDynamic(this, &UTDHUDWidget::HandleBaseHealthChanged);
		TDGameMode->OnGameOver.AddUniqueDynamic(this, &UTDHUDWidget::HandleGameOver);
		TDGameMode->OnGameWon.AddUniqueDynamic(this, &UTDHUDWidget::HandleVictory);
	}
	if (IsValid(TDWaveSpawner))
	{
		TDWaveSpawner->OnWaveStarted.AddUniqueDynamic(this, &UTDHUDWidget::HandleWaveStarted);
		TDWaveSpawner->OnEnemiesRemainingChanged.AddUniqueDynamic(this, &UTDHUDWidget::HandleEnemiesRemainingChanged);
	}
	if (IsValid(TDPlayerController))
	{
		TDPlayerController->OnTowerSelectionChanged.AddUniqueDynamic(this, &UTDHUDWidget::HandleTowerSelectionChanged);
	}
}

void UTDHUDWidget::UnbindFromGameplayState()
{
	if (IsValid(TDPlayerState))
	{
		TDPlayerState->OnCreditsChanged.RemoveDynamic(this, &UTDHUDWidget::HandleCreditsChanged);
	}
	if (IsValid(TDGameMode))
	{
		TDGameMode->OnBaseHealthChanged.RemoveDynamic(this, &UTDHUDWidget::HandleBaseHealthChanged);
		TDGameMode->OnGameOver.RemoveDynamic(this, &UTDHUDWidget::HandleGameOver);
		TDGameMode->OnGameWon.RemoveDynamic(this, &UTDHUDWidget::HandleVictory);
	}
	if (IsValid(TDWaveSpawner))
	{
		TDWaveSpawner->OnWaveStarted.RemoveDynamic(this, &UTDHUDWidget::HandleWaveStarted);
		TDWaveSpawner->OnEnemiesRemainingChanged.RemoveDynamic(this, &UTDHUDWidget::HandleEnemiesRemainingChanged);
	}
	if (IsValid(TDPlayerController))
	{
		TDPlayerController->OnTowerSelectionChanged.RemoveDynamic(this, &UTDHUDWidget::HandleTowerSelectionChanged);
	}
}

void UTDHUDWidget::RefreshAllValues()
{
	DisplayedCredits = IsValid(TDPlayerState) ? TDPlayerState->GetCredits() : 0;
	if (IsValid(TDGameMode))
	{
		DisplayedBaseHealth = TDGameMode->CurrentBaseHealth;
		DisplayedMaxBaseHealth = TDGameMode->MaxBaseHealth;
		if (TDGameMode->bGameOver)
		{
			HandleGameOver();
		}
		else if (TDGameMode->bGameWon)
		{
			HandleVictory();
		}
	}
	if (IsValid(TDWaveSpawner))
	{
		DisplayedWave = TDWaveSpawner->CurrentWave;
		DisplayedTotalWaves = TDWaveSpawner->TotalWaves;
		DisplayedEnemiesRemaining = TDWaveSpawner->EnemiesRemaining;
	}
	RefreshSlateText();
	RefreshBuildMenuState();
}

void UTDHUDWidget::RefreshSlateText()
{
	if (CreditsTextBlock.IsValid())
	{
		CreditsTextBlock->SetText(FText::FromString(FString::Printf(TEXT("Credits: %d"), DisplayedCredits)));
	}
	if (BaseHealthTextBlock.IsValid())
	{
		BaseHealthTextBlock->SetText(FText::FromString(FString::Printf(TEXT("Base: %d/%d"), DisplayedBaseHealth, DisplayedMaxBaseHealth)));
	}
	if (WaveTextBlock.IsValid())
	{
		WaveTextBlock->SetText(FText::FromString(FString::Printf(TEXT("Wave: %d/%d"), DisplayedWave, DisplayedTotalWaves)));
	}
	if (EnemiesTextBlock.IsValid())
	{
		EnemiesTextBlock->SetText(FText::FromString(FString::Printf(TEXT("Enemies: %d"), DisplayedEnemiesRemaining)));
	}
	if (TerminalTextBlock.IsValid())
	{
		TerminalTextBlock->SetText(FText::FromString(DisplayedTerminalText));
	}
}

void UTDHUDWidget::HandleCreditsChanged(const int32 NewBalance)
{
	DisplayedCredits = NewBalance;
	RefreshBuildMenuState();
	RefreshSlateText();
}

void UTDHUDWidget::HandleBaseHealthChanged(const int32 CurrentHealth, const int32 MaxHealth)
{
	DisplayedBaseHealth = CurrentHealth;
	DisplayedMaxBaseHealth = MaxHealth;
	RefreshSlateText();
}

void UTDHUDWidget::HandleWaveStarted(const int32 WaveNumber)
{
	DisplayedWave = WaveNumber;
	DisplayedTotalWaves = IsValid(TDWaveSpawner) ? TDWaveSpawner->TotalWaves : DisplayedTotalWaves;
	DisplayedEnemiesRemaining = IsValid(TDWaveSpawner) ? TDWaveSpawner->EnemiesRemaining : DisplayedEnemiesRemaining;
	RefreshSlateText();
}

void UTDHUDWidget::HandleEnemiesRemainingChanged(const int32 NewEnemiesRemaining)
{
	DisplayedEnemiesRemaining = NewEnemiesRemaining;
	RefreshSlateText();
}

void UTDHUDWidget::HandleGameOver()
{
	bTerminalVisible = true;
	DisplayedTerminalText = TEXT("GAME OVER");
	RefreshBuildMenuState();
	RefreshSlateText();
}

void UTDHUDWidget::HandleVictory()
{
	bTerminalVisible = true;
	DisplayedTerminalText = TEXT("VICTORY");
	RefreshBuildMenuState();
	RefreshSlateText();
}

void UTDHUDWidget::HandleTowerSelectionChanged(const TSubclassOf<ATDTowerBase> NewTowerClass)
{
	RefreshBuildMenuState();
}

void UTDHUDWidget::RefreshBuildMenuState()
{
	const bool bMatchEnded = IsValid(TDGameMode) && (TDGameMode->bGameOver || TDGameMode->bGameWon);
	const TSubclassOf<ATDTowerBase> StandardClass = IsValid(TDPlayerController) ? TDPlayerController->TowerClassToPlace : nullptr;
	const TSubclassOf<ATDTowerBase> RapidClass = IsValid(TDPlayerController) ? TDPlayerController->RapidTowerClass : nullptr;
	bStandardAffordable = !bMatchEnded && StandardClass && DisplayedCredits >= GetTowerCost(StandardClass);
	bRapidAffordable = !bMatchEnded && RapidClass && DisplayedCredits >= GetTowerCost(RapidClass);
	bStandardSelected = IsValid(TDPlayerController) && TDPlayerController->bPlacementModeActive
		&& TDPlayerController->SelectedTowerClass == StandardClass;
	bRapidSelected = IsValid(TDPlayerController) && TDPlayerController->bPlacementModeActive
		&& TDPlayerController->SelectedTowerClass == RapidClass;
	Invalidate(EInvalidateWidgetReason::Paint);
}

int32 UTDHUDWidget::GetTowerCost(const TSubclassOf<ATDTowerBase> TowerClass) const
{
	const ATDTowerBase* Defaults = TowerClass ? TowerClass->GetDefaultObject<ATDTowerBase>() : nullptr;
	return Defaults ? FMath::Max(0, Defaults->TowerCost) : 0;
}

FReply UTDHUDWidget::HandleStandardButtonClicked()
{
	if (bStandardAffordable && IsValid(TDPlayerController))
	{
		TDPlayerController->EnterPlacementMode();
	}
	return FReply::Handled();
}

FReply UTDHUDWidget::HandleRapidButtonClicked()
{
	if (bRapidAffordable && IsValid(TDPlayerController))
	{
		TDPlayerController->EnterRapidPlacementMode();
	}
	return FReply::Handled();
}
