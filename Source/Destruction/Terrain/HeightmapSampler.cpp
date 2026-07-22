// Copyright Epic Games, Inc. All Rights Reserved.

#include "HeightmapSampler.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"

bool FHeightmapSampler::SampleHeightmap(const FString& RelativeContentPath, int32 SampleWidth, int32 SampleHeight, TArray<float>& OutHeights01, int32 SmoothingPasses)
{
	const FString FullPath = FPaths::Combine(FPaths::ProjectContentDir(), RelativeContentPath);

	TArray<uint8> FileBytes;
	if (!FFileHelper::LoadFileToArray(FileBytes, *FullPath))
	{
		return false;
	}

	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
	if (!ImageWrapper.IsValid() || !ImageWrapper->SetCompressed(FileBytes.GetData(), FileBytes.Num()))
	{
		return false;
	}

	TArray64<uint8> RawGray;
	if (!ImageWrapper->GetRaw(ERGBFormat::Gray, 8, RawGray))
	{
		return false;
	}

	const int32 ImgWidth = ImageWrapper->GetWidth();
	const int32 ImgHeight = ImageWrapper->GetHeight();
	if (ImgWidth <= 0 || ImgHeight <= 0)
	{
		return false;
	}

	OutHeights01.SetNumUninitialized(SampleWidth * SampleHeight);

	float MinRaw = TNumericLimits<float>::Max();
	float MaxRaw = TNumericLimits<float>::Lowest();

	for (int32 Y = 0; Y < SampleHeight; ++Y)
	{
		const int32 SrcY0 = (Y * ImgHeight) / SampleHeight;
		const int32 SrcY1 = FMath::Max(SrcY0 + 1, ((Y + 1) * ImgHeight) / SampleHeight);

		for (int32 X = 0; X < SampleWidth; ++X)
		{
			const int32 SrcX0 = (X * ImgWidth) / SampleWidth;
			const int32 SrcX1 = FMath::Max(SrcX0 + 1, ((X + 1) * ImgWidth) / SampleWidth);

			int64 Sum = 0;
			int32 Count = 0;
			for (int32 SY = SrcY0; SY < SrcY1; ++SY)
			{
				const int64 RowOffset = static_cast<int64>(SY) * ImgWidth;
				for (int32 SX = SrcX0; SX < SrcX1; ++SX)
				{
					Sum += RawGray[RowOffset + SX];
					++Count;
				}
			}

			const float Avg = Count > 0 ? static_cast<float>(Sum) / static_cast<float>(Count) : 0.0f;
			OutHeights01[X + Y * SampleWidth] = Avg;
			MinRaw = FMath::Min(MinRaw, Avg);
			MaxRaw = FMath::Max(MaxRaw, Avg);
		}
	}

	// Contrast-stretch the sampled patch to [0, 1] so its full relief maps onto the caller's Z range,
	// regardless of the absolute grayscale values in the source DEM image.
	const float Range = FMath::Max(MaxRaw - MinRaw, 1.0f);
	for (float& H : OutHeights01)
	{
		H = FMath::Clamp((H - MinRaw) / Range, 0.0f, 1.0f);
	}

	for (int32 Pass = 0; Pass < SmoothingPasses; ++Pass)
	{
		TArray<float> Blurred;
		Blurred.SetNumUninitialized(OutHeights01.Num());

		for (int32 Y = 0; Y < SampleHeight; ++Y)
		{
			for (int32 X = 0; X < SampleWidth; ++X)
			{
				float Sum = 0.0f;
				int32 Count = 0;
				for (int32 DY = -1; DY <= 1; ++DY)
				{
					const int32 SY = FMath::Clamp(Y + DY, 0, SampleHeight - 1);
					for (int32 DX = -1; DX <= 1; ++DX)
					{
						const int32 SX = FMath::Clamp(X + DX, 0, SampleWidth - 1);
						Sum += OutHeights01[SX + SY * SampleWidth];
						++Count;
					}
				}
				Blurred[X + Y * SampleWidth] = Sum / static_cast<float>(Count);
			}
		}

		OutHeights01 = MoveTemp(Blurred);
	}

	return true;
}
