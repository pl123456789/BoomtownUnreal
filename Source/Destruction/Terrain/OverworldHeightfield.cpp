// Copyright Epic Games, Inc. All Rights Reserved.

#include "OverworldHeightfield.h"
#include "HeightmapSampler.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "NavigationSystem.h"

AOverworldHeightfield::AOverworldHeightfield()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionObjectType(ECC_WorldStatic);
	MeshComponent->SetMobility(EComponentMobility::Static);

	// Built at BeginPlay, not present in the level at edit time - the runtime navmesh (Dynamic
	// generation, see DefaultEngine.ini) needs this flagged so click-to-move can path over it.
	MeshComponent->SetCanEverAffectNavigation(true);
	MeshComponent->bFillCollisionUnderneathForNavmesh = true;
}

void AOverworldHeightfield::BeginPlay()
{
	Super::BeginPlay();
	BuildMesh();
}

void AOverworldHeightfield::BuildMesh()
{
	const int32 N = Resolution + 1;

	if (!FHeightmapSampler::SampleHeightmap(HeightmapFilePath, N, N, Heights01, SmoothingPasses))
	{
		UE_LOG(LogTemp, Warning, TEXT("AOverworldHeightfield: failed to load heightmap '%s'."), *HeightmapFilePath);
		Heights01.Init(0.0f, N * N);
	}

	const float CellSizeX = WorldSize.X / static_cast<float>(Resolution);
	const float CellSizeY = WorldSize.Y / static_cast<float>(Resolution);

	TArray<FVector> Vertices;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<int32> Triangles;
	Vertices.SetNumUninitialized(N * N);
	Normals.SetNumUninitialized(N * N);
	UVs.SetNumUninitialized(N * N);

	const auto SampleH = [this, N](int32 X, int32 Y)
	{
		X = FMath::Clamp(X, 0, N - 1);
		Y = FMath::Clamp(Y, 0, N - 1);
		return Heights01[X + Y * N];
	};

	for (int32 Y = 0; Y < N; ++Y)
	{
		for (int32 X = 0; X < N; ++X)
		{
			const int32 Idx = X + Y * N;
			const float H01 = Heights01[Idx];
			Vertices[Idx] = FVector(X * CellSizeX, Y * CellSizeY, H01 * ReliefHeight);

			const float DHdx = (SampleH(X + 1, Y) - SampleH(X - 1, Y)) * ReliefHeight / (2.0f * CellSizeX);
			const float DHdy = (SampleH(X, Y + 1) - SampleH(X, Y - 1)) * ReliefHeight / (2.0f * CellSizeY);
			Normals[Idx] = FVector(-DHdx, -DHdy, 1.0f).GetSafeNormal();

			UVs[Idx] = FVector2D(X / static_cast<float>(Resolution), Y / static_cast<float>(Resolution)) * UVTiling;
		}
	}

	Triangles.Reserve(Resolution * Resolution * 6);
	for (int32 Y = 0; Y < Resolution; ++Y)
	{
		for (int32 X = 0; X < Resolution; ++X)
		{
			const int32 Idx00 = X + Y * N;
			const int32 Idx10 = (X + 1) + Y * N;
			const int32 Idx01 = X + (Y + 1) * N;
			const int32 Idx11 = (X + 1) + (Y + 1) * N;

			Triangles.Add(Idx00);
			Triangles.Add(Idx01);
			Triangles.Add(Idx10);

			Triangles.Add(Idx10);
			Triangles.Add(Idx01);
			Triangles.Add(Idx11);
		}
	}

	MeshComponent->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, TArray<FLinearColor>(), TArray<FProcMeshTangent>(), true);

	if (HeightfieldMaterial)
	{
		MeshComponent->SetMaterial(0, HeightfieldMaterial);
	}

	// This component had no geometry (and therefore empty bounds) when it first registered with the
	// navigation octree at actor construction, so it was silently skipped there. UpdateComponentInNavOctree
	// trusts a cached bNavigationRelevant flag that was already latched false back then, so it just
	// no-ops (or worse, unregisters) instead of re-scanning - UpdateActorAndComponentsInNavOctree
	// re-evaluates IsNavigationRelevant() fresh and actually (re)registers the component, which is what
	// gets it into the octree so dynamic navmesh generation can build tiles over it.
	UNavigationSystemV1::UpdateActorAndComponentsInNavOctree(*this);
}

float AOverworldHeightfield::GetSurfaceHeightAtWorldXY(FVector2D WorldXY) const
{
	const int32 N = Resolution + 1;
	if (Heights01.Num() != N * N)
	{
		return GetActorLocation().Z;
	}

	const FVector Local = GetActorTransform().InverseTransformPosition(FVector(WorldXY.X, WorldXY.Y, 0.0f));
	const float CellSizeX = WorldSize.X / static_cast<float>(Resolution);
	const float CellSizeY = WorldSize.Y / static_cast<float>(Resolution);

	const float GridX = FMath::Clamp(Local.X / CellSizeX, 0.0f, static_cast<float>(Resolution));
	const float GridY = FMath::Clamp(Local.Y / CellSizeY, 0.0f, static_cast<float>(Resolution));

	const int32 X0 = FMath::Clamp(FMath::FloorToInt(GridX), 0, Resolution);
	const int32 Y0 = FMath::Clamp(FMath::FloorToInt(GridY), 0, Resolution);
	const int32 X1 = FMath::Min(X0 + 1, Resolution);
	const int32 Y1 = FMath::Min(Y0 + 1, Resolution);
	const float FracX = GridX - X0;
	const float FracY = GridY - Y0;

	const float H00 = Heights01[X0 + Y0 * N];
	const float H10 = Heights01[X1 + Y0 * N];
	const float H01 = Heights01[X0 + Y1 * N];
	const float H11 = Heights01[X1 + Y1 * N];

	const float HTop = FMath::Lerp(H00, H10, FracX);
	const float HBottom = FMath::Lerp(H01, H11, FracX);
	const float H01Interpolated = FMath::Lerp(HTop, HBottom, FracY);

	return GetActorLocation().Z + H01Interpolated * ReliefHeight;
}

FVector2D AOverworldHeightfield::FindLowestPointWorldXY() const
{
	const int32 N = Resolution + 1;
	if (Heights01.Num() != N * N)
	{
		return FVector2D(GetActorLocation().X, GetActorLocation().Y);
	}

	int32 BestIdx = 0;
	float BestHeight = Heights01[0];
	for (int32 I = 1; I < Heights01.Num(); ++I)
	{
		if (Heights01[I] < BestHeight)
		{
			BestHeight = Heights01[I];
			BestIdx = I;
		}
	}

	const int32 BestX = BestIdx % N;
	const int32 BestY = BestIdx / N;
	const float CellSizeX = WorldSize.X / static_cast<float>(Resolution);
	const float CellSizeY = WorldSize.Y / static_cast<float>(Resolution);
	const FVector WorldPos = GetActorTransform().TransformPosition(FVector(BestX * CellSizeX, BestY * CellSizeY, 0.0f));
	return FVector2D(WorldPos.X, WorldPos.Y);
}
