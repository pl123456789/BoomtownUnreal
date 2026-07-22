// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OverworldHeightfield.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;

// The visual, non-destructible "everywhere else" terrain: a smooth heightfield mesh sampled from a
// real DEM heightmap, with per-vertex normals from the height gradient (no blocky faces). Digging is
// not supported here - AGravelBarSite provides the only diggable ground, and matches its edges to
// GetSurfaceHeightAtWorldXY() so it blends into this mesh without a seam.
UCLASS()
class DESTRUCTION_API AOverworldHeightfield : public AActor
{
	GENERATED_BODY()

public:
	AOverworldHeightfield();

	// Path to a grayscale heightmap image, relative to the project's Content/ directory.
	UPROPERTY(EditAnywhere, Category = "Heightfield")
	FString HeightmapFilePath = TEXT("HeightMaps/Hope00.png");

	// World-space footprint of the whole heightfield, in cm.
	UPROPERTY(EditAnywhere, Category = "Heightfield")
	FVector2D WorldSize = FVector2D(12800.0f, 12800.0f);

	// Number of grid cells along each axis (vertex grid is (Resolution+1) x (Resolution+1)).
	UPROPERTY(EditAnywhere, Category = "Heightfield")
	int32 Resolution = 128;

	// Vertical relief range the contrast-stretched heightmap maps onto, in cm. Kept modest relative
	// to WorldSize - the source DEM covers a real mountain canyon many kilometres wide, so squeezing
	// its full relief onto a small in-game footprint at 1:1 would make every slope absurdly steep.
	UPROPERTY(EditAnywhere, Category = "Heightfield")
	float ReliefHeight = 1200.0f;

	// Box-blur passes applied to the sampled height grid to soften the faceted, jagged look that
	// comes from decimating a highly detailed DEM down to a coarse grid.
	UPROPERTY(EditAnywhere, Category = "Heightfield")
	int32 SmoothingPasses = 3;

	// How many times the height texture tiles across the whole footprint.
	UPROPERTY(EditAnywhere, Category = "Heightfield")
	float UVTiling = 32.0f;

	UPROPERTY(EditAnywhere, Category = "Heightfield")
	TObjectPtr<UMaterialInterface> HeightfieldMaterial;

	// Bilinearly-interpolated surface height (absolute world Z) at a given world-space XY. Returns
	// this actor's own Z as a fallback if the heightmap hasn't been sampled yet or XY is out of bounds.
	UFUNCTION(BlueprintCallable, Category = "Heightfield")
	float GetSurfaceHeightAtWorldXY(FVector2D WorldXY) const;

	// Scans the cached height grid for its lowest point (the real DEM's drainage channel / river,
	// per the box-filtered grayscale convention: dark = low). Used to place diggable sites where an
	// actual river exists in the source heightmap instead of at an arbitrary spot.
	UFUNCTION(BlueprintCallable, Category = "Heightfield")
	FVector2D FindLowestPointWorldXY() const;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Heightfield")
	TObjectPtr<UProceduralMeshComponent> MeshComponent;

	// Cached (Resolution+1) x (Resolution+1) sampled grid, contrast-stretched to [0, 1].
	TArray<float> Heights01;

	void BuildMesh();
};
