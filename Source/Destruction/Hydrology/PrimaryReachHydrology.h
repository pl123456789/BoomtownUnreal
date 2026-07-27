// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ReachHydrologyTypes.h"
#include "PrimaryReachHydrology.generated.h"

class USplineComponent;

// The permanent authoring record for the approved Primary tutorial reach - the ~1.887 km stretch of
// the Hope valley the player starts on.
//
// This actor is the gameplay authority for the river: where its centreline runs, how far downstream
// any point is, how wide the channel is, and (once established) which way the water actually flows.
// It deliberately draws nothing. Whatever ends up rendering visible water reads from this and can be
// swapped out later without touching anything that depends on the river as a gameplay object -
// the same separation FTerrainSurfaceQuery already gives us between gameplay and whichever terrain
// system happens to be providing collision.
//
// The centreline is authored data, not something rediscovered at runtime. Its points come from the
// approved M0014 corridor analysis and are compiled in below (see ApprovedCenterline in the .cpp)
// rather than sampled from terrain minima, so the reach cannot silently move if the Landscape is
// ever re-imported or locally resculpted.
UCLASS()
class DESTRUCTION_API APrimaryReachHydrology : public AActor
{
	GENERATED_BODY()

public:
	APrimaryReachHydrology();

	// The authored centreline. Points are placed in the approved order; index 0 is the upstream
	// endpoint as labelled by the M0014 analysis, which is not by itself a claim about flow
	// direction - see FlowDirection.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reach")
	TObjectPtr<USplineComponent> Centerline;

	// Which way the water runs. Left Undetermined on purpose: over this reach the source DEM's
	// vertical noise is larger than the reach's true fall, so the elevation profile is not a
	// reliable guide, and the approved endpoint elevations imply a net rise from the labelled
	// upstream end to the labelled downstream end. Nothing should consume a guessed direction, so
	// this stays unset until it is established deliberately.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reach")
	EReachFlowDirection FlowDirection = EReachFlowDirection::Undetermined;

	// One record per centreline point, index-aligned with the spline. Mostly defaulted for now:
	// this pass fixes the geometry and the shape of the record, not its hydrology contents.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reach")
	TArray<FReachHydrologySample> Samples;

	// Rebuilds the spline and the sample array from the compiled-in approved centreline, discarding
	// any hand edits. Explicit rather than automatic so the authored data is never silently
	// regenerated underneath someone's work in the editor.
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Reach")
	void RebuildFromApprovedCenterline();

	// Total centreline length in cm, straight from the spline.
	UFUNCTION(BlueprintCallable, Category = "Reach")
	float GetReachLength() const;

	// The sample nearest a world-space location, or nullptr if the reach has no samples. The cheap
	// query the digging, panning, and deposition systems will want first.
	const FReachHydrologySample* FindNearestSample(const FVector& WorldLocation) const;
};
