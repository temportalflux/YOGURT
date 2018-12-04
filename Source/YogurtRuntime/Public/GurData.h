// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "QuadPackingSolver.h"
#include "DataPoint.h"

#include "GurData.generated.h"

class FDataProcessingWorker;

UCLASS()
class YOGURTRUNTIME_API AGurData : public AActor
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data|General")
		FString ModuleId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data|General")
		FString RootDataPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data|Write")
		bool ShouldRecordData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data|Write")
		FString RecordFilenameFormat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data|Read")
		UTextureRenderTarget2D* RenderTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data|Read")
		TArray<FString> FilePaths;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data|Render")
		FIntPoint TextureSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data|Render")
		float UnitToUvScale = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data|Render")
		int32 AmountOfActorsToPack = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data|Render", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
		float TimeBegin = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data|Render", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
		float TimeEnd = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data|Render")
		UMaterialInterface* MaterialRenderToUvTexture;

	bool dataNeedsToBeSaved;

protected:

	FDateTime mTimeOnBegin;
	double mSystemTimeOnBegin;

public:
	// Sets default values for this actor's properties
	AGurData();

	UFUNCTION(BlueprintCallable)
		bool GetCommandLineArgStrDefault(FString key, FString defaultValue, FString& value);

	UFUNCTION(BlueprintCallable)
		bool GetCommandLineArgStr(FString key, FString& value);

	UFUNCTION(BlueprintCallable)
		bool GetCommandLineArgBoolDefault(FString key, bool defaultValue, bool& value);

	UFUNCTION(BlueprintCallable)
		bool GetCommandLineArgBool(FString key, bool& value);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual FDataProcessingWorker* SaveToDisk(TSharedPtr<FString> filePath);
	virtual FDataProcessingWorker* ReadFromDisk(FVector2D timeRange);

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
		virtual void Record(UDataPoint* point);

	UFUNCTION(BlueprintCallable)
		void RecordNow(UDataPoint* point);

	UFUNCTION(BlueprintCallable)
		void Save();

	UFUNCTION(BlueprintCallable)
		void Load(FVector2D timeRange);

};
