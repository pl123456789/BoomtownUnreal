// Copyright Epic Games, Inc. All Rights Reserved.

#include "PrimaryReachHydrology.h"
#include "Components/SplineComponent.h"

namespace
{
	// One authored station on the approved Primary reach: world X, world Y, the corridor elevation
	// the M0014 analysis recorded there, and the corridor width in metres it measured across.
	struct FApprovedCenterlinePoint
	{
		float WorldX;
		float WorldY;

		// Reference geometry only. NOT a validated river-water elevation - see the caveats below
		// before using this for anything that cares about where the water actually sits.
		float CorridorZ;

		float CorridorWidthMetres;
	};

	// The approved Primary tutorial reach, in order, exactly as validated in M0014 (rows 240-375 of
	// the corridor centreline analysis). 136 stations, 1.887 km of centreline.
	//
	// Compiled in rather than loaded from a data file on purpose. This is authored world geometry
	// that Philippe approved; keeping it in source means it is version-controlled with the code that
	// reads it and cannot be lost, and it removes any temptation to rediscover the river from
	// terrain minima at runtime.
	//
	// Two caveats travel with this data, and both are why the hydrology fields above it stay
	// defaulted for now:
	//
	//  - CorridorZ is the mean elevation of the whole masked valley corridor at that station, not
	//    the water surface. Where the corridor is wide it averages in bars, banks and floodplain and
	//    reads high - the corridor widens from ~350 m at the upstream end to ~860 m at the
	//    downstream end, and the profile rises about a metre across the reach as a result. It is
	//    recorded here because it is the approved analysis value, not because it is a water level.
	//
	//  - CorridorWidthMetres is likewise the full masked corridor, not the wetted channel. It is a
	//    useful upper bound on where the river could be, and nothing narrower has been authored yet.
	constexpr FApprovedCenterlinePoint ApprovedCenterline[] =
	{
		{ 532.6f, -280672.8f, -87256.1f, 351.5f },
		{ 532.6f, -279607.7f, -87263.6f, 351.5f },
		{ 532.6f, -278542.5f, -87270.8f, 351.5f },
		{ 1065.2f, -277477.3f, -87260.5f, 362.2f },
		{ 1065.2f, -276412.1f, -87268.7f, 362.2f },
		{ 1065.2f, -275347.0f, -87274.2f, 362.2f },
		{ 1597.8f, -274281.8f, -87261.9f, 372.8f },
		{ 1597.8f, -273216.6f, -87264.5f, 372.8f },
		{ 2662.9f, -272151.4f, -87263.5f, 372.8f },
		{ 3728.1f, -271086.3f, -87264.8f, 372.8f },
		{ 4260.7f, -270021.1f, -87244.0f, 383.5f },
		{ 5325.9f, -268955.9f, -87243.7f, 383.5f },
		{ 5858.4f, -267890.8f, -87256.2f, 372.8f },
		{ 6923.6f, -266825.6f, -87252.1f, 372.8f },
		{ 7988.8f, -265760.4f, -87253.6f, 372.8f },
		{ 9054.0f, -264695.2f, -87246.8f, 372.8f },
		{ 10119.1f, -263630.1f, -87249.4f, 372.8f },
		{ 11716.9f, -262564.9f, -87260.2f, 362.2f },
		{ 12782.1f, -261499.7f, -87258.1f, 362.2f },
		{ 13847.2f, -260434.6f, -87258.5f, 362.2f },
		{ 13847.2f, -259369.4f, -87257.6f, 362.2f },
		{ 14912.4f, -258304.2f, -87262.8f, 362.2f },
		{ 15977.6f, -257239.0f, -87263.8f, 362.2f },
		{ 16510.2f, -256173.9f, -87284.5f, 351.5f },
		{ 17042.8f, -255108.7f, -87272.9f, 362.2f },
		{ 19705.7f, -254043.5f, -87246.3f, 383.5f },
		{ 21037.1f, -252978.4f, -87230.6f, 404.8f },
		{ 22634.9f, -251913.2f, -87226.6f, 415.4f },
		{ 23433.8f, -250848.0f, -87216.5f, 436.7f },
		{ 25031.5f, -249782.8f, -87184.9f, 479.3f },
		{ 25564.1f, -248717.7f, -87215.4f, 468.7f },
		{ 25031.5f, -247652.5f, -87239.4f, 458.0f },
		{ 25564.1f, -246587.3f, -87262.0f, 447.4f },
		{ 25564.1f, -245522.1f, -87266.7f, 447.4f },
		{ 25564.1f, -244457.0f, -87289.1f, 426.1f },
		{ 25564.1f, -243391.8f, -87284.4f, 426.1f },
		{ 26096.7f, -242326.6f, -87286.7f, 415.4f },
		{ 26629.3f, -241261.5f, -87289.8f, 404.8f },
		{ 26629.3f, -240196.3f, -87275.2f, 404.8f },
		{ 27161.9f, -239131.1f, -87275.8f, 394.1f },
		{ 28227.1f, -238065.9f, -87262.2f, 394.1f },
		{ 28759.6f, -237000.8f, -87236.2f, 404.8f },
		{ 29824.8f, -235935.6f, -87229.2f, 404.8f },
		{ 30890.0f, -234870.4f, -87222.4f, 404.8f },
		{ 31422.6f, -233805.3f, -87207.5f, 415.4f },
		{ 31955.2f, -232740.1f, -87215.1f, 404.8f },
		{ 33020.3f, -231674.9f, -87206.6f, 404.8f },
		{ 33020.3f, -230609.7f, -87201.8f, 404.8f },
		{ 33552.9f, -229544.6f, -87212.9f, 394.1f },
		{ 34618.1f, -228479.4f, -87211.8f, 394.1f },
		{ 34618.1f, -227414.2f, -87213.5f, 394.1f },
		{ 35683.3f, -226349.1f, -87216.8f, 394.1f },
		{ 35683.3f, -225283.9f, -87220.9f, 394.1f },
		{ 36748.4f, -224218.7f, -87228.9f, 394.1f },
		{ 37813.6f, -223153.5f, -87237.7f, 394.1f },
		{ 38346.2f, -222088.4f, -87257.6f, 383.5f },
		{ 39411.4f, -221023.2f, -87265.3f, 383.5f },
		{ 39411.4f, -219958.0f, -87269.8f, 383.5f },
		{ 40476.5f, -218892.8f, -87276.1f, 383.5f },
		{ 41009.1f, -217827.7f, -87266.0f, 394.1f },
		{ 42074.3f, -216762.5f, -87274.0f, 394.1f },
		{ 42606.9f, -215697.3f, -87268.3f, 404.8f },
		{ 43672.1f, -214632.2f, -87276.8f, 404.8f },
		{ 44737.2f, -213567.0f, -87258.0f, 426.1f },
		{ 44737.2f, -212501.8f, -87258.4f, 426.1f },
		{ 45269.8f, -211436.6f, -87272.8f, 415.4f },
		{ 46335.0f, -210371.5f, -87270.8f, 415.4f },
		{ 46335.0f, -209306.3f, -87269.6f, 415.4f },
		{ 46867.6f, -208241.1f, -87282.8f, 404.8f },
		{ 46867.6f, -207176.0f, -87277.0f, 404.8f },
		{ 47400.2f, -206110.8f, -87288.1f, 394.1f },
		{ 48465.3f, -205045.6f, -87281.2f, 394.1f },
		{ 48465.3f, -203980.4f, -87275.8f, 394.1f },
		{ 48997.9f, -202915.3f, -87287.6f, 383.5f },
		{ 50063.1f, -201850.1f, -87284.8f, 383.5f },
		{ 50595.7f, -200784.9f, -87268.6f, 394.1f },
		{ 51128.3f, -199719.8f, -87289.7f, 383.5f },
		{ 51660.8f, -198654.6f, -87280.0f, 394.1f },
		{ 52726.0f, -197589.4f, -87284.2f, 394.1f },
		{ 53258.6f, -196524.2f, -87271.5f, 404.8f },
		{ 53791.2f, -195459.1f, -87289.3f, 394.1f },
		{ 53791.2f, -194393.9f, -87289.0f, 394.1f },
		{ 54323.8f, -193328.7f, -87272.9f, 404.8f },
		{ 54856.4f, -192263.5f, -87287.9f, 394.1f },
		{ 55388.9f, -191198.4f, -87272.5f, 404.8f },
		{ 55388.9f, -190133.2f, -87272.6f, 404.8f },
		{ 56454.1f, -189068.0f, -87273.7f, 404.8f },
		{ 56454.1f, -188002.9f, -87274.1f, 404.8f },
		{ 56454.1f, -186937.7f, -87271.8f, 404.8f },
		{ 57519.3f, -185872.5f, -87272.1f, 404.8f },
		{ 57519.3f, -184807.3f, -87274.6f, 404.8f },
		{ 58051.9f, -183742.2f, -87262.9f, 415.4f },
		{ 58584.5f, -182677.0f, -87281.5f, 404.8f },
		{ 59117.0f, -181611.8f, -87268.9f, 415.4f },
		{ 59117.0f, -180546.7f, -87270.0f, 415.4f },
		{ 58584.5f, -179481.5f, -87283.3f, 404.8f },
		{ 58584.5f, -178416.3f, -87280.8f, 404.8f },
		{ 59117.0f, -177351.1f, -87292.2f, 394.1f },
		{ 59117.0f, -176286.0f, -87287.0f, 394.1f },
		{ 59117.0f, -175220.8f, -87283.1f, 394.1f },
		{ 59117.0f, -174155.6f, -87279.9f, 394.1f },
		{ 59117.0f, -173090.5f, -87279.6f, 394.1f },
		{ 59649.6f, -172025.3f, -87269.2f, 404.8f },
		{ 59649.6f, -170960.1f, -87276.9f, 404.8f },
		{ 60182.2f, -169894.9f, -87274.2f, 415.4f },
		{ 60714.8f, -168829.8f, -87274.4f, 426.1f },
		{ 60714.8f, -167764.6f, -87289.7f, 426.1f },
		{ 60714.8f, -166699.4f, -87305.2f, 426.1f },
		{ 61247.4f, -165634.2f, -87302.7f, 436.7f },
		{ 61247.4f, -164569.1f, -87309.6f, 436.7f },
		{ 61247.4f, -163503.9f, -87315.7f, 436.7f },
		{ 61247.4f, -162438.7f, -87317.6f, 436.7f },
		{ 61247.4f, -161373.6f, -87318.8f, 436.7f },
		{ 60182.2f, -160308.4f, -87319.1f, 436.7f },
		{ 60182.2f, -159243.2f, -87319.6f, 436.7f },
		{ 60182.2f, -158178.0f, -87319.3f, 436.7f },
		{ 59649.6f, -157112.9f, -87302.2f, 447.4f },
		{ 59117.0f, -156047.7f, -87316.4f, 436.7f },
		{ 58584.5f, -154982.5f, -87299.1f, 447.4f },
		{ 58051.9f, -153917.4f, -87313.4f, 436.7f },
		{ 57519.3f, -152852.2f, -87298.3f, 447.4f },
		{ 56454.1f, -151787.0f, -87298.6f, 447.4f },
		{ 55388.9f, -150721.8f, -87273.9f, 468.7f },
		{ 53791.2f, -149656.7f, -87267.3f, 479.3f },
		{ 51660.8f, -148591.5f, -87258.6f, 500.6f },
		{ 50063.1f, -147526.3f, -87238.6f, 532.6f },
		{ 44204.6f, -146461.2f, -87181.6f, 628.5f },
		{ 43139.5f, -145396.0f, -87200.2f, 649.8f },
		{ 41541.7f, -144330.8f, -87224.1f, 660.4f },
		{ 41009.1f, -143265.6f, -87242.4f, 671.1f },
		{ 39411.4f, -142200.5f, -87238.1f, 681.7f },
		{ 37813.6f, -141135.3f, -87221.4f, 713.7f },
		{ 34618.1f, -140070.1f, -87197.2f, 756.3f },
		{ 31156.3f, -139004.9f, -87179.5f, 798.9f },
		{ 27428.2f, -137939.8f, -87155.8f, 852.1f },
		{ 26096.7f, -136874.6f, -87158.2f, 862.8f },
	};

	constexpr int32 ApprovedCenterlineNum = UE_ARRAY_COUNT(ApprovedCenterline);
}

APrimaryReachHydrology::APrimaryReachHydrology()
{
	PrimaryActorTick.bCanEverTick = false;

	Centerline = CreateDefaultSubobject<USplineComponent>(TEXT("Centerline"));
	SetRootComponent(Centerline);

	// The reach is authored in absolute world coordinates, so the spline must not inherit any
	// transform the actor happens to be placed with - otherwise nudging the actor in the outliner
	// would silently move the river.
	Centerline->SetAbsolute(true, true, true);
	Centerline->bDrawDebug = true;
}

void APrimaryReachHydrology::RebuildFromApprovedCenterline()
{
	if (!Centerline)
	{
		return;
	}

	Centerline->ClearSplinePoints(false);

	// The spline's own Z carries CorridorZ purely so the centreline sits somewhere sensible in the
	// viewport while authoring. Treat it as reference geometry: it is a valley-floor mean, not a
	// water level, and nothing should read a water surface off this spline. That value belongs in
	// FReachHydrologySample::WaterSurfaceElevation, which stays unset until it is established.
	for (int32 Index = 0; Index < ApprovedCenterlineNum; ++Index)
	{
		const FApprovedCenterlinePoint& Point = ApprovedCenterline[Index];
		Centerline->AddSplinePoint(
			FVector(Point.WorldX, Point.WorldY, Point.CorridorZ),
			ESplineCoordinateSpace::World,
			false);
	}

	// Linear rather than curved: these stations came from a raster trace roughly 10 m apart, so a
	// curve fit through them would invent meander detail the source data never contained.
	for (int32 Index = 0; Index < ApprovedCenterlineNum; ++Index)
	{
		Centerline->SetSplinePointType(Index, ESplinePointType::Linear, false);
	}

	Centerline->UpdateSpline();

	// Rebuild the sample records index-aligned with the spline. Only the fields that follow directly
	// from the approved geometry are filled; everything hydrological stays at its default until it
	// is established from something that can actually resolve it.
	Samples.Reset(ApprovedCenterlineNum);
	for (int32 Index = 0; Index < ApprovedCenterlineNum; ++Index)
	{
		FReachHydrologySample Sample;
		Sample.DownstreamDistance = Centerline->GetDistanceAlongSplineAtSplinePoint(Index);

		// The corridor half-width is the widest the channel could be here, not the channel itself.
		// Recorded symmetrically because nothing has yet established which bank the deep water runs
		// against; splitting it properly is bank-classification work.
		const float CorridorHalfWidthCm = ApprovedCenterline[Index].CorridorWidthMetres * 100.0f * 0.5f;
		Sample.LeftWidth = CorridorHalfWidthCm;
		Sample.RightWidth = CorridorHalfWidthCm;

		Samples.Add(Sample);
	}
}

float APrimaryReachHydrology::GetReachLength() const
{
	return Centerline ? Centerline->GetSplineLength() : 0.0f;
}

const FReachHydrologySample* APrimaryReachHydrology::FindNearestSample(const FVector& WorldLocation) const
{
	if (!Centerline || Samples.Num() == 0)
	{
		return nullptr;
	}

	// Nearest by distance along the spline rather than nearest point in space: two stations on
	// opposite sides of a tight bend can be close together in world space while being a long way
	// apart along the river, and downstream order is what every consumer of this data cares about.
	const float InputKey = Centerline->FindInputKeyClosestToWorldLocation(WorldLocation);
	const int32 Index = FMath::Clamp(FMath::RoundToInt(InputKey), 0, Samples.Num() - 1);
	return &Samples[Index];
}
