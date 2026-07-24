// Copyright Epic Games, Inc. All Rights Reserved.

#include "TerrainSurfaceQuery.h"
#include "Engine/World.h"
#include "Engine/HitResult.h"

bool FTerrainSurfaceQuery::TraceGroundZ(const UWorld* World, float WorldX, float WorldY, float& OutZ)
{
	if (!World)
	{
		return false;
	}

	// Large enough to clear both the old prototype's ~12m relief and the persistent Landscape's
	// real-world elevation range (observed up to roughly +-900m for the Hope TRIM import).
	static constexpr float TraceHalfHeight = 1000000.0f;
	const FVector Start(WorldX, WorldY, TraceHalfHeight);
	const FVector End(WorldX, WorldY, -TraceHalfHeight);

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TerrainSurfaceQuery), false);
	if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params))
	{
		OutZ = Hit.Location.Z;
		return true;
	}

	return false;
}
