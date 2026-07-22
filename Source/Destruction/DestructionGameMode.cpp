// Copyright Epic Games, Inc. All Rights Reserved.

#include "DestructionGameMode.h"
#include "Prospector/ProspectorCharacter.h"
#include "Prospector/ProspectorPlayerController.h"
#include "Prospector/RTSCameraPawn.h"
#include "Terrain/OverworldHeightfield.h"
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
	if (!Overworld)
	{
		Overworld = GetWorld()->SpawnActor<AOverworldHeightfield>(OverworldClass ? *OverworldClass : AOverworldHeightfield::StaticClass(), FTransform::Identity);
	}

	AGravelBarSite* GravelSite = Cast<AGravelBarSite>(UGameplayStatics::GetActorOfClass(GetWorld(), AGravelBarSite::StaticClass()));
	if (!GravelSite)
	{
		GravelSite = GetWorld()->SpawnActor<AGravelBarSite>(GravelBarSiteClass ? *GravelBarSiteClass : AGravelBarSite::StaticClass(), FTransform::Identity);
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

		GetWorld()->SpawnActor<AProspectorCharacter>(ProspectorClass ? *ProspectorClass : AProspectorCharacter::StaticClass(), FTransform(SpawnLocation));
	}
}
