// Copyright Epic Games, Inc. All Rights Reserved.

#include "DensityChunkComponent.h"
#include "NavigationSystem.h"

namespace
{
	// 12 edges of a unit cube, as pairs of corner indices. Corner index = dx + dy*2 + dz*4 for
	// dx,dy,dz in {0,1}.
	constexpr int32 CubeEdges[12][2] =
	{
		{0, 1}, {0, 2}, {0, 4}, {1, 3}, {1, 5}, {2, 3},
		{2, 6}, {3, 7}, {4, 5}, {4, 6}, {5, 7}, {6, 7}
	};

	FORCEINLINE FIntVector CornerOffset(int32 CornerIndex)
	{
		return FIntVector(CornerIndex & 1, (CornerIndex >> 1) & 1, (CornerIndex >> 2) & 1);
	}

	FLinearColor GetMaterialTintColor(EGeoMaterial InMaterial)
	{
		switch (InMaterial)
		{
		case EGeoMaterial::Topsoil: return FLinearColor(0.30f, 0.20f, 0.10f);
		case EGeoMaterial::Clay:    return FLinearColor(0.55f, 0.35f, 0.25f);
		case EGeoMaterial::Sand:    return FLinearColor(0.75f, 0.65f, 0.40f);
		case EGeoMaterial::Gravel:  return FLinearColor(0.45f, 0.45f, 0.45f);
		case EGeoMaterial::Bedrock: return FLinearColor(0.25f, 0.25f, 0.28f);
		case EGeoMaterial::Quartz:  return FLinearColor(0.85f, 0.85f, 0.88f);
		default:                    return FLinearColor(1.0f, 1.0f, 1.0f);
		}
	}
}

UDensityChunkComponent::UDensityChunkComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	SetCollisionObjectType(ECC_WorldStatic);
	bUseComplexAsSimpleCollision = true;

	// Built at BeginPlay and rebuilt whenever dug into - the runtime navmesh (Dynamic generation,
	// see DefaultEngine.ini) needs this flagged so click-to-move can path over the gravel bar too.
	SetCanEverAffectNavigation(true);
	bFillCollisionUnderneathForNavmesh = true;
}

void UDensityChunkComponent::InitChunk(const FIntVector& InGridDims, float InVoxelSize)
{
	GridDims = InGridDims;
	VoxelSize = InVoxelSize;

	const int32 Count = GridDims.X * GridDims.Y * GridDims.Z;
	Density.Init(-1.0f, Count);
	Material.Init(EGeoMaterial::Air, Count);
	GoldGramsPerM3.Init(0.0f, Count);
}

void UDensityChunkComponent::SetPointDuringGeneration(int32 X, int32 Y, int32 Z, float InDensity, EGeoMaterial InMaterial, float InGoldGramsPerM3)
{
	if (!InBounds(X, Y, Z))
	{
		return;
	}
	const int32 Idx = PointIndex(X, Y, Z);
	Density[Idx] = InDensity;
	Material[Idx] = InMaterial;
	GoldGramsPerM3[Idx] = InGoldGramsPerM3;
}

float UDensityChunkComponent::GetDensity(int32 X, int32 Y, int32 Z) const
{
	X = FMath::Clamp(X, 0, GridDims.X - 1);
	Y = FMath::Clamp(Y, 0, GridDims.Y - 1);
	Z = FMath::Clamp(Z, 0, GridDims.Z - 1);
	return Density[PointIndex(X, Y, Z)];
}

EGeoMaterial UDensityChunkComponent::GetMaterialAt(int32 X, int32 Y, int32 Z) const
{
	if (!InBounds(X, Y, Z))
	{
		return EGeoMaterial::Air;
	}
	return Material[PointIndex(X, Y, Z)];
}

float UDensityChunkComponent::GetGoldAt(int32 X, int32 Y, int32 Z) const
{
	if (!InBounds(X, Y, Z))
	{
		return 0.0f;
	}
	return GoldGramsPerM3[PointIndex(X, Y, Z)];
}

void UDensityChunkComponent::RebuildMesh()
{
	const FIntVector CellDims = GridDims - FIntVector(1, 1, 1);
	if (CellDims.X <= 0 || CellDims.Y <= 0 || CellDims.Z <= 0)
	{
		return;
	}

	TArray<int32> CellVertexIndex;
	CellVertexIndex.Init(-1, CellDims.X * CellDims.Y * CellDims.Z);
	const auto CellIdx = [&CellDims](int32 X, int32 Y, int32 Z) { return X + Y * CellDims.X + Z * CellDims.X * CellDims.Y; };
	const auto CellValid = [&CellDims](int32 X, int32 Y, int32 Z)
	{
		return X >= 0 && Y >= 0 && Z >= 0 && X < CellDims.X && Y < CellDims.Y && Z < CellDims.Z;
	};

	TArray<FVector> Vertices;
	TArray<FLinearColor> VertexColors;

	// Pass 1: one vertex per surface-crossing cell, at the averaged edge zero-crossing.
	for (int32 CZ = 0; CZ < CellDims.Z; ++CZ)
	{
		for (int32 CY = 0; CY < CellDims.Y; ++CY)
		{
			for (int32 CX = 0; CX < CellDims.X; ++CX)
			{
				float CornerDensity[8];
				bool bAnySolid = false, bAnyAir = false;
				for (int32 C = 0; C < 8; ++C)
				{
					const FIntVector Off = CornerOffset(C);
					CornerDensity[C] = GetDensity(CX + Off.X, CY + Off.Y, CZ + Off.Z);
					if (CornerDensity[C] > 0.0f)
					{
						bAnySolid = true;
					}
					else
					{
						bAnyAir = true;
					}
				}

				if (!bAnySolid || !bAnyAir)
				{
					continue;
				}

				FVector OffsetSum = FVector::ZeroVector;
				int32 CrossingCount = 0;
				for (const auto& Edge : CubeEdges)
				{
					const float DA = CornerDensity[Edge[0]];
					const float DB = CornerDensity[Edge[1]];
					if ((DA > 0.0f) == (DB > 0.0f))
					{
						continue;
					}
					const float T = DA / (DA - DB);
					const FIntVector OffA = CornerOffset(Edge[0]);
					const FIntVector OffB = CornerOffset(Edge[1]);
					OffsetSum += FVector(
						FMath::Lerp(static_cast<float>(OffA.X), static_cast<float>(OffB.X), T),
						FMath::Lerp(static_cast<float>(OffA.Y), static_cast<float>(OffB.Y), T),
						FMath::Lerp(static_cast<float>(OffA.Z), static_cast<float>(OffB.Z), T));
					++CrossingCount;
				}

				if (CrossingCount == 0)
				{
					continue;
				}

				const FVector CellOrigin(static_cast<float>(CX), static_cast<float>(CY), static_cast<float>(CZ));
				const FVector LocalPos = (CellOrigin + OffsetSum / static_cast<float>(CrossingCount)) * VoxelSize;

				// Tint by whichever corner is most solid - the most representative material for this crossing.
				int32 BestCorner = 0;
				for (int32 C = 1; C < 8; ++C)
				{
					if (CornerDensity[C] > CornerDensity[BestCorner])
					{
						BestCorner = C;
					}
				}
				const FIntVector BestOff = CornerOffset(BestCorner);
				const EGeoMaterial CellMaterial = GetMaterialAt(CX + BestOff.X, CY + BestOff.Y, CZ + BestOff.Z);

				CellVertexIndex[CellIdx(CX, CY, CZ)] = Vertices.Num();
				Vertices.Add(LocalPos);
				VertexColors.Add(GetMaterialTintColor(CellMaterial));
			}
		}
	}

	if (Vertices.Num() == 0)
	{
		ClearAllMeshSections();
		return;
	}

	TArray<int32> Triangles;
	TArray<FVector> NormalAccum;
	NormalAccum.Init(FVector::ZeroVector, Vertices.Num());

	const auto AddQuad = [&Triangles](int32 A, int32 B, int32 C, int32 D, bool bFlip)
	{
		if (!bFlip)
		{
			Triangles.Add(A); Triangles.Add(B); Triangles.Add(D);
			Triangles.Add(A); Triangles.Add(D); Triangles.Add(C);
		}
		else
		{
			Triangles.Add(A); Triangles.Add(D); Triangles.Add(B);
			Triangles.Add(A); Triangles.Add(C); Triangles.Add(D);
		}
	};

	// Pass 2: for every interior grid edge that crosses the surface, connect the 4 surrounding cells'
	// vertices into a quad. Only fires when all 4 cells are within the real (non-padded) grid, so the
	// outer perimeter/floor of the volume is left open by design (see class comment).
	for (int32 Z = 0; Z < GridDims.Z; ++Z)
	{
		for (int32 Y = 0; Y < GridDims.Y; ++Y)
		{
			for (int32 X = 0; X < GridDims.X; ++X)
			{
				const float D0 = GetDensity(X, Y, Z);
				const bool bSolid0 = D0 > 0.0f;

				// X-direction edge (X, Y, Z) -> (X+1, Y, Z)
				if (X + 1 < GridDims.X)
				{
					const bool bSolid1 = GetDensity(X + 1, Y, Z) > 0.0f;
					if (bSolid0 != bSolid1 && CellValid(X, Y - 1, Z - 1) && CellValid(X, Y, Z - 1) && CellValid(X, Y - 1, Z) && CellValid(X, Y, Z))
					{
						const int32 A = CellVertexIndex[CellIdx(X, Y - 1, Z - 1)];
						const int32 B = CellVertexIndex[CellIdx(X, Y, Z - 1)];
						const int32 C = CellVertexIndex[CellIdx(X, Y - 1, Z)];
						const int32 D = CellVertexIndex[CellIdx(X, Y, Z)];
						if (A >= 0 && B >= 0 && C >= 0 && D >= 0)
						{
							AddQuad(A, B, D, C, bSolid0);
						}
					}
				}

				// Y-direction edge (X, Y, Z) -> (X, Y+1, Z)
				if (Y + 1 < GridDims.Y)
				{
					const bool bSolid1 = GetDensity(X, Y + 1, Z) > 0.0f;
					if (bSolid0 != bSolid1 && CellValid(X - 1, Y, Z - 1) && CellValid(X, Y, Z - 1) && CellValid(X - 1, Y, Z) && CellValid(X, Y, Z))
					{
						const int32 A = CellVertexIndex[CellIdx(X - 1, Y, Z - 1)];
						const int32 B = CellVertexIndex[CellIdx(X, Y, Z - 1)];
						const int32 C = CellVertexIndex[CellIdx(X - 1, Y, Z)];
						const int32 D = CellVertexIndex[CellIdx(X, Y, Z)];
						if (A >= 0 && B >= 0 && C >= 0 && D >= 0)
						{
							AddQuad(A, B, D, C, !bSolid0);
						}
					}
				}

				// Z-direction edge (X, Y, Z) -> (X, Y, Z+1)
				if (Z + 1 < GridDims.Z)
				{
					const bool bSolid1 = GetDensity(X, Y, Z + 1) > 0.0f;
					if (bSolid0 != bSolid1 && CellValid(X - 1, Y - 1, Z) && CellValid(X, Y - 1, Z) && CellValid(X - 1, Y, Z) && CellValid(X, Y, Z))
					{
						const int32 A = CellVertexIndex[CellIdx(X - 1, Y - 1, Z)];
						const int32 B = CellVertexIndex[CellIdx(X, Y - 1, Z)];
						const int32 C = CellVertexIndex[CellIdx(X - 1, Y, Z)];
						const int32 D = CellVertexIndex[CellIdx(X, Y, Z)];
						if (A >= 0 && B >= 0 && C >= 0 && D >= 0)
						{
							AddQuad(A, B, D, C, bSolid0);
						}
					}
				}
			}
		}
	}

	// Smooth per-vertex normals from accumulated face normals.
	for (int32 T = 0; T < Triangles.Num(); T += 3)
	{
		const FVector& V0 = Vertices[Triangles[T]];
		const FVector& V1 = Vertices[Triangles[T + 1]];
		const FVector& V2 = Vertices[Triangles[T + 2]];
		const FVector FaceNormal = FVector::CrossProduct(V1 - V0, V2 - V0).GetSafeNormal();
		NormalAccum[Triangles[T]] += FaceNormal;
		NormalAccum[Triangles[T + 1]] += FaceNormal;
		NormalAccum[Triangles[T + 2]] += FaceNormal;
	}

	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	Normals.SetNumUninitialized(Vertices.Num());
	UVs.SetNumUninitialized(Vertices.Num());
	for (int32 I = 0; I < Vertices.Num(); ++I)
	{
		Normals[I] = NormalAccum[I].IsNearlyZero() ? FVector::UpVector : NormalAccum[I].GetSafeNormal();
		UVs[I] = FVector2D(Vertices[I].X, Vertices[I].Y) / (VoxelSize * 4.0f);
	}

	CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, VertexColors, TArray<FProcMeshTangent>(), true);

	// This component has no geometry (and therefore empty bounds) until the first RebuildMesh call,
	// so it's silently skipped by the nav octree at registration time. UpdateComponentInNavOctree
	// trusts a cached bNavigationRelevant flag that was already latched false back then, so it just
	// no-ops (or worse, unregisters) instead of re-scanning - UpdateActorAndComponentsInNavOctree
	// re-evaluates IsNavigationRelevant() fresh and actually (re)registers the component.
	if (AActor* Owner = GetOwner())
	{
		UNavigationSystemV1::UpdateActorAndComponentsInNavOctree(*Owner);
	}
}

FSedimentPacket UDensityChunkComponent::SubtractSphere(FVector LocalLocation, float Radius)
{
	FSedimentPacket Result;

	const float PointVolumeM3 = FMath::Pow(VoxelSize / 100.0f, 3.0f);
	const int32 Margin = FMath::CeilToInt(Radius / VoxelSize) + 1;
	const FVector GridCenter = LocalLocation / VoxelSize;
	const FIntVector Center(FMath::RoundToInt(GridCenter.X), FMath::RoundToInt(GridCenter.Y), FMath::RoundToInt(GridCenter.Z));

	bool bChanged = false;

	for (int32 DZ = -Margin; DZ <= Margin; ++DZ)
	{
		for (int32 DY = -Margin; DY <= Margin; ++DY)
		{
			for (int32 DX = -Margin; DX <= Margin; ++DX)
			{
				const int32 X = Center.X + DX;
				const int32 Y = Center.Y + DY;
				const int32 Z = Center.Z + DZ;
				if (!InBounds(X, Y, Z))
				{
					continue;
				}

				const FVector PointLocal = FVector(X, Y, Z) * VoxelSize;
				if (FVector::Dist(PointLocal, LocalLocation) > Radius)
				{
					continue;
				}

				const int32 Idx = PointIndex(X, Y, Z);
				if (Density[Idx] <= 0.0f)
				{
					continue;
				}

				const EGeoMaterial PointMaterial = Material[Idx];
				if (PointMaterial == EGeoMaterial::Bedrock || PointMaterial == EGeoMaterial::Quartz)
				{
					continue;
				}

				const float BulkDensity = GetGeoMaterialBulkDensityKgPerM3(PointMaterial);
				Result.AddMaterial(PointMaterial, BulkDensity * PointVolumeM3);

				const float GoldGrams = GoldGramsPerM3[Idx] * PointVolumeM3;
				if (GoldGrams > 0.0f)
				{
					float Fine, Flake, Nugget;
					SplitGoldByGrainSize(PointMaterial, GoldGrams, Fine, Flake, Nugget);
					Result.FineGoldGrams += Fine;
					Result.FlakeGoldGrams += Flake;
					Result.NuggetGoldGrams += Nugget;
				}

				Density[Idx] = -1.0f;
				GoldGramsPerM3[Idx] = 0.0f;
				bChanged = true;
			}
		}
	}

	if (bChanged)
	{
		RebuildMesh();
	}

	return Result;
}
