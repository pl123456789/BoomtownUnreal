// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DestructionGameMode.generated.h"

class AOverworldHeightfield;
class AGravelBarSite;
class AProspectorCharacter;

UCLASS()
class DESTRUCTION_API ADestructionGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADestructionGameMode();

	// Spawned automatically in BeginPlay if none already exist in the level and bSpawnRuntimeHeightfield
	// is true. Preserved as the rollback terrain path (M0013) now that a persistent Landscape provides
	// normal gameplay terrain - kept fully intact, just not spawned by default. Flip
	// bSpawnRuntimeHeightfield back to true to restore the old runtime-generated terrain with no other
	// changes required.
	UPROPERTY(EditDefaultsOnly, Category = "Terrain")
	TSubclassOf<AOverworldHeightfield> OverworldClass;

	// M0013: the persistent Landscape saved in the level is now the normal gameplay terrain, so the
	// old runtime-generated heightfield is disabled by default. Left as a property (not deleted) so
	// re-enabling it is a one-flag rollback.
	UPROPERTY(EditDefaultsOnly, Category = "Terrain")
	bool bSpawnRuntimeHeightfield = false;

	UPROPERTY(EditDefaultsOnly, Category = "Terrain")
	TSubclassOf<AGravelBarSite> GravelBarSiteClass;

	// Extra offset (world XY) from the player start used to place the gravel bar site so it doesn't
	// overlap the player's own spawn point.
	UPROPERTY(EditDefaultsOnly, Category = "Terrain")
	FVector2D GravelBarSiteOffsetFromStart = FVector2D(2000.0f, 2000.0f);

	// The commandable unit - not the player's pawn (that's the independent RTSCameraPawn), spawned
	// separately and AI-controlled.
	UPROPERTY(EditDefaultsOnly, Category = "Prospector")
	TSubclassOf<AProspectorCharacter> ProspectorClass;

	// Standing height above the traced ground surface Bill's spawn snaps to, so he lands standing on
	// the terrain instead of embedded in it or floating - independent of PlayerStart's own Z or which
	// terrain system (Landscape or the rollback heightfield) is actually providing collision there.
	UPROPERTY(EditDefaultsOnly, Category = "Prospector")
	float ProspectorSpawnStandingHeight = 92.0f;

protected:
	virtual void BeginPlay() override;
};
