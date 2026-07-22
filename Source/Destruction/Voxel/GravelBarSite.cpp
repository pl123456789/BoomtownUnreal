// Copyright Epic Games, Inc. All Rights Reserved.

#include "GravelBarSite.h"
#include "DensityChunkComponent.h"
#include "Terrain/OverworldHeightfield.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

AGravelBarSite::AGravelBarSite()
{
	PrimaryActorTick.bCanEverTick = false;

	Chunk = CreateDefaultSubobject<UDensityChunkComponent>(TEXT("Chunk"));
	RootComponent = Chunk;
}

void AGravelBarSite::BeginPlay()
{
	Super::BeginPlay();

	PlaceNearRiver();
	GenerateGeology();
	Chunk->RebuildMesh();
}

void AGravelBarSite::PlaceNearRiver()
{
	AOverworldHeightfield* Overworld = Cast<AOverworldHeightfield>(UGameplayStatics::GetActorOfClass(GetWorld(), AOverworldHeightfield::StaticClass()));
	if (!Overworld)
	{
		return;
	}

	const FVector2D RiverXY = Overworld->FindLowestPointWorldXY();
	const float SurfaceZ = Overworld->GetSurfaceHeightAtWorldXY(RiverXY);

	// Bottom-back corner of the box sits at RiverXY (centered on the river point), with enough
	// headroom above the sampled surface for the real terrain to vary a bit across the footprint
	// without running off the top or bottom of the density grid.
	const float HeadroomAboveSurface = 100.0f;
	const FVector NewLocation(
		RiverXY.X - SiteSize.X * 0.5f,
		RiverXY.Y - SiteSize.Y * 0.5f,
		SurfaceZ - (SiteSize.Z - HeadroomAboveSurface));

	SetActorLocation(NewLocation);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 120.0f, FColor::Magenta,
			FString::Printf(TEXT("GravelBarSite placed at X=%.0f Y=%.0f Z=%.0f"), NewLocation.X, NewLocation.Y, NewLocation.Z));
	}
}

void AGravelBarSite::GenerateGeology()
{
	AOverworldHeightfield* Overworld = Cast<AOverworldHeightfield>(UGameplayStatics::GetActorOfClass(GetWorld(), AOverworldHeightfield::StaticClass()));

	const int32 NX = FMath::Max(2, FMath::RoundToInt(SiteSize.X / VoxelResolution) + 1);
	const int32 NY = FMath::Max(2, FMath::RoundToInt(SiteSize.Y / VoxelResolution) + 1);
	const int32 NZ = FMath::Max(2, FMath::RoundToInt(SiteSize.Z / VoxelResolution) + 1);
	Chunk->InitChunk(FIntVector(NX, NY, NZ), VoxelResolution);

	const int32 BedrockCellZ = FMath::RoundToInt(BedrockDepth / VoxelResolution);
	const int32 TopsoilCells = FMath::Max(1, FMath::RoundToInt(TopsoilDepth / VoxelResolution));
	const int32 SandCells = FMath::Max(1, FMath::RoundToInt(SandDepth / VoxelResolution));

	const FVector ActorLoc = GetActorLocation();

	// Pass 1: sample the real surface height per column, and find this site's own lowest point as a
	// proxy for where the actual water channel runs through it (we don't carry a true river spline
	// into a site this small yet).
	TArray<int32> SurfaceCellZs;
	TArray<float> TrapScores;
	SurfaceCellZs.SetNumUninitialized(NX * NY);
	TrapScores.SetNumUninitialized(NX * NY);

	int32 LowestColumnIndex = 0;
	int32 LowestSurfaceCellZ = TNumericLimits<int32>::Max();

	for (int32 Y = 0; Y < NY; ++Y)
	{
		for (int32 X = 0; X < NX; ++X)
		{
			const int32 ColumnIdx = X + Y * NX;
			const FVector WorldXY = ActorLoc + FVector(X * VoxelResolution, Y * VoxelResolution, 0.0f);
			const float SurfaceWorldZ = Overworld ? Overworld->GetSurfaceHeightAtWorldXY(FVector2D(WorldXY.X, WorldXY.Y)) : ActorLoc.Z + SiteSize.Z * 0.5f;

			float SurfaceLocalZ = SurfaceWorldZ - ActorLoc.Z;
			SurfaceLocalZ = FMath::Clamp(SurfaceLocalZ, BedrockDepth + VoxelResolution, SiteSize.Z - VoxelResolution);
			const int32 SurfaceCellZ = FMath::RoundToInt(SurfaceLocalZ / VoxelResolution);
			SurfaceCellZs[ColumnIdx] = SurfaceCellZ;

			if (SurfaceCellZ < LowestSurfaceCellZ)
			{
				LowestSurfaceCellZ = SurfaceCellZ;
				LowestColumnIndex = ColumnIdx;
			}
		}
	}

	const int32 ChannelX = LowestColumnIndex % NX;
	const int32 ChannelY = LowestColumnIndex / NX;
	const float ChannelSearchRadius = FMath::Max(2.0f, (NX + NY) * 0.35f);
	const FVector2D NoiseOffset(RandomSeed * 0.01f, RandomSeed * 0.017f);

	// Pass 2: score every column's hydraulic trap potential - flat gravel (slowed current), a
	// synthetic bend/flow-speed proxy, and proximity to this site's channel proxy.
	float TrapScoreSum = 0.0f;
	for (int32 Y = 0; Y < NY; ++Y)
	{
		for (int32 X = 0; X < NX; ++X)
		{
			const int32 ColumnIdx = X + Y * NX;

			const int32 XR = FMath::Min(X + 1, NX - 1);
			const int32 XL = FMath::Max(X - 1, 0);
			const int32 YR = FMath::Min(Y + 1, NY - 1);
			const int32 YL = FMath::Max(Y - 1, 0);
			const float SlopeX = static_cast<float>(SurfaceCellZs[XR + Y * NX] - SurfaceCellZs[XL + Y * NX]);
			const float SlopeY = static_cast<float>(SurfaceCellZs[X + YR * NX] - SurfaceCellZs[X + YL * NX]);
			const float Slope = FMath::Sqrt(SlopeX * SlopeX + SlopeY * SlopeY);
			const float GravelProbability = FMath::Clamp(1.0f - Slope / 4.0f, 0.0f, 1.0f);

			const float BendNoise = (FMath::PerlinNoise2D(FVector2D(X, Y) * MeanderFrequency + NoiseOffset) + 1.0f) * 0.5f;
			const float FlowNoise = (FMath::PerlinNoise2D(FVector2D(X, Y) * (MeanderFrequency * 1.7f) - NoiseOffset) + 1.0f) * 0.5f;

			const float DistToChannel = FVector2D::Distance(FVector2D(X, Y), FVector2D(ChannelX, ChannelY));
			const float ChannelProximity = FMath::Clamp(1.0f - DistToChannel / ChannelSearchRadius, 0.0f, 1.0f);

			const float TrapScore =
				GravelProbabilityWeight * GravelProbability +
				SlowWaterWeight * FlowNoise +
				ShallowWaterWeight * (1.0f - FlowNoise) +
				BankTrapWeight * ChannelProximity +
				BendTrapWeight * BendNoise;

			TrapScores[ColumnIdx] = FMath::Max(0.0f, TrapScore);
			TrapScoreSum += TrapScores[ColumnIdx];
		}
	}

	const float CellVolumeM3 = FMath::Pow(VoxelResolution / 100.0f, 3.0f);

	// Pass 3: lay down the material layers, then mass-conserve gold - each column's share of
	// TotalGoldReserveGrams comes out of its trap score, and within the column it's weighted toward
	// bedrock (heavy gold sinks) but always sums back up to exactly that column's share.
	for (int32 Y = 0; Y < NY; ++Y)
	{
		for (int32 X = 0; X < NX; ++X)
		{
			const int32 ColumnIdx = X + Y * NX;
			const int32 SurfaceCellZ = SurfaceCellZs[ColumnIdx];
			const int32 SandTopCellZ = SurfaceCellZ - TopsoilCells;
			const int32 GravelTopCellZ = SandTopCellZ - SandCells;

			const float ColumnGoldBudgetGrams = TrapScoreSum > 0.0f ? (TrapScores[ColumnIdx] / TrapScoreSum) * TotalGoldReserveGrams : 0.0f;
			const int32 DiggableCellCount = FMath::Max(1, SurfaceCellZ - BedrockCellZ);

			float WeightSum = 0.0f;
			for (int32 Z = BedrockCellZ + 1; Z <= SurfaceCellZ; ++Z)
			{
				const float HeightAboveBedrock01 = FMath::Clamp(1.0f - static_cast<float>(Z - BedrockCellZ) / static_cast<float>(DiggableCellCount), 0.0f, 1.0f);
				WeightSum += 0.3f + 0.7f * HeightAboveBedrock01;
			}

			for (int32 Z = 0; Z < NZ; ++Z)
			{
				EGeoMaterial Mat;
				float Density;

				if (Z <= BedrockCellZ)
				{
					Mat = EGeoMaterial::Bedrock;
					Density = 1.0f;
				}
				else if (Z <= GravelTopCellZ)
				{
					Mat = EGeoMaterial::Gravel;
					Density = 1.0f;
				}
				else if (Z <= SandTopCellZ)
				{
					Mat = EGeoMaterial::Sand;
					Density = 1.0f;
				}
				else if (Z <= SurfaceCellZ)
				{
					Mat = EGeoMaterial::Topsoil;
					Density = 1.0f;
				}
				else
				{
					Mat = EGeoMaterial::Air;
					Density = -1.0f;
				}

				float Gold = 0.0f;
				if (Mat != EGeoMaterial::Air && Mat != EGeoMaterial::Bedrock && WeightSum > 0.0f)
				{
					const float HeightAboveBedrock01 = FMath::Clamp(1.0f - static_cast<float>(Z - BedrockCellZ) / static_cast<float>(DiggableCellCount), 0.0f, 1.0f);
					const float CellWeight = 0.3f + 0.7f * HeightAboveBedrock01;
					const float CellGoldGrams = ColumnGoldBudgetGrams * (CellWeight / WeightSum);
					Gold = CellGoldGrams / CellVolumeM3;
				}

				Chunk->SetPointDuringGeneration(X, Y, Z, Density, Mat, Gold);
			}
		}
	}
}

FSedimentPacket AGravelBarSite::DigSphereAtWorldLocation(FVector WorldLocation, float Radius)
{
	const FVector LocalLoc = GetActorTransform().InverseTransformPosition(WorldLocation);

	if (LocalLoc.X < -Radius || LocalLoc.Y < -Radius || LocalLoc.Z < -Radius ||
		LocalLoc.X > SiteSize.X + Radius || LocalLoc.Y > SiteSize.Y + Radius || LocalLoc.Z > SiteSize.Z + Radius)
	{
		return FSedimentPacket();
	}

	return Chunk->SubtractSphere(LocalLoc, Radius);
}
