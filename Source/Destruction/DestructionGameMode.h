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

	// Spawned automatically in BeginPlay if none already exist in the level. Spawned in this order -
	// the gravel bar site's BeginPlay samples the overworld's height grid to place and shape itself,
	// so the overworld must already exist first.
	UPROPERTY(EditDefaultsOnly, Category = "Terrain")
	TSubclassOf<AOverworldHeightfield> OverworldClass;

	UPROPERTY(EditDefaultsOnly, Category = "Terrain")
	TSubclassOf<AGravelBarSite> GravelBarSiteClass;

	// The commandable unit - not the player's pawn (that's the independent RTSCameraPawn), spawned
	// separately and AI-controlled.
	UPROPERTY(EditDefaultsOnly, Category = "Prospector")
	TSubclassOf<AProspectorCharacter> ProspectorClass;

protected:
	virtual void BeginPlay() override;
};
