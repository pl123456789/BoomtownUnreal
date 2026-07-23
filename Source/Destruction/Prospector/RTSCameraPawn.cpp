// Copyright Epic Games, Inc. All Rights Reserved.

#include "RTSCameraPawn.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

ARTSCameraPawn::ARTSCameraPawn()
{
	// Tick is only needed while following a unit - capable of ticking, but started disabled and
	// toggled on/off at runtime by Begin/StopFollowingActor rather than running every frame otherwise.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 1600.0f;
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->SetRelativeRotation(FRotator(-55.0f, 0.0f, 0.0f));

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
}

void ARTSCameraPawn::PanBy(const FVector2D& Direction, float DeltaSeconds)
{
	const FRotator YawRotation(0.0f, CameraBoom->GetRelativeRotation().Yaw, 0.0f);
	const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddActorWorldOffset((Forward * Direction.Y + Right * Direction.X) * PanSpeed * DeltaSeconds);
}

void ARTSCameraPawn::ZoomBy(float Delta)
{
	CameraBoom->TargetArmLength = FMath::Clamp(CameraBoom->TargetArmLength - Delta * ZoomStep, MinArmLength, MaxArmLength);
}

float ARTSCameraPawn::GetViewYaw() const
{
	return GetActorRotation().Yaw + CameraBoom->GetRelativeRotation().Yaw;
}

void ARTSCameraPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AActor* Target = FollowTarget.Get();
	if (!Target)
	{
		// Followed actor was destroyed/invalidated since the last tick - stop safely in place, no
		// world-scan and no automatic replacement target.
		StopFollowingActor();
		return;
	}

	CenterOnActor(Target);
}

void ARTSCameraPawn::CenterOnActor(const AActor* Target)
{
	if (!Target)
	{
		return;
	}

	// The followed unit is the actual orbit pivot: the camera pawn's root is placed exactly at Target's
	// world location (X, Y AND Z - superseding M0009A's Z-preserving, ray-intersection approach, which
	// only kept Target centered for a fixed pitch. Under variable pitch that approach let the true
	// camera-to-target distance drift away from TargetArmLength, shrinking the unit at shallow pitch even
	// though the arm length itself never changed). Since USpringArmComponent computes the camera position
	// as Cam = RootPos - Forward*TargetArmLength and looks along +Forward toward RootPos, placing RootPos
	// exactly on Target puts Target trivially on the center ray at exactly TargetArmLength from the
	// camera, for any pitch or yaw - centered and at consistent apparent size with no ray math needed.
	SetActorLocation(Target->GetActorLocation());
}

void ARTSCameraPawn::OrbitBy(const FVector2D& MouseDelta)
{
	FRotator BoomRotation = CameraBoom->GetRelativeRotation();

	// Yaw: moving the mouse right increases yaw, rotating the view right (standard UE rotator
	// convention: increasing yaw turns clockwise viewed from above). Wraps continuously - never clamped.
	BoomRotation.Yaw = FRotator::NormalizeAxis(BoomRotation.Yaw + MouseDelta.X * OrbitYawSpeed);

	// Pitch: raw Mouse2D.Y follows screen-space convention (moving the mouse up reports a negative raw
	// delta), so it's negated here to make "mouse up" increase pitch toward the shallower MaxPitch and
	// "mouse down" decrease it toward the steeper MinPitch, per the milestone's specified feel. If this
	// reads as inverted once tested live, flipping the sign of OrbitPitchSpeed alone corrects it.
	BoomRotation.Pitch = FMath::Clamp(BoomRotation.Pitch - MouseDelta.Y * OrbitPitchSpeed, MinPitch, MaxPitch);

	BoomRotation.Roll = 0.0f;

	CameraBoom->SetRelativeRotation(BoomRotation);

	if (AActor* Target = FollowTarget.Get())
	{
		CenterOnActor(Target);
	}
}

void ARTSCameraPawn::BeginFollowingActor(AActor* Target)
{
	if (!Target)
	{
		return;
	}

	FollowTarget = Target;
	CenterOnActor(Target);
	SetActorTickEnabled(true);
}

void ARTSCameraPawn::StopFollowingActor()
{
	FollowTarget = nullptr;
	SetActorTickEnabled(false);
}
