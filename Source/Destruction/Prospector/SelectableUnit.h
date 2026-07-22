// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SelectableUnit.generated.h"

UINTERFACE(BlueprintType)
class DESTRUCTION_API USelectableUnit : public UInterface
{
	GENERATED_BODY()
};

// Reusable selection contract for any unit a PlayerController can select (Bill today, Ted and other
// workers later). Deliberately minimal - just enough for a single-owner selection cursor to toggle a
// visual state. No networking here; a future multiplayer-authoritative command system would extend
// this rather than replace it.
class DESTRUCTION_API ISelectableUnit
{
	GENERATED_BODY()

public:
	virtual void SetSelected(bool bSelected) = 0;
	virtual bool IsSelected() const = 0;
};
