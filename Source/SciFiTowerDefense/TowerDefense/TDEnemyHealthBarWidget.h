// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/SlateStructs.h"
#include "TDEnemyHealthBarWidget.generated.h"

/** Minimal native health bar displayed above enemies via a WidgetComponent. */
UCLASS()
class SCIFITOWERDEFENSE_API UTDEnemyHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	/** Updates the displayed fill fraction. Clamped to [0, 1]. */
	UFUNCTION(BlueprintCallable, Category = "Tower Defense|Health")
	void SetHealthPercent(float NewPercent);

private:
	FSlateColor GetFillColor() const;
	FOptionalSize GetFillWidth() const;

	static constexpr float BarWidth = 36.0f;
	static constexpr float BarHeight = 4.0f;
	static constexpr float BarPadding = 1.0f;

	float HealthPercent = 1.0f;
};
