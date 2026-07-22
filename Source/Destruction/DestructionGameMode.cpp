// Copyright Epic Games, Inc. All Rights Reserved.

#include "DestructionGameMode.h"
#include "Prospector/ProspectorCharacter.h"
#include "Prospector/ProspectorPlayerController.h"
#include "Prospector/RTSCameraPawn.h"
#include "Terrain/OverworldHeightfield.h"
#include "Voxel/GravelBarSite.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

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

	if (!UGameplayStatics::GetActorOfClass(GetWorld(), AOverworldHeightfield::StaticClass()))
	{
		GetWorld()->SpawnActor<AOverworldHeightfield>(OverworldClass ? *OverworldClass : AOverworldHeightfield::StaticClass(), FTransform::Identity);
	}

	if (!UGameplayStatics::GetActorOfClass(GetWorld(), AGravelBarSite::StaticClass()))
	{
		GetWorld()->SpawnActor<AGravelBarSite>(GravelBarSiteClass ? *GravelBarSiteClass : AGravelBarSite::StaticClass(), FTransform::Identity);
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
