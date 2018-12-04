// Fill out your copyright notice in the Description page of Project Settings.

#include "GurDataHeatmap2D.h"

#include "Heatmap/2D/DataPointHeatmap2D.h"
#include "DataProcessingWorker.h"

#include "RecordingAreaQuad.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"

UDataPointHeatmap2D* AGurDataHeatmap2D::MakeData(FIntPoint coordinate, FIntPoint radius, float strength)
{
	UDataPointHeatmap2D* point = NewObject<UDataPointHeatmap2D>();
	point->mTimestamp = FPlatformTime::Seconds() - this->mSystemTimeOnBegin;
	point->mCoordinate = coordinate;
	point->mRadius = radius;
	point->mStrength = strength;
	return point;
}
void AGurData::BeginPlay()
{
	Super::BeginPlay();

	TSharedPtr<QuadPackingSolver> pSolver = MakeShareable(new QuadPackingSolver());
	pSolver->SetMaxSize(this->TextureSize);
	this->BuildRecordingArea(pSolver);
	pSolver.Reset();
}

void AGurDataHeatmap2D::Record(UDataPoint* point)
{
	UDataPointHeatmap2D* dataPoint = Cast<UDataPointHeatmap2D>(point);
	if (dataPoint != nullptr)
	{
		this->mDataPoints.Add(dataPoint);
	}
}

FDataProcessingWorker* AGurDataHeatmap2D::SaveToDisk(TSharedPtr<FString> filePath)
{
	return FDataProcessingWorker::ProcessWrite(filePath, this->mDataPoints);
}

FDataProcessingWorker* AGurDataHeatmap2D::ReadFromDisk(FVector2D timeRange)
{
	// Construct the meshes
	TSharedPtr<QuadPackingSolver> pSolver = MakeShareable(new QuadPackingSolver());
	pSolver->SetMaxSize(this->TextureSize);

	// Do this first, before threading, to solve the packing data
	this->BuildRecordingArea(pSolver);

	//Multi-threading, returns handle that could be cached.
	//	use static function FPrimeNumberWorker::Shutdown() if necessary
	auto thread = FDataProcessingWorker::ProcessRead(this->RootDataPath, this->FilePaths, this->RenderTarget, pSolver->GetRootNode(), timeRange);

	pSolver.Reset();

	return thread;
}

void AGurDataHeatmap2D::BuildRecordingArea(TSharedPtr<QuadPackingSolver> pSolver)
{

	TArray<ARecordingAreaQuad*> recordingAreas;
	for (TActorIterator<AActor> iter(this->GetWorld(), ARecordingAreaQuad::StaticClass()); iter; ++iter)
	{
		ARecordingAreaQuad* Actor = Cast<ARecordingAreaQuad>(*iter);
		if (Actor && !Actor->IsPendingKill())
		{
			Actor->SizeToUvScale = this->UnitToUvScale;
			recordingAreas.Add(Actor);
		}
	}

	recordingAreas.StableSort([](ARecordingAreaQuad& a, ARecordingAreaQuad& b)
	{
		FVector2D aSize = a.LocalToUvScaleRatio();
		FVector2D bSize = b.LocalToUvScaleRatio();
		// sort by largest in height, then largest in width
		// decreasing order
		if (aSize.Y > bSize.Y) return true;
		else if (bSize.Y > aSize.Y) return false;
		else if (aSize.X > bSize.X) return true;
		else return a.GetName() < b.GetName();
	});

	// for the generated UV texture
	UMaterialInstanceDynamic* MaterialInstanceRenderToUvTexture = nullptr;
	if (this->MaterialRenderToUvTexture)
	{
		MaterialInstanceRenderToUvTexture = UMaterialInstanceDynamic::Create(this->MaterialRenderToUvTexture, this);
	}

	int32 actorsAttemptedPacked = 0;

	for (auto& Actor : recordingAreas)
	{
		if (this->AmountOfActorsToPack >= 0 && actorsAttemptedPacked >= this->AmountOfActorsToPack) break;

		FIntPoint quadSize = Actor->LocalToUvScaleRatio().IntPoint();
		//UE_LOG(LogYogurtRuntime, Log, TEXT("Trying to pack %s, with uv pixel size of (%.3f, %.3f)"), *(Actor->GetName()), quadSize.X, quadSize.Y);

		FIntPoint uvCoordinate;
		if (pSolver->TryPack(quadSize, uvCoordinate))
		{
			UE_LOG(LogYogurtRuntime, Log, TEXT("Packed %s at uv coord (%i, %i)"),
				*(Actor->GetName()), uvCoordinate.X, uvCoordinate.Y);
			// successfully packed the quad, position stored in uvCoordinate
			Actor->SetMeshUvCoordinate(FVector2D(uvCoordinate), this->TextureSize, MaterialInstanceRenderToUvTexture);
		}
		else
		{
			// failed to pack the quad
			UE_LOG(LogYogurtRuntime, Warning, TEXT("Failed to pack quad named %s"), *(Actor->GetName()));
		}

		actorsAttemptedPacked++;
	}

	//this->OnUpdatePackingNodes(pSolver->GetPackingNodes());
}
