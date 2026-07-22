// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Voxel/GeoTypes.h"
#include "PanningMinigameComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMinigameFinished, bool, bSuccess, float, GoldBankedGrams, FSedimentPacket, LostToTailings);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPanProgressChanged, float, NormalizedJunkRemaining);

// Active gold-panning mini-game: the player continuously tilts the pan (SetTiltInput) to wash junk
// mass out while a randomly drifting "flow direction" rewards keeping tilt aligned with it. Gold is
// tracked by real grain size - fine gold washes out fastest when overtilted, nuggets hang on longest -
// so a careful player keeps more of the fine gold than a beginner who overtilts.
UCLASS(ClassGroup = (Prospector), meta = (BlueprintSpawnableComponent))
class DESTRUCTION_API UPanningMinigameComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPanningMinigameComponent();

	UFUNCTION(BlueprintCallable, Category = "Panning")
	void StartMinigame(const FSedimentPacket& Bucket);

	UFUNCTION(BlueprintCallable, Category = "Panning")
	void SetTiltInput(FVector2D Tilt);

	// Ends the minigame immediately, banking nothing (everything still in the pan is lost to tailings).
	UFUNCTION(BlueprintCallable, Category = "Panning")
	void CancelMinigame();

	UFUNCTION(BlueprintPure, Category = "Panning")
	bool IsPanningActive() const { return bIsActive; }

	UPROPERTY(BlueprintAssignable, Category = "Panning")
	FOnMinigameFinished OnMinigameFinished;

	UPROPERTY(BlueprintAssignable, Category = "Panning")
	FOnPanProgressChanged OnProgressChanged;

	UPROPERTY(EditAnywhere, Category = "Panning|Tuning")
	float Duration = 8.0f;

	// Kilograms of junk washed out per second at zero tilt-flow alignment.
	UPROPERTY(EditAnywhere, Category = "Panning|Tuning")
	float BaseWashRate = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Panning|Tuning")
	float TiltWashMultiplier = 2.5f;

	// Tilt magnitude (0-1) above which gold starts washing out along with the junk.
	UPROPERTY(EditAnywhere, Category = "Panning|Tuning")
	float OvertiltThreshold = 0.75f;

	// Fraction of remaining gold lost per second while overtilted, per grain size - fine gold washes
	// out fastest, nuggets are dense/large enough to hang on the longest.
	UPROPERTY(EditAnywhere, Category = "Panning|Tuning")
	float FineGoldLossRatePerSecond = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Panning|Tuning")
	float FlakeGoldLossRatePerSecond = 0.2f;

	UPROPERTY(EditAnywhere, Category = "Panning|Tuning")
	float NuggetGoldLossRatePerSecond = 0.03f;

	UPROPERTY(EditAnywhere, Category = "Panning|Tuning")
	float FlowDirectionChangeSpeed = 0.15f;

	// What's still in the pan right now.
	UPROPERTY(BlueprintReadOnly, Category = "Panning")
	FSedimentPacket Remaining;

	UPROPERTY(BlueprintReadOnly, Category = "Panning")
	float TimeRemaining = 0.0f;

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	bool bIsActive = false;
	float StartingJunkKg = 0.0f;
	FSedimentPacket LostSoFar;
	FVector2D CurrentTilt = FVector2D::ZeroVector;
	float FlowNoiseTime = 0.0f;

	void FinishMinigame(bool bSuccess);
	FVector2D GetFlowDirection() const;
};
