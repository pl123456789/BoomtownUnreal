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

	// Which way the water runs. Established from evidence rather than from the station labels or from
	// the elevation profile, both of which are unreliable here - see RebuildFromApprovedCenterline for
	// the four independent lines that settled it. Regenerated on every rebuild: the authored water
	// surface is flat, so ordered point direction is the only thing carrying flow, and losing it would
	// silently invert placer-gold transport along the whole reach.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reach")
	EReachFlowDirection FlowDirection = EReachFlowDirection::TowardIncreasingIndex;

	// One record per centreline point, index-aligned with the spline. Geometry, downstream chainage and
	// water surface are authored; depth, bank classification, bend and deposition are still defaulted.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reach")
	TArray<FReachHydrologySample> Samples;

	// Rebuilds the spline, flow direction and sample array from the compiled-in approved data,
	// discarding any hand edits. Explicit rather than automatic so the authored data is never silently
	// regenerated underneath someone's work in the editor.
	//
	// Everything approved is regenerated here rather than hand-entered on the instance, so re-running
	// this reproduces identical values instead of erasing them. Anything typed into Samples by hand
	// will not survive - if a field needs to be authored per station, give it a source of truth here
	// first.
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Reach")
	void RebuildFromApprovedCenterline();

	// Total centreline length in cm, straight from the spline.
	UFUNCTION(BlueprintCallable, Category = "Reach")
	float GetReachLength() const;

	// The sample nearest a world-space location, or nullptr if the reach has no samples. The cheap
	// query the digging, panning, and deposition systems will want first.
	const FReachHydrologySample* FindNearestSample(const FVector& WorldLocation) const;
};
