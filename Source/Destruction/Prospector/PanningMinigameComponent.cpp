// Copyright Epic Games, Inc. All Rights Reserved.

#include "PanningMinigameComponent.h"
#include "Engine/Engine.h"

UPanningMinigameComponent::UPanningMinigameComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetComponentTickEnabled(false);
}

void UPanningMinigameComponent::StartMinigame(const FSedimentPacket& Bucket)
{
	if (bIsActive || Bucket.TotalMassKg() <= 0.0f)
	{
		return;
	}

	bIsActive = true;
	StartingJunkKg = Bucket.TotalMassKg();
	Remaining = Bucket;
	LostSoFar = FSedimentPacket();
	TimeRemaining = Duration;
	CurrentTilt = FVector2D::ZeroVector;
	FlowNoiseTime = FMath::FRand() * 1000.0f;

	SetComponentTickEnabled(true);
	OnProgressChanged.Broadcast(1.0f);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, Duration + 1.0f, FColor::Cyan,
			FString::Printf(TEXT("Panning started: %.1f kg sediment, %.2f g gold in the pan. Tilt to wash out the junk!"),
				StartingJunkKg, Bucket.TotalGoldGrams()));
	}
}

void UPanningMinigameComponent::SetTiltInput(FVector2D Tilt)
{
	CurrentTilt = Tilt.GetClampedToMaxSize(1.0f);
}

void UPanningMinigameComponent::CancelMinigame()
{
	if (bIsActive)
	{
		FinishMinigame(false);
	}
}

FVector2D UPanningMinigameComponent::GetFlowDirection() const
{
	const float Angle = FMath::PerlinNoise1D(FlowNoiseTime) * PI * 2.0f;
	return FVector2D(FMath::Cos(Angle), FMath::Sin(Angle));
}

void UPanningMinigameComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsActive)
	{
		return;
	}

	FlowNoiseTime += DeltaTime * FlowDirectionChangeSpeed;
	TimeRemaining -= DeltaTime;

	const float TiltMagnitude = CurrentTilt.Size();

	if (TiltMagnitude > KINDA_SMALL_NUMBER)
	{
		const FVector2D TiltDirection = CurrentTilt / TiltMagnitude;
		const float Alignment = FMath::Max(0.0f, FVector2D::DotProduct(TiltDirection, GetFlowDirection()));
		const float WashRate = (BaseWashRate + TiltWashMultiplier * TiltMagnitude) * Alignment;

		const float JunkBefore = Remaining.TotalMassKg();
		if (JunkBefore > 0.0f)
		{
			const float KgToRemove = FMath::Min(JunkBefore, WashRate * DeltaTime);
			const float KeepFactor = (JunkBefore - KgToRemove) / JunkBefore;
			const float LoseFactor = 1.0f - KeepFactor;

			LostSoFar.TopsoilKg += Remaining.TopsoilKg * LoseFactor;
			LostSoFar.ClayKg += Remaining.ClayKg * LoseFactor;
			LostSoFar.SandKg += Remaining.SandKg * LoseFactor;
			LostSoFar.GravelKg += Remaining.GravelKg * LoseFactor;

			Remaining.TopsoilKg *= KeepFactor;
			Remaining.ClayKg *= KeepFactor;
			Remaining.SandKg *= KeepFactor;
			Remaining.GravelKg *= KeepFactor;
		}

		if (TiltMagnitude > OvertiltThreshold)
		{
			const float FineLoss = Remaining.FineGoldGrams * FMath::Clamp(FineGoldLossRatePerSecond * DeltaTime, 0.0f, 1.0f);
			Remaining.FineGoldGrams -= FineLoss;
			LostSoFar.FineGoldGrams += FineLoss;

			const float FlakeLoss = Remaining.FlakeGoldGrams * FMath::Clamp(FlakeGoldLossRatePerSecond * DeltaTime, 0.0f, 1.0f);
			Remaining.FlakeGoldGrams -= FlakeLoss;
			LostSoFar.FlakeGoldGrams += FlakeLoss;

			const float NuggetLoss = Remaining.NuggetGoldGrams * FMath::Clamp(NuggetGoldLossRatePerSecond * DeltaTime, 0.0f, 1.0f);
			Remaining.NuggetGoldGrams -= NuggetLoss;
			LostSoFar.NuggetGoldGrams += NuggetLoss;
		}
	}

	OnProgressChanged.Broadcast(StartingJunkKg > 0.0f ? Remaining.TotalMassKg() / StartingJunkKg : 0.0f);

	if (Remaining.TotalMassKg() <= 0.0f)
	{
		FinishMinigame(true);
	}
	else if (TimeRemaining <= 0.0f)
	{
		FinishMinigame(false);
	}
}

void UPanningMinigameComponent::FinishMinigame(bool bSuccess)
{
	bIsActive = false;
	SetComponentTickEnabled(false);

	FSedimentPacket LostFinal = LostSoFar;
	float GoldBanked = 0.0f;

	if (bSuccess)
	{
		GoldBanked = Remaining.TotalGoldGrams();
	}
	else
	{
		// Ran out of time (or was cancelled) with junk still in the pan - only recover the fraction
		// of gold proportional to how much of the junk actually got washed away; the rest, plus
		// whatever junk never washed out, is dumped into the tailings.
		const float FractionJunkLeft = StartingJunkKg > 0.0f ? FMath::Clamp(Remaining.TotalMassKg() / StartingJunkKg, 0.0f, 1.0f) : 0.0f;
		const float KeepFraction = 1.0f - FractionJunkLeft;
		GoldBanked = Remaining.TotalGoldGrams() * KeepFraction;

		LostFinal.TopsoilKg += Remaining.TopsoilKg;
		LostFinal.ClayKg += Remaining.ClayKg;
		LostFinal.SandKg += Remaining.SandKg;
		LostFinal.GravelKg += Remaining.GravelKg;
		LostFinal.FineGoldGrams += Remaining.FineGoldGrams * FractionJunkLeft;
		LostFinal.FlakeGoldGrams += Remaining.FlakeGoldGrams * FractionJunkLeft;
		LostFinal.NuggetGoldGrams += Remaining.NuggetGoldGrams * FractionJunkLeft;
	}

	Remaining = FSedimentPacket();
	LostSoFar = FSedimentPacket();

	OnMinigameFinished.Broadcast(bSuccess, GoldBanked, LostFinal);
}
