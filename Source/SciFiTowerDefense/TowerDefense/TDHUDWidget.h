// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TDHUDWidget.generated.h"

class ATDGameModeBase;
class ATDPlayerState;
class ATDWaveSpawner;
class ATDPlayerController;
class ATDTowerBase;
class SButton;
class STextBlock;
class FReply;

/** Minimal event-driven tower-defense status HUD. */
UCLASS(Blueprintable)
class SCIFITOWERDEFENSE_API UTDHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|HUD|Runtime")
	int32 DisplayedCredits = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|HUD|Runtime")
	int32 DisplayedBaseHealth = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|HUD|Runtime")
	int32 DisplayedMaxBaseHealth = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|HUD|Runtime")
	int32 DisplayedWave = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|HUD|Runtime")
	int32 DisplayedTotalWaves = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|HUD|Runtime")
	int32 DisplayedEnemiesRemaining = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|HUD|Runtime")
	FString DisplayedTerminalText;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|HUD|Runtime")
	bool bTerminalVisible = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|HUD|Build")
	bool bStandardAffordable = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|HUD|Build")
	bool bRapidAffordable = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|HUD|Build")
	bool bStandardSelected = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tower Defense|HUD|Build")
	bool bRapidSelected = false;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void BindToGameplayState();
	void UnbindFromGameplayState();
	void RefreshAllValues();
	void RefreshSlateText();
	void RefreshBuildMenuState();
	int32 GetTowerCost(TSubclassOf<ATDTowerBase> TowerClass) const;
	FReply HandleStandardButtonClicked();
	FReply HandleRapidButtonClicked();

	UFUNCTION()
	void HandleCreditsChanged(int32 NewBalance);

	UFUNCTION()
	void HandleBaseHealthChanged(int32 CurrentHealth, int32 MaxHealth);

	UFUNCTION()
	void HandleWaveStarted(int32 WaveNumber);

	UFUNCTION()
	void HandleEnemiesRemainingChanged(int32 NewEnemiesRemaining);

	UFUNCTION()
	void HandleGameOver();

	UFUNCTION()
	void HandleVictory();

	UFUNCTION()
	void HandleTowerSelectionChanged(TSubclassOf<ATDTowerBase> NewTowerClass);

	UPROPERTY(Transient)
	TObjectPtr<ATDPlayerState> TDPlayerState;

	UPROPERTY(Transient)
	TObjectPtr<ATDGameModeBase> TDGameMode;

	UPROPERTY(Transient)
	TObjectPtr<ATDWaveSpawner> TDWaveSpawner;

	UPROPERTY(Transient)
	TObjectPtr<ATDPlayerController> TDPlayerController;

	TSharedPtr<STextBlock> CreditsTextBlock;
	TSharedPtr<STextBlock> BaseHealthTextBlock;
	TSharedPtr<STextBlock> WaveTextBlock;
	TSharedPtr<STextBlock> EnemiesTextBlock;
	TSharedPtr<STextBlock> TerminalTextBlock;
	TSharedPtr<SButton> StandardTowerButton;
	TSharedPtr<SButton> RapidTowerButton;
};
