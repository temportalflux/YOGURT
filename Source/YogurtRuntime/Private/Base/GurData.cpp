// Fill out your copyright notice in the Description page of Project Settings.

#include "Base/GurData.h"

#include "YogurtRuntime.h"

#include "CommandLine.h"
#include "Regex.h"

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
}

// Called every frame
void AGurData::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (this->dataNeedsToBeSaved)
	{

		FString filePath = FString(this->RecordFilenameFormat);
		const FRegexPattern patternTime = FRegexPattern(TEXT("\\$\\{time\\|([^\\}]*?)\\}"));
		FRegexMatcher matchTime = FRegexMatcher(patternTime, filePath);
		while (matchTime.FindNext())
		{
			int32 iBegin = matchTime.GetMatchBeginning();
			int32 iEnd = matchTime.GetMatchEnding();
			FString fullMatch = matchTime.GetCaptureGroup(0);
			FString timeFormat = matchTime.GetCaptureGroup(1);
			FString formatted = this->mTimeOnBegin.ToString(*timeFormat);
			filePath = filePath.Replace(*fullMatch, *formatted);
		}
		filePath = filePath.Replace(TEXT("${moduleId}"), *this->ModuleId);
		filePath = FPaths::Combine(this->RootDataPath, filePath);
		
		auto createdThread = this->SaveToDisk(MakeShareable(new FString(filePath)));
		if (createdThread)
		{
			//UE_LOG(LogYogurtRuntime, Warning, TEXT("Saving %i data points"), this->mDataPoints.Num());
			this->dataNeedsToBeSaved = false;
		}
	}
}

bool AGurData::SaveToDisk(TSharedPtr<FString> filePath)
{
	return false;
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

void AGurData::Record(UDataPoint* point)
{
}

void AGurData::RecordNow(UDataPoint* point)
{
	point->mTimestamp = FPlatformTime::Seconds() - this->mSystemTimeOnBegin;
	this->Record(point);
}

void AGurData::Save()
{
	this->dataNeedsToBeSaved = true;
	UE_LOG(LogYogurtRuntime, Log, TEXT("Queueing data to be saved"));
}

void AGurData::Load(FVector2D timeRange)
{
	UE_LOG(LogYogurtRuntime, Log, TEXT("Attempting to read from data files"));

	this->ReadFromDisk(timeRange);
}

bool AGurData::ReadFromDisk(FVector2D timeRange)
{
	return false;
}

