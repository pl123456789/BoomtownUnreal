// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "RTSCameraPawn.generated.h"

class USpringArmComponent;
class UCameraComponent;

// An independent, floating top-down camera the player directly possesses - COH3-style: WASD pans
// the view across the ground plane and the mouse wheel zooms, but it is not attached to any unit.
// AProspectorCharacter is commanded via right-click (see AProspectorPlayerController), not possessed.
UCLASS()
class DESTRUCTION_API ARTSCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	ARTSCameraPawn();

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	// World units per second at Direction magnitude 1.
	UPROPERTY(EditAnywhere, Category = "Camera")
	float PanSpeed = 2000.0f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float ZoomStep = 200.0f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float MinArmLength = 400.0f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float MaxArmLength = 4000.0f;

	// Moves the camera across the ground plane. Direction.Y = forward (up-screen), Direction.X =
	// right (right-screen), both relative to the boom's fixed yaw so panning feels screen-relative
	// despite the fixed downward camera tilt.
	void PanBy(const FVector2D& Direction, float DeltaSeconds);

	// Adjusts zoom by changing the spring arm length, clamped to [MinArmLength, MaxArmLength].
	void ZoomBy(float Delta);
};
