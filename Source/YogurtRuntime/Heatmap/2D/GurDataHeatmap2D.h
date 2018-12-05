// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GurData.h"
#include "GurDataHeatmap2D.generated.h"

class UDataPointHeatmap2D;
class QuadPackingSolver;

/**
 * 
 */
UCLASS()
class YOGURTRUNTIME_API AGurDataHeatmap2D : public AGurData
{
	GENERATED_BODY()

private:

	TArray<UDataPointHeatmap2D*> mDataPoints;
	
public:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable)
		UDataPointHeatmap2D* MakeData(FIntPoint coordinate, FIntPoint radius, float strength);

	virtual void Record(UDataPoint* point) override;

protected:

	virtual bool SaveToDisk(TSharedPtr<FString> filePath) override;
	virtual bool ReadFromDisk(FVector2D timeRange) override;

	void BuildRecordingArea(TSharedPtr<QuadPackingSolver> pSolver);

};
