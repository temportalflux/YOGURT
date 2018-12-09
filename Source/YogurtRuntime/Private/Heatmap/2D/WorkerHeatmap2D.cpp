// Fill out your copyright notice in the Description page of Project Settings.

#include "Heatmap/2D/WorkerHeatmap2D.h"

#include "RunnableThread.h"

#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "RenderResource.h"
#include "RHIResources.h"
#include "IntPoint.h"
#include "Version.h"

#include "QuadPackingSolver.h"

bool FWorkerHeatmap2D::ProcessRead(FString& rootPath, TArray<FString>& subPaths, FVector2D timeRange,
	UTextureRenderTarget2D* renderTarget, TSharedPtr<QuadNode> quadMapRoot)
{
	//Create new instance of thread if it does not exist
	//	and the platform supports multi threading!
	if (ThreadHolder::CanCreate())
	{
		return ThreadHolder::Create(new FWorkerHeatmap2D(rootPath, subPaths, timeRange, renderTarget, quadMapRoot), TEXT("FWorkerHeatmap2D::Read"));
	}
	return false;
}

FWorkerHeatmap2D::FWorkerHeatmap2D(FString& rootPath, TArray<FString>& subPaths, FVector2D timeRange,
	UTextureRenderTarget2D* renderTarget, TSharedPtr<QuadNode> quadMapRoot)
	: FDataProcessingWorker(rootPath, subPaths, timeRange)
	, RenderTarget(renderTarget)
	, mpRootQuadNode(quadMapRoot)
{

}

FWorkerHeatmap2D::FWorkerHeatmap2D(TSharedPtr<FString> filePath, TArray<DataPoint>& data)
	: FDataProcessingWorker(filePath, data)
{

}

void FWorkerHeatmap2D::ReadAndProcess()
{
	FDataProcessingWorker::ReadAndProcess();
	this->RenderSplatToTarget(this->Data);
}

void FWorkerHeatmap2D::RenderSplatToTarget(TArray<UDataPointHeatmap2D*>& data)
{
	//UE_LOG(LogYogurtRuntime, Log, TEXT("Will render %i data points (%i)"), data.Num(), this->DataPointCount);
	//this->DataPointsProcessed = this->DataPointCount;

	if (this->RenderTarget == nullptr)
	{
		UE_LOG(LogYogurtRuntime, Warning, TEXT("RenderTarget is null"));
		return;
	}
	if (this->RenderTarget->Resource == nullptr)
	{
		UE_LOG(LogYogurtRuntime, Warning, TEXT("RenderTarget Resource is null"));
		return;
	}
	if (this->RenderTarget->Resource->TextureRHI == nullptr)
	{
		UE_LOG(LogYogurtRuntime, Warning, TEXT("TextureRHI is null"));
		return;
	}

	FRHITexture2D* renderTexture = this->RenderTarget->Resource->TextureRHI->GetTexture2D();
	FIntPoint size = renderTexture->GetSizeXY();

	UE_LOG(LogYogurtRuntime, Log, TEXT("Texture Target Size: %ix%i (%i)"), size.X, size.Y, size.X * size.Y);

	TArray<float> Pixels;
	//TArray<uint32> Pixels;
	Pixels.Init(0, size.X * size.Y);

	//SetColor(Pixels, size, FIntPoint(0, 0), 1);
	float maxIntensity = 0.0f;
	//uint32 maxIntensity = 0;

	this->RenderDataPointsToSplat(data, size, Pixels, maxIntensity);

	/*
	FIntPoint sizePerSplat = FIntPoint(10, 10);
	FIntPoint spaceBetweenCenterSplats = FIntPoint(20, 20);
	FIntPoint drawPixel = FIntPoint(0, 0);
	for (drawPixel.X = 0; drawPixel.X < size.X / spaceBetweenCenterSplats.X; drawPixel.X++)
	{
	//drawPixel.Y = 0;
	for (drawPixel.Y = 0; drawPixel.Y < size.Y / spaceBetweenCenterSplats.Y; drawPixel.Y++)
	{
	this->DrawSmoothSplat(size, Pixels, maxIntensity,
	FIntPoint(
	drawPixel.X * spaceBetweenCenterSplats.X,
	drawPixel.Y * spaceBetweenCenterSplats.Y
	),
	sizePerSplat,
	0.1f
	);
	}
	}
	//*/

	if (maxIntensity > 0)
	{
		for (int32 iPixel = 0; iPixel < Pixels.Num(); ++iPixel)
		{
			Pixels[iPixel] /= maxIntensity;
		}
	}

	//UE_LOG(LogYogurtRuntime, Log, Text("%.3f"), Pixels);
	/*
	FIntPoint boundMin = FIntPoint(0, 0);
	FIntPoint boundMax = FIntPoint(30, size.Y / 2);
	FIntPoint pixel = FIntPoint(0, 0);
	for (pixel.Y = boundMin.Y; pixel.Y < boundMax.Y; pixel.Y++)
	{
	FString line = FString("");
	for (pixel.X = boundMin.X; pixel.X < boundMax.X; pixel.X++)
	{
	int32 redValue = FMath::TruncToInt(Pixels[pixel.Y * size.X + pixel.X] * 255);
	FString redValueStr = FString::FromInt(redValue);
	if (redValue > 0)
	{
	if (redValue < 10) line.Append("00");
	else if (redValue < 100) line.Append("0");
	line.Append(redValueStr);
	}
	else {
	line.Append("   ");
	}
	line.Append("  ");
	}
	UE_LOG(LogYogurtRuntime, Log, TEXT("%s"), *line);
	}
	//*/

	// Actually creates the GPU resource?
	//NewDataBuffer->RefreshSamplerStates();


	FRHITexture2D* TexParam = this->RenderTarget->Resource->TextureRHI->GetTexture2D();

	this->CopyPixelsToTexture(TexParam, Pixels);

	UE_LOG(LogYogurtRuntime, Log, TEXT("Splat to texture complete"));
}

void FWorkerHeatmap2D::RenderDataPointsToSplat(TArray<UDataPointHeatmap2D*>& data, FIntPoint size, TArray<float>& pixels, float& maxIntensity)
{
	UE_LOG(LogYogurtRuntime, Log, TEXT("Rendering %i data points"), data.Num());
	if (data.Num() <= 0) return;
	UDataPointHeatmap2D* lastPoint = data.Last();
	double lastTimeStamp = lastPoint->mTimestamp;
	//UE_LOG(LogYogurtRuntime, Log, TEXT("%.06f"), lastTimeStamp);
	for (UDataPointHeatmap2D* point : data)
	{
		double timeFrac = point->mTimestamp / lastTimeStamp;
		//UE_LOG(LogYogurtRuntime, Log, TEXT("%.06f"), point->mTimestamp);
		//UE_LOG(LogYogurtRuntime, Log, TEXT("%.06f < %.06f < %.06f"), this->mTimeRange.X, timeFrac, this->mTimeRange.Y);
		if (timeFrac < this->mTimeRange.X || timeFrac > this->mTimeRange.Y) continue;

		//UE_LOG(LogYogurtRuntime, Log, TEXT("(%i, %i)"), point->mCoordinate.X, point->mCoordinate.Y);
		this->DrawSmoothSplat(size, pixels, maxIntensity, point->mCoordinate, point->mRadius, point->mStrength);

		this->DataPointsProcessed++;
	}
}

void FWorkerHeatmap2D::DrawSmoothSplat(FIntPoint size, TArray<float>& pixels, float& maxIntensity, FIntPoint uvCenter, FIntPoint radius, float strength)
{
	QuadNode* nodeForCenter = this->mpRootQuadNode->GetSubnodeAt(uvCenter);
	if (nodeForCenter == nullptr)
	{
		UE_LOG(LogYogurtRuntime, Log, TEXT("No node found for uv (%i, %i)"), uvCenter.X, uvCenter.Y);
		return;
	}
	else
	{
		/*
		UE_LOG(LogYogurtRuntime, Log, TEXT("Found node [%i, %i]->[%i, %i] for uv (%i, %i)"),
		nodeForCenter->mPosition.X,
		nodeForCenter->mPosition.Y,
		nodeForCenter->mPosition.X + nodeForCenter->mFilledSize.X,
		nodeForCenter->mPosition.Y + nodeForCenter->mFilledSize.Y,
		uvCenter.X, uvCenter.Y
		);
		//*/
	}

	FIntPoint offset, uvCoord;
	float distToCenterSq = 0.0f;
	int32 maxRadiusSq = radius.X * radius.Y;
	for (offset.X = -radius.X; offset.X <= radius.X; ++offset.X)
	{
		for (offset.Y = -radius.Y; offset.Y <= radius.Y; ++offset.Y)
		{
			distToCenterSq = offset.X * offset.X + offset.Y * offset.Y;
			if (distToCenterSq > maxRadiusSq)
				continue;

			uvCoord = uvCenter + offset;

			if (uvCoord.X < 0 || uvCoord.Y < 0)
			{
				//UE_LOG(LogYogurtRuntime, Warning, TEXT("Error, negative UV coord (%i, %i)"), uvCoord.X, uvCoord.Y);
				continue;
			}

			if (!nodeForCenter->ContainsPointInFilledArea(uvCoord))
			{
				// uv coord outside the mask for the quad
				//UE_LOG(LogYogurtRuntime, Warning, TEXT("Discarding radius uv (%i, %i). It is outside the quad's masked area."), uvCoord.X, uvCoord.Y);
				continue;
			}

			float colorIntensity = strength * (1 - FMath::SmoothStep(0.0f, (float)maxRadiusSq, distToCenterSq));

			float intensityTotal = this->AddColor(pixels, size, uvCoord, colorIntensity);
			if (intensityTotal > maxIntensity) maxIntensity = intensityTotal;
		}
	}
}

void FWorkerHeatmap2D::CopyPixelsToTexture(FRHITexture2D* texture, TArray<float>& colors)
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		void,
		FRHITexture2D*, InputTexture, texture,
		TArray<float>, InputValues, colors,
		{
			// the textures row stride (pitch)
			uint32 DestStride = 0;
			// float array of the color values per pixel
			// also locks the texture across threads
			// note that the lock function returns a void*
			float* DestBuffer = (float*)RHILockTexture2D(InputTexture, 0, RLM_WriteOnly, DestStride, false, true);
			// copy the pixels into the buffer
			// NOTE TO SELF: The third param is not just a count, but a size -> total num * size of each
			FMemory::Memcpy(DestBuffer, InputValues.GetData(), InputValues.Num() * sizeof(float));
			// unlock the texture across threads
			RHIUnlockTexture2D(InputTexture, 0, false, true);
		}
	);
	//this->RenderTarget->UpdateResource();
}
