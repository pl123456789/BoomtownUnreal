// Copyright Epic Games, Inc. All Rights Reserved.

#include "DestructionGameMode.h"
#include "Prospector/ProspectorCharacter.h"
#include "Prospector/ProspectorPlayerController.h"
#include "Prospector/RTSCameraPawn.h"
#include "Terrain/OverworldHeightfield.h"
#include "Terrain/TerrainSurfaceQuery.h"
#include "Voxel/GravelBarSite.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"

ADestructionGameMode::ADestructionGameMode()
{
	DefaultPawnClass = ARTSCameraPawn::StaticClass();
	PlayerControllerClass = AProspectorPlayerController::StaticClass();
	OverworldClass = AOverworldHeightfield::StaticClass();
	GravelBarSiteClass = AGravelBarSite::StaticClass();
	ProspectorClass = AProspectorCharacter::StaticClass();
}

void ADestructionGameMode::BeginPlay()
{
	Super::BeginPlay();

	AOverworldHeightfield* Overworld = Cast<AOverworldHeightfield>(UGameplayStatics::GetActorOfClass(GetWorld(), AOverworldHeightfield::StaticClass()));
	if (!Overworld && bSpawnRuntimeHeightfield)
	{
		Overworld = GetWorld()->SpawnActor<AOverworldHeightfield>(OverworldClass ? *OverworldClass : AOverworldHeightfield::StaticClass(), FTransform::Identity);
	}

	// Place near the player start (offset so it doesn't overlap Bill's own spawn point) rather than
	// at the world origin - with the persistent Landscape as the normal terrain, the origin may be
	// nowhere near the actual playable area. GravelBarSite snaps its own Z to the traced ground
	// surface in PlaceNearRiver(), so only the XY placement matters here.
	AGravelBarSite* GravelSite = Cast<AGravelBarSite>(UGameplayStatics::GetActorOfClass(GetWorld(), AGravelBarSite::StaticClass()));
	if (!GravelSite)
	{
		FVector GravelSpawnLocation(6400.0f + GravelBarSiteOffsetFromStart.X, 6400.0f + GravelBarSiteOffsetFromStart.Y, 0.0f);
		if (APlayerStart* Start = Cast<APlayerStart>(UGameplayStatics::GetActorOfClass(GetWorld(), APlayerStart::StaticClass())))
		{
			const FVector StartLocation = Start->GetActorLocation();
			GravelSpawnLocation = FVector(StartLocation.X + GravelBarSiteOffsetFromStart.X, StartLocation.Y + GravelBarSiteOffsetFromStart.Y, 0.0f);
		}

		GravelSite = GetWorld()->SpawnActor<AGravelBarSite>(GravelBarSiteClass ? *GravelBarSiteClass : AGravelBarSite::StaticClass(), FTransform(GravelSpawnLocation));
	}

	// bAutoCreateNavigationData has proven unreliable for this world - MainNavData sometimes never
	// gets registered even though a RecastNavMesh actor exists in the level (AAIController::MoveTo
	// silently no-ops with no path found in that case). Force it explicitly now that the terrain
	// actors above have spawned and registered themselves as nav-relevant.
	if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		NavSys->GetDefaultNavDataInstance(FNavigationSystem::Create);

		// Registering a component as nav-relevant (done in the terrain actors' BuildMesh/RebuildMesh)
		// only updates the octree's spatial index - it doesn't by itself guarantee a tile rebuild gets
		// queued for that region. Explicitly dirty the terrain's own mesh bounds so Recast actually
		// voxelizes it now that the geometry exists, instead of leaving zero tiles generated.
		// (Not using the NavMeshBoundsVolume's bounds here - as a generically-spawned, never CSG-built
		// brush actor, GetComponentsBoundingBox() on it returns an invalid box at runtime; the terrain
		// actors have real ProceduralMeshComponent geometry, so their bounds are always valid.)
		FBox DirtyArea(ForceInit);
		if (Overworld)
		{
			DirtyArea += Overworld->GetComponentsBoundingBox();
		}
		if (GravelSite)
		{
			DirtyArea += GravelSite->GetComponentsBoundingBox();
		}
		if (DirtyArea.IsValid)
		{
			NavSys->AddDirtyArea(DirtyArea, ENavigationDirtyFlag::All, TEXT("TerrainSpawned"));
		}
	}

	if (!UGameplayStatics::GetActorOfClass(GetWorld(), AProspectorCharacter::StaticClass()))
	{
		FVector SpawnLocation(6400.0f, 6400.0f, 3500.0f);
		if (APlayerStart* Start = Cast<APlayerStart>(UGameplayStatics::GetActorOfClass(GetWorld(), APlayerStart::StaticClass())))
		{
			SpawnLocation = Start->GetActorLocation();
		}

		// Snap to whichever terrain is actually providing collision here (the persistent Landscape,
		// or the rollback heightfield) instead of trusting PlayerStart's own Z or the hardcoded
		// fallback - keeps Bill from spawning buried or floating regardless of terrain system.
		float GroundZ;
		if (FTerrainSurfaceQuery::TraceGroundZ(GetWorld(), SpawnLocation.X, SpawnLocation.Y, GroundZ))
		{
			SpawnLocation.Z = GroundZ + ProspectorSpawnStandingHeight;
		}

		GetWorld()->SpawnActor<AProspectorCharacter>(ProspectorClass ? *ProspectorClass : AProspectorCharacter::StaticClass(), FTransform(SpawnLocation));
	}
}
