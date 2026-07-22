// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "GeoTypes.h"
#include "DensityChunkComponent.generated.h"

// A single smooth-surface diggable volume, extracted from a 3D density field via Naive Surface Nets:
// one vertex per surface-crossing cell (positioned at the averaged zero-crossing of its edges), with
// quads connecting the 4 cells around every interior grid edge that crosses the surface. Chosen over
// classic Marching Cubes so meshing doesn't depend on its large 256-entry triangulation table - this
// gives the same smooth, non-blocky result for our purposes. Interior holes/craters (the actual
// digging gameplay) always mesh correctly; the outer perimeter/floor of the grid is intentionally left
// unmeshed, since this volume is meant to sit embedded in AOverworldHeightfield with only its dug-into
// interior ever exposed.
UCLASS(ClassGroup = (Voxel), meta = (BlueprintSpawnableComponent))
class DESTRUCTION_API UDensityChunkComponent : public UProceduralMeshComponent
{
	GENERATED_BODY()

public:
	UDensityChunkComponent(const FObjectInitializer& ObjectInitializer);

	// GridDims = number of density sample points along X/Y/Z (there are GridDims-1 cells per axis).
	void InitChunk(const FIntVector& InGridDims, float InVoxelSize);

	// Sets one grid point's density (positive = solid, zero/negative = air) and geology. Generation-time only.
	void SetPointDuringGeneration(int32 X, int32 Y, int32 Z, float InDensity, EGeoMaterial InMaterial, float InGoldGramsPerM3);

	float GetDensity(int32 X, int32 Y, int32 Z) const;
	EGeoMaterial GetMaterialAt(int32 X, int32 Y, int32 Z) const;

	// Rebuilds the render + collision mesh from the current density field via Surface Nets.
	void RebuildMesh();

	// Subtracts a sphere of the given radius (component-local space) from the density field. Never
	// removes Bedrock/Quartz (the hard floor/veins). Returns the mass/gold that came loose and
	// rebuilds the mesh before returning.
	FSedimentPacket SubtractSphere(FVector LocalLocation, float Radius);

	float GetVoxelSize() const { return VoxelSize; }
	FIntVector GetGridDims() const { return GridDims; }

private:
	FIntVector GridDims = FIntVector(1, 1, 1);
	float VoxelSize = 20.0f;

	TArray<float> Density;
	TArray<EGeoMaterial> Material;
	TArray<float> GoldGramsPerM3;

	FORCEINLINE bool InBounds(int32 X, int32 Y, int32 Z) const
	{
		return X >= 0 && Y >= 0 && Z >= 0 && X < GridDims.X && Y < GridDims.Y && Z < GridDims.Z;
	}

	FORCEINLINE int32 PointIndex(int32 X, int32 Y, int32 Z) const
	{
		return X + Y * GridDims.X + Z * GridDims.X * GridDims.Y;
	}

	float GetGoldAt(int32 X, int32 Y, int32 Z) const;
};
