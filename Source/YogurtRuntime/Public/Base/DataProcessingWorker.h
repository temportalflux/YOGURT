// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Runnable.h"
#include "RunnableThread.h"

#include "YogurtRuntime.h"

#include "BufferArchive.h"
#include "FileHelper.h"
#include "Paths.h"
#include "MemoryReader.h"

#include "Utility/Version.h"

#include "DataProcessingWorker.generated.h"

class UVersion;

UENUM(BlueprintType)
enum class EDataProcessingMode : uint8
{
	Writing UMETA(DisplayName = "Writing"),
	Reading UMETA(DisplayName = "Reading"),
};

class YOGURTRUNTIME_API ThreadHolder
{

public:
	/** Thread to run the worker FRunnable on */
	static FRunnableThread* Thread;

	static bool CanCreate()
	{
		return ThreadHolder::Thread == NULL;
	}

	static bool Create(FRunnable* runnable, const TCHAR* name)
	{
		//Create new instance of thread if it does not exist
		//	and the platform supports multi threading!
		if (!CanCreate())
		{
			UE_LOG(LogYogurtRuntime, Warning, TEXT("Cannot create new thread, thread still running. Ptr: %i"), ThreadHolder::Thread);
			return false;
		}
		if (FPlatformProcess::SupportsMultithreading())
		{
			ThreadHolder::Thread = FRunnableThread::Create(runnable, name, 0, TPri_BelowNormal);
			//windows default = 8mb for thread, could specify more
			return true;
		}
		return false;
	}

};

template<typename TUDataPoint>
class YOGURTRUNTIME_API FDataProcessingWorker : public FRunnable
{

protected:
	typedef TUDataPoint* DataPoint;
	
	EDataProcessingMode threadMode;

	/** ~~~ Begin: Read Data ~~~ */

	FString* RootFilePath;
	TArray<FString> SubFilePaths;
	FVector2D mTimeRange;

	/** ~~~~~ END: Read Data ~~~ */

	/** ~~~ Begin: Write Data ~~~ */

	bool WriteFinished;
	TSharedPtr<FString> WriteFilePath;

	/** ~~~~~ END: Write Data ~~~ */

	/** The Data */
	TArray<DataPoint> Data;

	uint32 FilesToLoad;
	uint32 FilesLoaded;
	uint64 DataPointCount;
	uint64 DataPointsProcessed;

	void Serialize(FArchive& archive, UVersion* version, int32& count, TArray<DataPoint>& data)
	{
		check(version != nullptr);
		// 0.0.0
		if (!version->IsValid())
		{
			archive << count;

			for (int32 i = 0; i < count; ++i)
			{
				DataPoint point = i < data.Num() ? data[i] : NewObject<TUDataPoint>();
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
				DataPoint point = i < data.Num() ? data[i] : NewObject<TUDataPoint>();
				check(point != nullptr);
				point->SerializeData(archive, version);
				if (data.Num() <= i) data.Add(point);
				else data[i] = point;
			}

			return;
		}

	}


	bool WriteToBinary(FString filePath, TArray<DataPoint>& data)
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


	void ReadFromBinary(TArray<DataPoint>& dataOut)
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

	void ReadFromBinary(FString absolutePath, TArray<DataPoint>& dataOut)
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

protected:

	virtual void ReadAndProcess()
	{
		this->ReadFromBinary(this->Data);
	}

public:

	/* Start the thread and the worker from static (easy access)!
	This code ensures only 1 Prime Number thread will be able to run at a time. This function returns a handle to the newly started instance.
	*/
	static bool ProcessWrite(TSharedPtr<FString> filePath, TArray<DataPoint>& data)
	{
		if (ThreadHolder::CanCreate())
		{
			return ThreadHolder::Create(new FDataProcessingWorker(filePath, data), TEXT("FDataProcessingWorker::Write"));
		}
		return false;
	}

	/** ~~~ Begin: Thread Core Functions ~~~ */

	//Constructor / Destructor
	FDataProcessingWorker(FString& rootPath, TArray<FString>& subPaths, FVector2D timeRange)
		: threadMode(EDataProcessingMode::Reading)

		, RootFilePath(&rootPath)
		, SubFilePaths(TArray<FString>(subPaths))
		, mTimeRange(timeRange)

		, Data(TArray<DataPoint>())

		, FilesLoaded(0)
		, DataPointCount(0)
		, DataPointsProcessed(0)
	{
	}

	FDataProcessingWorker(TSharedPtr<FString> filePath, TArray<DataPoint>& data)
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

	virtual ~FDataProcessingWorker()
	{
		delete ThreadHolder::Thread;
		ThreadHolder::Thread = NULL;
	}

	/** ~~~ Begin: FRunnable ~~~ */
	virtual bool Init()
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

	virtual uint32 Run()
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

	virtual void Stop() {}

	virtual void Exit()
	{
		UE_LOG(LogYogurtRuntime, Log, TEXT("Deleting data processing worker"));

		ThreadHolder::Thread->Kill(false);
		//delete Thread;
		ThreadHolder::Thread = NULL;
	}

	/** ~~~~~ End: FRunnable ~~~ */
	
	/** ~~~~~ End: Thread Core Functions ~~~ */

};
