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
	NewLocation.X = TargetLocation.X;
	NewLocation.Y = TargetLocation.Y;
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
