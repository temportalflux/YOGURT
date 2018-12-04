// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Runnable.h"
#include "GurData.h"
#include "YogurtRuntime.h"

#include "DataProcessingWorker.generated.h"

class QuadNode; // from QuadPackingSolver

UENUM(BlueprintType)
enum class EDataProcessingMode : uint8
{
	Writing UMETA(DisplayName = "Writing"),
	Reading UMETA(DisplayName = "Reading"),
};

USTRUCT(BlueprintType)
struct FVersion
{
	GENERATED_BODY()

public:

	static FVersion Parse(FString string);

	FVersion(int32 major, int32 minor, int32 patch)
		: Major(major)
		, Minor(minor)
		, Patch(patch)
	{
	}

	FVersion() : FVersion(0, 0, 0)
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "General")
		int32 Major;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "General")
		int32 Minor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "General")
		int32 Patch;

	FString ToString();

	bool IsValid();

};

/**
 * 
 */
class YOGURTRUNTIME_API FDataProcessingWorker : public FRunnable
{

private:

	/** Thread to run the worker FRunnable on */
	static FRunnableThread* Thread;
	
	EDataProcessingMode threadMode;

	/** ~~~ Begin: Read Data ~~~ */

	FString* RootFilePath;
	TArray<FString> SubFilePaths;
	UTextureRenderTarget2D* RenderTarget;
	// the root node for the BST of quads - represents the texture size node
	TSharedPtr<QuadNode> mpRootQuadNode;
	FVector2D mTimeRange;

	/** ~~~~~ END: Read Data ~~~ */

	/** ~~~ Begin: Write Data ~~~ */

	bool WriteFinished;
	FString* WriteFilenameFormat;
	FDateTime* WriteBeginTime;
	FString* WriteModuleId;

	/** ~~~~~ END: Write Data ~~~ */

	/** The Data */
	TArray<FDataPoint> Data;

	uint32 FilesToLoad;
	uint32 FilesLoaded;
	uint64 DataPointCount;
	uint64 DataPointsProcessed;

	void ReadAndProcess();

	void Serialize(FArchive& archive, FVersion& version, int32& count, TArray<FDataPoint>& data);

	bool WriteToBinary(FString rootPath, FString filenameForamt, FDateTime beginTime, FString moduleId, TArray<FDataPoint>& data);
	void ReadFromBinary(TArray<FDataPoint>& dataOut);
	void ReadFromBinary(FString absolutePath, TArray<FDataPoint>& dataOut);
	
	void RenderSplatToTarget(TArray<FDataPoint>& data);
	void RenderDataPointsToSplat(TArray<FDataPoint>& data, FIntPoint size, TArray<float>& pixels, float& maxIntensity);
	void DrawSmoothSplat(FIntPoint size, TArray<float>& pixels, float& maxIntensity, FIntPoint uvCenter, FIntPoint radius, float strength);
	void DrawSmoothSplat(FIntPoint size, TArray<uint32>& pixels, uint32& maxIntensity, FIntPoint uvCenter, FIntPoint radius, float strength);

	void CopyPixelsToTexture(FRHITexture2D* texture, TArray<float>& colors);
	void CopyPixelsToTexture(FRHITexture2D* texture, TArray<uint32>& colors);

	template<typename T>
	float AddColor(TArray<T>& pixels, FIntPoint size, FIntPoint position, T value)
	{
		int32 index = position.Y * size.Y + position.X;
		if (index >= pixels.Num())
		{
			UE_LOG(LogYogurtRuntime, Warning, TEXT("Coordinate (%i, %i) out of bounds of size (%i, %i) (Len: %i)"),
				position.X, position.Y,
				size.X, size.Y,
				pixels.Num());
			return 0.0f;
		}
		pixels[index] += value;
		return pixels[index];
	}

public:

	/* Start the thread and the worker from static (easy access)!
	This code ensures only 1 Prime Number thread will be able to run at a time. This function returns a handle to the newly started instance.
	*/
	static FDataProcessingWorker* ProcessRead(FString& rootPath, TArray<FString>& subPaths, UTextureRenderTarget2D* renderTarget, TSharedPtr<QuadNode> quadMapRoot, FVector2D timeRange);
	static FDataProcessingWorker* ProcessWrite(FString& rootPath, FString& filenameFormat, FDateTime& timeBegin, FString& moduleId, TArray<FDataPoint>& data);

	/** ~~~ Begin: Thread Core Functions ~~~ */

	//Constructor / Destructor
	FDataProcessingWorker(FString& rootPath, TArray<FString>& subPaths, UTextureRenderTarget2D* renderTarget, TSharedPtr<QuadNode> quadMapRoot, FVector2D timeRange);
	FDataProcessingWorker(FString& rootPath, FString& filenameFormat, FDateTime& timeBegin, FString& moduleId, TArray<FDataPoint>& data);
	virtual ~FDataProcessingWorker();

	/** ~~~ Begin: FRunnable ~~~ */
	virtual bool Init();
	virtual uint32 Run();
	virtual void Stop();
	virtual void Exit();
	/** ~~~~~ End: FRunnable ~~~ */
	
	/** ~~~~~ End: Thread Core Functions ~~~ */

};
