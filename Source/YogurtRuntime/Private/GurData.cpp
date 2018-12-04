// Fill out your copyright notice in the Description page of Project Settings.

#include "GurData.h"

#include "YogurtRuntime.h"

#include "CommandLine.h"
#include "Regex.h"

#include "BufferArchive.h"
#include "FileHelper.h"
#include "Paths.h"
#include "MemoryReader.h"

#include "Materials/MaterialInstanceDynamic.h"

#include "DataProcessingWorker.h"
#include "Engine/World.h"
#include "RecordingAreaQuad.h"
#include "EngineUtils.h"

// Sets default values
AGurData::AGurData()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	this->ShouldRecordData = true;
	this->RootDataPath = FPaths::Combine(FPaths::RootDir(), TEXT("Yogurt"));
	this->RecordFilenameFormat = FString("${time|%Y.%m.%d-%H.%M.%S}_${moduleId}.dat");

}

// Called when the game starts or when spawned
void AGurData::BeginPlay()
{
	this->mTimeOnBegin = FDateTime::Now();
	this->mSystemTimeOnBegin = FPlatformTime::Seconds();

	UE_LOG(LogYogurtRuntime, Log, TEXT("Loading options from command line"));
	bool foundShouldRecord = this->GetCommandLineArgBoolDefault("GurShouldRecord", this->ShouldRecordData, this->ShouldRecordData);
	bool foundFolder = this->GetCommandLineArgStrDefault("GurFolder", this->RootDataPath, this->RootDataPath);
	this->GetCommandLineArgStrDefault("GurFilenameFormat", this->RecordFilenameFormat, this->RecordFilenameFormat);
	UE_LOG(LogYogurtRuntime, Log, TEXT("Loaded options: GurShouldRecord:%s GurFolder:%s GurFilenameFormat:%s"), *FString(this->ShouldRecordData ? "true" : "false"), *this->RootDataPath, *this->RecordFilenameFormat);

	Super::BeginPlay();

	TSharedPtr<QuadPackingSolver> pSolver = MakeShareable(new QuadPackingSolver());
	pSolver->SetMaxSize(this->TextureSize);
	this->BuildRecordingArea(pSolver);
	pSolver.Reset();

}

// Called every frame
void AGurData::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (this->dataNeedsToBeSaved)
	{
		auto thread = FDataProcessingWorker::ProcessWrite(this->RootDataPath, this->RecordFilenameFormat, this->mTimeOnBegin, this->ModuleId, this->mDataPoints);
		if (thread != nullptr)
		{
			//UE_LOG(LogYogurtRuntime, Warning, TEXT("Saving %i data points"), this->mDataPoints.Num());
			this->dataNeedsToBeSaved = false;
		}
	}
}

void AGurData::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

bool AGurData::GetCommandLineArgStrDefault(FString key, FString defaultValue, FString& value)
{
	value = defaultValue;
	bool hasValue = FParse::Value(FCommandLine::Get(), *key, value);
	if (hasValue)
	{
		value = value.Replace(TEXT("="), TEXT(""));
		if (value.StartsWith("\""))
		{
			value.RemoveAt(0);
			if (value.EndsWith("\""))
				value.RemoveAt(value.Len() - 1);
		}
	}
	return hasValue;
}

bool AGurData::GetCommandLineArgStr(FString key, FString& value)
{
	return this->GetCommandLineArgStrDefault(key, "", value);
}

bool AGurData::GetCommandLineArgBoolDefault(FString key, bool defaultValue, bool& value)
{
	value = defaultValue;
	FString str;
	if (this->GetCommandLineArgStrDefault(key, defaultValue ? "true" : "false", str))
	{
		value = str.ToBool();
		return true;
	}
	return false;
}

bool AGurData::GetCommandLineArgBool(FString key, bool& value)
{
	return this->GetCommandLineArgBoolDefault(key, false, value);
}

void AGurData::RecordNow(FDataPoint point)
{
	point.timestamp = FPlatformTime::Seconds() - this->mSystemTimeOnBegin;
	this->mDataPoints.Add(point);
}

void AGurData::Save()
{
	this->dataNeedsToBeSaved = true;
	UE_LOG(LogYogurtRuntime, Log, TEXT("Queueing data to be saved"));
}

void AGurData::Load(FVector2D timeRange)
{
	UE_LOG(LogYogurtRuntime, Log, TEXT("Attempting to read from data files"));

	// Construct the meshes
	TSharedPtr<QuadPackingSolver> pSolver = MakeShareable(new QuadPackingSolver());
	pSolver->SetMaxSize(this->TextureSize);

	// Do this first, before threading, to solve the packing data
	this->BuildRecordingArea(pSolver);
	
	//Multi-threading, returns handle that could be cached.
	//	use static function FPrimeNumberWorker::Shutdown() if necessary
	FDataProcessingWorker::ProcessRead(this->RootDataPath, this->FilePaths, this->RenderTarget, pSolver->GetRootNode(), timeRange);

	pSolver.Reset();
}

void AGurData::BuildRecordingArea(TSharedPtr<QuadPackingSolver> pSolver)
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

	recordingAreas.StableSort([](ARecordingAreaQuad& a, ARecordingAreaQuad& b) {
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

