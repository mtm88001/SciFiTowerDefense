// Copyright Epic Games, Inc. All Rights Reserved.

#include "TowerDefense/TDPlayerState.h"

ATDPlayerState::ATDPlayerState()
{
	StartingCredits = 500;
	CurrentCredits = StartingCredits;
}

void ATDPlayerState::BeginPlay()
{
	Super::BeginPlay();
	StartingCredits = FMath::Max(0, StartingCredits);
	CurrentCredits = StartingCredits;
	UE_LOG(LogTemp, Log, TEXT("TD Credits initialized: %d"), CurrentCredits);
	OnCreditsChanged.Broadcast(CurrentCredits);
}

bool ATDPlayerState::CanAfford(const int32 Cost) const
{
	return Cost >= 0 && CurrentCredits >= Cost;
}

bool ATDPlayerState::SpendCredits(const int32 Amount)
{
	if (Amount < 0 || !CanAfford(Amount))
	{
		return false;
	}

	if (Amount == 0)
	{
		return true;
	}

	CurrentCredits -= Amount;
	OnCreditsChanged.Broadcast(CurrentCredits);
	return true;
}

void ATDPlayerState::AddCredits(const int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	const int64 NewBalance = static_cast<int64>(CurrentCredits) + static_cast<int64>(Amount);
	CurrentCredits = static_cast<int32>(FMath::Min<int64>(NewBalance, MAX_int32));
	OnCreditsChanged.Broadcast(CurrentCredits);
}
