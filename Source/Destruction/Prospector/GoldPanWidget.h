// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GoldPanWidget.generated.h"

// Thin C++ base for the gold-panning HUD. Carries no visuals of its own - subclass this as a
// Widget Blueprint and implement the BlueprintImplementableEvents below to show progress/results.
UCLASS()
class DESTRUCTION_API UGoldPanWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Panning UI")
	void HandleProgressChanged(float NormalizedJunkRemaining);

	UFUNCTION(BlueprintCallable, Category = "Panning UI")
	void HandleMinigameFinished(bool bSuccess, float GoldBankedGrams);

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Panning UI")
	void OnJunkProgressChanged(float NormalizedJunkRemaining);

	UFUNCTION(BlueprintImplementableEvent, Category = "Panning UI")
	void OnMinigameEnded(bool bSuccess, float GoldBankedGrams);
};
