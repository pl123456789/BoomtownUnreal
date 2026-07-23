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

	FVector NewLocation = GetActorLocation();
	const FVector TargetLocation = Target->GetActorLocation();

	// The boom is pitched downward, so the camera's center ray - which passes through this pawn's root
	// position offset backward by TargetArmLength along Forward, per USpringArmComponent's own
	// DesiredLoc -= Forward * TargetArmLength - only intersects a target at the SAME Z as this pawn's
	// root. Naively copying Target's X/Y onto the root's X/Y (the old approach) is therefore only exact
	// when TargetLocation.Z == NewLocation.Z, which is false in general (a standing character capsule
	// sits well above this pawn's root height): solve instead for the root X/Y that puts Target exactly
	// on the ray. Parametrizing the ray as (RootPos - Forward*L) + t*Forward and requiring its Z to match
	// TargetLocation.Z gives (t - L) = (TargetLocation.Z - RootPos.Z) / Forward.Z; substituting into the
	// ray's X/Y makes the L terms cancel out entirely, so this correction is identical at every zoom
	// level - exactly why the old drift was more or less visible depending on zoom rather than caused by
	// it, and why this fix holds at minimum, maximum and intermediate arm lengths alike.
	const FVector Forward = CameraBoom->GetComponentRotation().Vector();
	if (!FMath::IsNearlyZero(Forward.Z, 0.0001f))
	{
		const float K = (TargetLocation.Z - NewLocation.Z) / Forward.Z;
		NewLocation.X = TargetLocation.X - Forward.X * K;
		NewLocation.Y = TargetLocation.Y - Forward.Y * K;
	}
	else
	{
		// Near-horizontal look direction - not reachable with this camera's fixed -55 degree pitch, but
		// guarded in case pitch ever changes, rather than dividing by a near-zero Forward.Z.
		NewLocation.X = TargetLocation.X;
		NewLocation.Y = TargetLocation.Y;
	}

	SetActorLocation(NewLocation);
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
