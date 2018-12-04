// Fill out your copyright notice in the Description page of Project Settings.

#include "DataProcessingWorker.h"

#include "YogurtRuntime.h"

#include "RunnableThread.h"
#include "Paths.h"

#include "BufferArchive.h"
#include "FileHelper.h"
#include "Paths.h"
#include "MemoryReader.h"

#include "Engine/Texture2D.h"
#include "RenderResource.h"
#include "RHIResources.h"
#include "IntPoint.h"
#include "Version.h"

//***********************************************************
//Thread Worker Starts as NULL, prior to being instanced
//	This line is essential! Compiler error without it
FRunnableThread* FDataProcessingWorker::Thread = NULL;
//***********************************************************

FDataProcessingWorker* FDataProcessingWorker::ProcessRead(FString& rootPath, TArray<FString>& subPaths,
	UTextureRenderTarget2D* renderTarget, TSharedPtr<QuadNode> quadMapRoot, FVector2D timeRange)
{
	//Create new instance of thread if it does not exist
	//	and the platform supports multi threading!
	if (Thread == NULL && FPlatformProcess::SupportsMultithreading())
	{
		FDataProcessingWorker* runnable = new FDataProcessingWorker(rootPath, subPaths, renderTarget, quadMapRoot, timeRange);

		Thread = FRunnableThread::Create(runnable, TEXT("FDataProcessingWorker"), 0, TPri_BelowNormal);
		//windows default = 8mb for thread, could specify more
		UE_LOG(LogYogurtRuntime, Warning, TEXT("Created thread: %i"), Thread);

		return runnable;
	}
	return NULL;
}

FDataProcessingWorker* FDataProcessingWorker::ProcessWrite(TSharedPtr<FString> filePath, TArray<DataPoint>& data)
{
	//Create new instance of thread if it does not exist
	//	and the platform supports multi threading!
	if (Thread != NULL)
	{
		UE_LOG(LogYogurtRuntime, Warning, TEXT("Cannot create new thread, thread still running. Ptr: %i"), Thread);
		return NULL;
	}
	if (FPlatformProcess::SupportsMultithreading())
	{
		FDataProcessingWorker* runnable = new FDataProcessingWorker(filePath, data);

		Thread = FRunnableThread::Create(runnable, TEXT("FDataProcessingWorker"), 0, TPri_BelowNormal);
		//windows default = 8mb for thread, could specify more
		//UE_LOG(LogYogurtRuntime, Warning, TEXT("Created thread"));

		return runnable;
	}
	return NULL;
}

FDataProcessingWorker::FDataProcessingWorker(FString& rootPath, TArray<FString>& subPaths,
	UTextureRenderTarget2D* renderTarget, TSharedPtr<QuadNode> quadMapRoot, FVector2D timeRange)
	: threadMode(EDataProcessingMode::Reading)

	, RootFilePath(&rootPath)
	, SubFilePaths(TArray<FString>(subPaths))
	, RenderTarget(renderTarget)
	, mpRootQuadNode(quadMapRoot)
	, mTimeRange(timeRange)

	, Data(TArray<DataPoint>())

	, FilesLoaded(0)
	, DataPointCount(0)
	, DataPointsProcessed(0)
{
}

FDataProcessingWorker::FDataProcessingWorker(TSharedPtr<FString> filePath, TArray<DataPoint>& data)
	: threadMode(EDataProcessingMode::Writing)

	, RootFilePath(nullptr)
	, SubFilePaths(TArray<FString>())

	, WriteFilePath(filePath)
	, WriteFinished(false)

	, FilesLoaded(0)
	, DataPointCount(0)
	, DataPointsProcessed(0)
{
	this->Data = TArray<DataPoint>();
	this->Data.Init(nullptr, data.Num());
	for (int32 i = 0; i < data.Num(); i++)
	{
		this->Data[i] = data[i]->Clone();
	}

}

FDataProcessingWorker::~FDataProcessingWorker()
{
	delete Thread;
	Thread = NULL;
}

//Init
bool FDataProcessingWorker::Init()
{
	//Init the Data

	switch (this->threadMode)
	{
		case EDataProcessingMode::Reading:
			{
				this->FilesToLoad = this->SubFilePaths.Num();
				this->Data.Empty();
			}
			break;
		case EDataProcessingMode::Writing:
			{
				this->WriteFinished = false;
			}
			break;
		default:
			break;
	}

	//UE_LOG(LogYogurtRuntime, Log, TEXT("**********************************"));
	switch (this->threadMode)
	{
		case EDataProcessingMode::Reading:
			//UE_LOG(LogYogurtRuntime, Log, TEXT("Data Processing Thread Started! Reading mode"));
			break;
		case EDataProcessingMode::Writing:
			//UE_LOG(LogYogurtRuntime, Log, TEXT("Data Processing Thread Started! Writing mode"));
			break;
		default:
			break;
	}
	//UE_LOG(LogYogurtRuntime, Log, TEXT("**********************************"));
	
	/*
	if (ThePC)
	{
		ThePC->ClientMessage("**********************************");
		ThePC->ClientMessage("Prime Number Thread Started!");
		ThePC->ClientMessage("**********************************");
	}*/

	return true;
}

//Run
uint32 FDataProcessingWorker::Run()
{
	//Initial wait before starting
	FPlatformProcess::Sleep(0.03);

	switch (this->threadMode)
	{
		case EDataProcessingMode::Reading:
			this->ReadAndProcess();
			break;
		case EDataProcessingMode::Writing:
			this->WriteToBinary(*this->WriteFilePath, this->Data);
			break;
		default:
			break;
	}

	return 0;
}

void FDataProcessingWorker::Stop()
{
}

void FDataProcessingWorker::Exit()
{
	UE_LOG(LogYogurtRuntime, Log, TEXT("Deleting data processing worker"));

	Thread->Kill(false);
	//delete Thread;
	Thread = NULL;
	
	if (this->threadMode == EDataProcessingMode::Reading)
	{
		this->mpRootQuadNode.Reset();
	}
}

void FDataProcessingWorker::ReadAndProcess()
{
	this->ReadFromBinary(this->Data);
	this->RenderSplatToTarget(this->Data);
}

void FDataProcessingWorker::ReadFromBinary(TArray<DataPoint>& dataOut)
{
	while (this->SubFilePaths.Num() > 0)
	{
		FString filePathRelative = this->SubFilePaths.Pop(true);
		FString filePathAbsolute = FPaths::Combine(*(this->RootFilePath), filePathRelative);

		TArray<DataPoint> data;

		this->ReadFromBinary(filePathAbsolute, data);

		this->FilesLoaded++;
		this->DataPointCount += data.Num();

		dataOut.Append(data);

		//***************************************
		//Show Incremental Results in Main Game Thread!
		//	Please note you should not create, destroy,
		// or modify UObjects here.
		//	Do those sort of things after all thread are completed.

		//	All calcs for making stuff can be done in the threads
		//	But the actual making/modifying of the UObjects should
		// be done in main game thread.
		//ThePC->ClientMessage(FString::FromInt(PrimeNumbers->Last()));
		UE_LOG(LogYogurtRuntime, Log, TEXT("Loaded %i data points from data file %s"), data.Num(), *filePathAbsolute);

		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		//prevent thread from using too many resources
		FPlatformProcess::Sleep(0.01);
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	}
}

void FDataProcessingWorker::Serialize(FArchive& archive, UVersion* version, int32& count, TArray<DataPoint>& data)
{
	check(version != nullptr);
	// 0.0.0
	if (!version->IsValid())
	{
		archive << count;

		for (int32 i = 0; i < count; ++i)
		{
			DataPoint point = i < data.Num() ? data[i] : NewObject<UDataPointHeatmap2D>();
			check(point != nullptr);
			point->SerializeData(archive, version);
			if (data.Num() <= i) data.Add(point);
			else data[i] = point;
		}

		return;
	}

	// 0.0.1
	if (version->Major == 0 && version->Minor == 0 && version->Patch == 1)
	{
		archive << count;

		for (int32 i = 0; i < count; ++i)
		{
			DataPoint point = i < data.Num() ? data[i] : NewObject<UDataPointHeatmap2D>();
			check(point != nullptr);
			point->SerializeData(archive, version);
			if (data.Num() <= i) data.Add(point);
			else data[i] = point;
		}

		return;
	}

}

bool FDataProcessingWorker::WriteToBinary(FString filePath, TArray<DataPoint>& data)
{
	// http://runedegroot.com/saving-and-loading-actor-data-in-unreal-engine-4/

	//UE_LOG(LogYogurtRuntime, Warning, TEXT("Saving to %s"), *filePath);
	
	FBufferArchive ToBinary;

	UVersion* version = NewObject<UVersion>();
	version->Patch = 1;
	FString versionStr = version->ToString();
	ToBinary << versionStr;

	int32 count = data.Num();
	this->Serialize(ToBinary, version, count, data);

	//UE_LOG(LogYogurtRuntime, Warning, TEXT("Serialized"));

	if (ToBinary.Num() <= 0) return false;
	
	bool result = FFileHelper::SaveArrayToFile(ToBinary, *filePath);

	//UE_LOG(LogYogurtRuntime, Warning, TEXT("Saved"));

	ToBinary.FlushCache();
	ToBinary.Empty();

	UE_LOG(LogYogurtRuntime, Warning, TEXT("Saved %i data points to %s"), data.Num(), *filePath);

	this->WriteFinished = true;

	return result;
}

void FDataProcessingWorker::ReadFromBinary(FString absolutePath, TArray<DataPoint>& dataOut)
{
	dataOut.Empty();

	FString filePath = absolutePath;

	TArray<uint8> BinaryArray;

	//load disk data to binary array
	if (!FFileHelper::LoadFileToArray(BinaryArray, *filePath) || BinaryArray.Num() <= 0)
	{
		UE_LOG(LogYogurtRuntime, Warning, TEXT("Load Failed! %s"), *filePath);
		return;
	}

	//Memory reader is the archive that we're going to use in order to read the loaded data
	FMemoryReader FromBinary = FMemoryReader(BinaryArray, true);
	FromBinary.Seek(0);

	FString versionStr;
	FromBinary << versionStr;
	UVersion* version = UVersion::Parse(versionStr);
	if (!version->IsValid())
	{
		FromBinary.Seek(0);
	}

	UE_LOG(LogYogurtRuntime, Log, TEXT("%s"), *versionStr);

	int32 count;
	this->Serialize(FromBinary, version, count, dataOut);

	//Empty the buffer's contents
	FromBinary.FlushCache();
	BinaryArray.Empty();
	//Close the stream
	FromBinary.Close();
}

void FDataProcessingWorker::RenderSplatToTarget(TArray<DataPoint>& data)
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

	UE_LOG(LogYogurtRuntime, Log, TEXT("%ix%i (%i)"), size.X, size.Y, size.X * size.Y);

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

void FDataProcessingWorker::CopyPixelsToTexture(FRHITexture2D* texture, TArray<float>& colors)
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

void FDataProcessingWorker::CopyPixelsToTexture(FRHITexture2D* texture, TArray<uint32>& colors)
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		void,
		FRHITexture2D*, InputTexture, texture,
		TArray<uint32>, InputValues, colors,
		{
			// the textures row stride (pitch)
			uint32 DestStride;
			// float array of the color values per pixel
			// also locks the texture across threads
			// note that the lock function returns a void*
			uint32* DestBuffer = (uint32*)RHILockTexture2D(InputTexture, 0, RLM_WriteOnly, DestStride, false, true);
			// copy the pixels into the buffer
			FMemory::Memcpy(DestBuffer, InputValues.GetData(), InputValues.Num());
			// unlock the texture across threads
			RHIUnlockTexture2D(InputTexture, 0, false, true);
		}
	);
	//this->RenderTarget->UpdateResource();
}

void FDataProcessingWorker::RenderDataPointsToSplat(TArray<DataPoint>& data, FIntPoint size, TArray<float>& pixels, float& maxIntensity)
{
	FIntPoint radius, uvCoordCenter;
	DataPoint lastPoint = data.Last();
	double lastTimeStamp = lastPoint->mTimestamp;
	for (DataPoint point : data)
	{
		double timeFrac = point->mTimestamp / lastTimeStamp;
		if (timeFrac < this->mTimeRange.X || timeFrac > this->mTimeRange.Y) continue;

		//UE_LOG(LogYogurtRuntime, Log, TEXT("%.6f, %.6f"), point.data.X, point.data.Y);
		uvCoordCenter = point->mCoordinate;
		//UE_LOG(LogYogurtRuntime, Log, TEXT("%i, %i"), uvCoord.X, uvCoord.Y);

		radius = point->mRadius;

		//UE_LOG(LogYogurtRuntime, Log, TEXT("Point: (%i, %i) Radius: (%i, %i)"),
		//	uvCoordCenter.X, uvCoordCenter.Y, radius.X, radius.Y);

		this->DrawSmoothSplat(size, pixels, maxIntensity, uvCoordCenter, radius, point->mStrength);

		//float intensityTotal = this->AddColor(pixels, size, uvCoordCenter, 1.0f);
		//if (intensityTotal > maxIntensity) maxIntensity = intensityTotal;
	}
}

void FDataProcessingWorker::DrawSmoothSplat(FIntPoint size, TArray<float>& pixels, float& maxIntensity, FIntPoint uvCenter, FIntPoint radius, float strength)
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

void FDataProcessingWorker::DrawSmoothSplat(FIntPoint size, TArray<uint32>& pixels, uint32& maxIntensity, FIntPoint uvCenter, FIntPoint radius, float strength)
{
	FIntPoint offset, uvCoord;
	float distToCenter = 0.0f;
	int32 maxRadius = radius.X * radius.Y;
	for (offset.X = -radius.X; offset.X <= radius.X; ++offset.X)
	{
		for (offset.Y = -radius.Y; offset.Y <= radius.Y; ++offset.Y)
		{
			distToCenter = offset.X * offset.X + offset.Y * offset.Y;
			if (distToCenter > maxRadius)
				continue;

			uvCoord = uvCenter + offset;

			if (uvCoord.X < 0 || uvCoord.Y < 0)
			{
				//UE_LOG(LogYogurtRuntime, Warning, TEXT("Error, negative UV coord (%i, %i)"), uvCoord.X, uvCoord.Y);
				continue;
			}

			float colorIntensity = strength * (1 - FMath::SmoothStep(0.0f, (float)maxRadius, distToCenter));

			float intensityTotal = this->AddColor(pixels, size, uvCoord, (uint32)FMath::TruncToInt(colorIntensity * 255));
			if (intensityTotal > maxIntensity) maxIntensity = intensityTotal;
		}
	}
}
