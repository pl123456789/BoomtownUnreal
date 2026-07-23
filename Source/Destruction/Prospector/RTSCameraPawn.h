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

	// Degrees of yaw applied per unit of horizontal mouse delta while orbiting.
	UPROPERTY(EditAnywhere, Category = "Camera")
	float OrbitYawSpeed = 0.8f;

	// Degrees of pitch applied per unit of vertical mouse delta while orbiting.
	UPROPERTY(EditAnywhere, Category = "Camera")
	float OrbitPitchSpeed = 0.8f;

	// Steepest allowed pitch (most negative - looking most steeply down).
	UPROPERTY(EditAnywhere, Category = "Camera")
	float MinPitch = -80.0f;

	// Shallowest allowed pitch (least negative - closest to the horizon). Near-horizontal rather than
	// -25 so, combined with the follow-pivot correction, orbiting to this end gives a genuine eye-height/
	// over-the-shoulder view of the followed unit instead of always looking down at it.
	UPROPERTY(EditAnywhere, Category = "Camera")
	float MaxPitch = -5.0f;

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

	// Moves the camera pawn's root to Target's exact world location (X, Y and Z) immediately, making
	// Target the actual orbit pivot - see the .cpp for why this, not preserving the pawn's own Z, is
	// what keeps Target centered and at a consistent apparent size across the full pitch range. Zoom
	// (TargetArmLength), pitch and yaw are left untouched; Target itself is never moved.
	void CenterOnActor(const AActor* Target);

	// Centers on Target immediately, then ticks every frame to keep matching its full world location
	// until stopped.
	void BeginFollowingActor(AActor* Target);

	// Stops following, if active, and disables ticking. Safe to call even when not currently following.
	void StopFollowingActor();

	bool IsFollowingActor() const { return FollowTarget.IsValid(); }

	// Rotates the camera boom by a raw mouse delta while middle-mouse orbit is held: horizontal delta
	// changes yaw (wraps continuously through 360 degrees via FRotator::NormalizeAxis), vertical delta
	// changes pitch (clamped to [MinPitch, MaxPitch]); roll always stays zero. Never touches
	// TargetArmLength. If no follow target is active, the pawn's root position is untouched too, so
	// rotation orbits around wherever the free camera currently is. If a follow target IS active, calls
	// CenterOnActor afterward, which relocates the root onto the target - making the target the actual
	// orbit pivot, so it stays centered and at a consistent apparent size under the new angle.
	void OrbitBy(const FVector2D& MouseDelta);

private:
	// Weak so a destroyed/invalidated target clears itself - Tick() detects this and stops following on
	// its own, leaving the camera exactly where it was (no world-scan, no auto-reselect).
	TWeakObjectPtr<AActor> FollowTarget;
};
