// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GeoTypes.h"
#include "GravelBarSite.generated.h"

class UDensityChunkComponent;

// The one diggable pocket in this prototype: a small smooth-voxel volume with layered geology
// (bedrock -> gravel -> sand -> topsoil) and gold concentration weighted toward bedrock plus a
// couple of hand-placed pay-streak pockets. Its own XY placement comes from wherever the game
// mode spawns it (see ADestructionGameMode); PlaceNearRiver() snaps its Z to the traced ground
// surface at that XY via FTerrainSurfaceQuery, and every column's surface height in
// GenerateGeology() is sampled the same way, so it blends into whichever terrain is actually
// present (the persistent Landscape, or the rollback AOverworldHeightfield) without a seam - with
// no direct dependency on either terrain class.
UCLASS()
class DESTRUCTION_API AGravelBarSite : public AActor
{
	GENERATED_BODY()

public:
	AGravelBarSite();

	// Horizontal footprint and depth of the diggable volume, in cm.
	UPROPERTY(EditAnywhere, Category = "GravelBar")
	FVector SiteSize = FVector(800.0f, 800.0f, 300.0f);

	// Density grid spacing, in cm - the doc's suggested 10-20cm resolution.
	UPROPERTY(EditAnywhere, Category = "GravelBar")
	float VoxelResolution = 20.0f;

	UPROPERTY(EditAnywhere, Category = "GravelBar")
	float TopsoilDepth = 20.0f;

	UPROPERTY(EditAnywhere, Category = "GravelBar")
	float SandDepth = 15.0f;

	UPROPERTY(EditAnywhere, Category = "GravelBar")
	float BedrockDepth = 40.0f;

	// Total gold actually present in this whole site, in grams. Mass-conserved: every column's gold
	// is carved out of this fixed budget by its hydraulic trap score rather than from an unbounded
	// per-cubic-metre concentration, so a richer spot is only richer at a poorer spot's expense - the
	// way a real placer deposit works, instead of gold appearing from nowhere.
	UPROPERTY(EditAnywhere, Category = "GravelBar|Gold")
	float TotalGoldReserveGrams = 40.0f;

	// Hydraulic trap score weights - how much each factor biases where, within the reserve, gold
	// actually ends up. Mirrors real placer deposition: gold drops out where the current slows and
	// shallows over flat gravel near the channel, not randomly.
	UPROPERTY(EditAnywhere, Category = "GravelBar|Gold")
	float GravelProbabilityWeight = 0.31f;

	UPROPERTY(EditAnywhere, Category = "GravelBar|Gold")
	float SlowWaterWeight = 0.24f;

	UPROPERTY(EditAnywhere, Category = "GravelBar|Gold")
	float BankTrapWeight = 0.19f;

	UPROPERTY(EditAnywhere, Category = "GravelBar|Gold")
	float BendTrapWeight = 0.14f;

	UPROPERTY(EditAnywhere, Category = "GravelBar|Gold")
	float ShallowWaterWeight = 0.12f;

	// Spatial frequency of the synthetic meander/flow noise. We don't carry a true river spline into
	// this small a site yet, so bend strength and flow speed are approximated with noise at roughly a
	// real meander wavelength rather than left uniform.
	UPROPERTY(EditAnywhere, Category = "GravelBar|Gold")
	float MeanderFrequency = 0.15f;

	UPROPERTY(EditAnywhere, Category = "GravelBar")
	int32 RandomSeed = 8675309;

	// Digs a sphere of the given radius at a world-space location. Returns an empty packet if the
	// location is outside this site's volume, or if it only hit bedrock.
	UFUNCTION(BlueprintCallable, Category = "GravelBar")
	FSedimentPacket DigSphereAtWorldLocation(FVector WorldLocation, float Radius);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "GravelBar")
	TObjectPtr<UDensityChunkComponent> Chunk;

	void PlaceNearRiver();
	void GenerateGeology();
};
