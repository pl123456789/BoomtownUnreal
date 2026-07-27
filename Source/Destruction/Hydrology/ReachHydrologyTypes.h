// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ReachHydrologyTypes.generated.h"

// The interpreted environment on one side of the channel. Carried over from the BoomtownUnity
// RiverLandscapeType classification, which survives the move to an authored world unchanged - it
// was never tied to the discarded generation machinery. This is not a texture, material, foliage
// rule, or gold deposit; it is the shared classification those systems read later, and it is what
// lets a player learn to read a bank by eye and predict whether it is worth working.
UENUM(BlueprintType)
enum class EReachBankType : uint8
{
	// Not yet authored. Explicit so a defaulted sample is never mistaken for a real call.
	Unclassified	UMETA(DisplayName = "Unclassified"),

	// The main flowing-water corridor.
	ActiveChannel	UMETA(DisplayName = "Active Channel"),

	// Depositional gravel ground, usually on the inside of a bend or in a wider, slower reach.
	// The classic pay ground, and the reason inside bends matter to a prospector.
	GravelBar		UMETA(DisplayName = "Gravel Bar"),

	// A steep, eroding outside bank with little deposition.
	CutBank			UMETA(DisplayName = "Cut Bank"),

	// Low ground beside the river that may flood seasonally.
	Floodplain		UMETA(DisplayName = "Floodplain"),

	// An older raised river surface above the active floodplain - what ProspectorStart sits on.
	Terrace			UMETA(DisplayName = "Terrace"),

	// Exposed or shallow bedrock bordering the river. Gold works down to bedrock and stops there,
	// so an exposed margin is a natural trap.
	BedrockMargin	UMETA(DisplayName = "Bedrock Margin")
};

// Which way the water actually runs along the authored centreline. Stored explicitly rather than
// inferred from point order or from elevation: the source DEM's vertical resolution over this reach
// is coarser than the reach's true fall, so elevation alone cannot be trusted to say which end is
// upstream. Every downstream-ordered system (gold transport, deposition, travel distance) reads
// this, so guessing it wrong would silently invert the whole placer model.
UENUM(BlueprintType)
enum class EReachFlowDirection : uint8
{
	// Not yet established. Deliberately the default so nothing consumes an unverified direction.
	Undetermined			UMETA(DisplayName = "Undetermined"),

	// Water runs from spline point 0 toward the last point.
	TowardIncreasingIndex	UMETA(DisplayName = "Toward Increasing Index"),

	// Water runs from the last spline point back toward point 0.
	TowardDecreasingIndex	UMETA(DisplayName = "Toward Decreasing Index")
};

// One authored station along the reach: the gameplay-facing description of the river at a single
// point on the centreline. This is the permanent hydrology record - it is deliberately independent
// of whatever renders the visible water, so the water representation can be replaced later without
// touching anything that reads these values.
//
// Field set mirrors BoomtownUnity's RiverSample so the recovered design survives, minus the parts
// that only existed to serve procedural generation. Most fields are intentionally left at their
// defaults for now: this pass establishes the centreline and the shape of the record, not its
// contents. Baking real values is later, approved work.
USTRUCT(BlueprintType)
struct FReachHydrologySample
{
	GENERATED_BODY()

	// Distance along the centreline from the upstream end, in cm. Populated from the authored
	// spline rather than stored by hand, so it cannot drift out of sync with the geometry.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reach|Path")
	float DownstreamDistance = 0.0f;

	// Distance from the centreline to the water's edge on each side, in cm. Split rather than a
	// single width because a real channel is not centred in its own corridor - the deep water sits
	// against the cut bank while the bar builds out from the other side.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reach|Channel")
	float LeftWidth = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reach|Channel")
	float RightWidth = 0.0f;

	// Absolute world Z of the water surface at this station, in cm. Deliberately left at zero: this
	// is NOT yet solved, and the authored centreline's own Z is a valley-floor corridor mean, not a
	// water level, so it must not be copied in here as a stand-in. Establishing this needs a real
	// water-surface pass - see EReachFlowDirection for why the source elevation data cannot settle it.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reach|Channel")
	float WaterSurfaceElevation = 0.0f;

	// Approximate water depth below the surface, in cm.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reach|Channel")
	float Depth = 0.0f;

	// Interpreted environment on each bank, looking downstream.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reach|Banks")
	EReachBankType LeftBank = EReachBankType::Unclassified;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reach|Banks")
	EReachBankType RightBank = EReachBankType::Unclassified;

	// Signed curvature at this station: negative bends left, positive bends right, zero is straight.
	// Kept signed rather than as a bare "bend strength" because which side the bend favours is the
	// whole point - the bar builds on the inside, the cut bank forms on the outside.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reach|Banks", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float SignedBend = 0.0f;

	// Placeholder for where sediment and placer gold tend to settle, 0 to 1. Nothing computes or
	// consumes this yet; it exists so the record's shape is settled before deposition work starts.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reach|Deposition", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DepositionPotential = 0.0f;
};
