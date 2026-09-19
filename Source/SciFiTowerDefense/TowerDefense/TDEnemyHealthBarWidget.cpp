// Copyright Epic Games, Inc. All Rights Reserved.

#include "TowerDefense/TDEnemyHealthBarWidget.h"

#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"

TSharedRef<SWidget> UTDEnemyHealthBarWidget::RebuildWidget()
{
	return SNew(SBox)
		.WidthOverride(BarWidth)
		.HeightOverride(BarHeight)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				// Solid dark backing plate. SColorBlock paints a flat rect with no
				// 9-slice brush, unlike SBorder, which looks hollow at this size.
				SNew(SColorBlock)
				.Color(FLinearColor(0.02f, 0.02f, 0.02f, 0.9f))
			]
			+ SOverlay::Slot()
			.Padding(FMargin(BarPadding))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Fill)
				.AutoWidth()
				[
					SNew(SBox)
					.WidthOverride(TAttribute<FOptionalSize>::Create(
						TAttribute<FOptionalSize>::FGetter::CreateUObject(this, &UTDEnemyHealthBarWidget::GetFillWidth)))
					.HeightOverride(BarHeight - (BarPadding * 2.0f))
					[
						SNew(SColorBlock)
						.Color(TAttribute<FLinearColor>::Create(
							TAttribute<FLinearColor>::FGetter::CreateUObject(this, &UTDEnemyHealthBarWidget::GetFillColor)))
					]
				]
			]
		];
}

void UTDEnemyHealthBarWidget::SetHealthPercent(const float NewPercent)
{
	HealthPercent = FMath::Clamp(NewPercent, 0.0f, 1.0f);
	Invalidate(EInvalidateWidgetReason::Paint);
}

FLinearColor UTDEnemyHealthBarWidget::GetFillColor() const
{
	if (HealthPercent > 0.5f)
	{
		return FMath::Lerp(FLinearColor(0.9f, 0.85f, 0.1f), FLinearColor(0.1f, 0.85f, 0.2f), (HealthPercent - 0.5f) * 2.0f);
	}
	return FMath::Lerp(FLinearColor(0.85f, 0.1f, 0.1f), FLinearColor(0.9f, 0.85f, 0.1f), HealthPercent * 2.0f);
}

FOptionalSize UTDEnemyHealthBarWidget::GetFillWidth() const
{
	return FOptionalSize(FMath::Max(0.0f, (BarWidth - (BarPadding * 2.0f)) * HealthPercent));
}
