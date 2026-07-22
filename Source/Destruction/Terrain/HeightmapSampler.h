// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// Loads a grayscale heightmap image from disk (relative to Content/) and box-filters it down to
// an arbitrary sample grid, contrast-stretched to [0, 1]. Shared by anything that needs to match
// the real DEM relief in Content/HeightMaps/Hope00.png - the overworld heightfield mesh and the
// gravel bar site's river-finding logic both sample through this instead of decoding the PNG twice.
class FHeightmapSampler
{
public:
	// SmoothingPasses applies simple 3x3 box blurs after the box-filter/contrast-stretch, to soften
	// the faceted, jagged look that comes from decimating a highly detailed real-world DEM down to a
	// coarse sample grid stretched over a much smaller in-game footprint.
	// Returns false (leaving OutHeights01 untouched) if the file can't be loaded/decoded.
	static bool SampleHeightmap(const FString& RelativeContentPath, int32 SampleWidth, int32 SampleHeight, TArray<float>& OutHeights01, int32 SmoothingPasses = 2);
};
