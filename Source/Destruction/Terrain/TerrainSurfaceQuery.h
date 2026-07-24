// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// Reusable ground-height query used by gameplay code that needs "what's the terrain surface Z
// here" without depending on which system is actually providing collision. A line trace against
// WorldStatic collision finds whichever terrain is physically present - the persistent Landscape
// during normal play, or the rollback AOverworldHeightfield/AGravelBarSite if that's ever
// re-enabled instead - with no direct reference to either class.
class DESTRUCTION_API FTerrainSurfaceQuery
{
public:
	// Traces straight down through WorldX/WorldY across a very large Z range and returns the
	// first WorldStatic hit. Returns false if nothing was hit.
	static bool TraceGroundZ(const UWorld* World, float WorldX, float WorldY, float& OutZ);
};
