// Copyright Epic Games, Inc. All Rights Reserved.

#include "TowerDefense/TDEnemyHealthBarWidget.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
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
				SNew(SBorder)
				.BorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.02f, 0.85f))
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
							SNew(SBorder)
							.BorderBackgroundColor(TAttribute<FSlateColor>::Create(
								TAttribute<FSlateColor>::FGetter::CreateUObject(this, &UTDEnemyHealthBarWidget::GetFillColor)))
						]
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

FSlateColor UTDEnemyHealthBarWidget::GetFillColor() const
{
	if (HealthPercent > 0.5f)
	{
		return FSlateColor(FMath::Lerp(FLinearColor(0.9f, 0.85f, 0.1f), FLinearColor(0.1f, 0.85f, 0.2f), (HealthPercent - 0.5f) * 2.0f));
	}
	return FSlateColor(FMath::Lerp(FLinearColor(0.85f, 0.1f, 0.1f), FLinearColor(0.9f, 0.85f, 0.1f), HealthPercent * 2.0f));
}

FOptionalSize UTDEnemyHealthBarWidget::GetFillWidth() const
{
	return FOptionalSize(FMath::Max(0.0f, (BarWidth - (BarPadding * 2.0f)) * HealthPercent));
}
