// Copyright Epic Games, Inc. All Rights Reserved.

#include "RTSCameraPawn.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

ARTSCameraPawn::ARTSCameraPawn()
{
	PrimaryActorTick.bCanEverTick = false;

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
