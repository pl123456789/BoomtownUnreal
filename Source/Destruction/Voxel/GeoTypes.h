// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GeoTypes.generated.h"

UENUM(BlueprintType)
enum class EGeoMaterial : uint8
{
	Air		UMETA(DisplayName = "Air"),
	Topsoil	UMETA(DisplayName = "Topsoil"),
	Clay	UMETA(DisplayName = "Clay"),
	Sand	UMETA(DisplayName = "Sand"),
	Gravel	UMETA(DisplayName = "Gravel"),
	Bedrock	UMETA(DisplayName = "Bedrock"),
	Quartz	UMETA(DisplayName = "Quartz")
};

// Bulk density of each loose material, in kg per cubic metre. Used to convert a removed volume of
// the density field into a real mass for FSedimentPacket. Bedrock/Quartz are the hard floor and
// veins - not diggable as loose sediment in this pass, but keep a value for future hard-rock work.
FORCEINLINE float GetGeoMaterialBulkDensityKgPerM3(EGeoMaterial Material)
{
	switch (Material)
	{
	case EGeoMaterial::Topsoil: return 1200.0f;
	case EGeoMaterial::Clay:    return 1700.0f;
	case EGeoMaterial::Sand:    return 1600.0f;
	case EGeoMaterial::Gravel:  return 1800.0f;
	case EGeoMaterial::Bedrock: return 2700.0f;
	case EGeoMaterial::Quartz:  return 2650.0f;
	default:                   return 0.0f;
	}
}

// A real, composed mass of dug-up material: how much of each sediment type, and how much gold in
// each grain-size class. This is what actually moves between the ground, the shovel, the bucket,
// and the pan - gold is never a generic "ore" count, it is grams of a specific size class that
// behaves differently when washed.
USTRUCT(BlueprintType)
struct FSedimentPacket
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Sediment")
	float TopsoilKg = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Sediment")
	float ClayKg = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Sediment")
	float SandKg = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Sediment")
	float GravelKg = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Sediment")
	float WaterLitres = 0.0f;

	// Washes out almost immediately - easiest to lose, first to concentrate.
	UPROPERTY(BlueprintReadOnly, Category = "Sediment")
	float FineGoldGrams = 0.0f;

	// Washes out with moderate agitation.
	UPROPERTY(BlueprintReadOnly, Category = "Sediment")
	float FlakeGoldGrams = 0.0f;

	// Dense and large enough that only serious overtilting loses it.
	UPROPERTY(BlueprintReadOnly, Category = "Sediment")
	float NuggetGoldGrams = 0.0f;

	float TotalMassKg() const
	{
		return TopsoilKg + ClayKg + SandKg + GravelKg;
	}

	float TotalGoldGrams() const
	{
		return FineGoldGrams + FlakeGoldGrams + NuggetGoldGrams;
	}

	FSedimentPacket& operator+=(const FSedimentPacket& Other)
	{
		TopsoilKg += Other.TopsoilKg;
		ClayKg += Other.ClayKg;
		SandKg += Other.SandKg;
		GravelKg += Other.GravelKg;
		WaterLitres += Other.WaterLitres;
		FineGoldGrams += Other.FineGoldGrams;
		FlakeGoldGrams += Other.FlakeGoldGrams;
		NuggetGoldGrams += Other.NuggetGoldGrams;
		return *this;
	}

	void AddMaterial(EGeoMaterial Material, float Kg)
	{
		switch (Material)
		{
		case EGeoMaterial::Topsoil: TopsoilKg += Kg; break;
		case EGeoMaterial::Clay:    ClayKg += Kg; break;
		case EGeoMaterial::Sand:    SandKg += Kg; break;
		case EGeoMaterial::Gravel:  GravelKg += Kg; break;
		default: break;
		}
	}
};

// Gold doesn't sit in the ground as a single uniform grain size - what layer it came out of biases
// how coarse it is. Gravel (heavy material that settles with heavy gold) skews toward flakes/nuggets;
// fine sand and topsoil skew toward fine gold. Approximates the doc's "geological history" placement
// without simulating actual erosion/transport for this first pass.
FORCEINLINE void SplitGoldByGrainSize(EGeoMaterial Material, float TotalGrams, float& OutFineGrams, float& OutFlakeGrams, float& OutNuggetGrams)
{
	float FineFrac = 0.9f, FlakeFrac = 0.1f, NuggetFrac = 0.0f;
	switch (Material)
	{
	case EGeoMaterial::Gravel:
		FineFrac = 0.2f; FlakeFrac = 0.5f; NuggetFrac = 0.3f;
		break;
	case EGeoMaterial::Sand:
		FineFrac = 0.6f; FlakeFrac = 0.35f; NuggetFrac = 0.05f;
		break;
	default:
		break;
	}
	OutFineGrams = TotalGrams * FineFrac;
	OutFlakeGrams = TotalGrams * FlakeFrac;
	OutNuggetGrams = TotalGrams * NuggetFrac;
}
