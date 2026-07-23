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

	// Horizontal facing yaw of the view (actor yaw + boom yaw). Used to make selected-character WASD
	// movement camera-relative; stays correct if the camera gains yaw rotation later (M0007).
	float GetViewYaw() const;

	virtual void Tick(float DeltaTime) override;

	// Moves the camera's X/Y to Target's X/Y immediately (no interpolation). Z, zoom, pitch and yaw are
	// left untouched; Target itself is never moved.
	void CenterOnActor(const AActor* Target);

	// Centers on Target immediately, then ticks every frame to keep matching its X/Y until stopped.
	void BeginFollowingActor(AActor* Target);

	// Stops following, if active, and disables ticking. Safe to call even when not currently following.
	void StopFollowingActor();

	bool IsFollowingActor() const { return FollowTarget.IsValid(); }

private:
	// Weak so a destroyed/invalidated target clears itself - Tick() detects this and stops following on
	// its own, leaving the camera exactly where it was (no world-scan, no auto-reselect).
	TWeakObjectPtr<AActor> FollowTarget;
};
