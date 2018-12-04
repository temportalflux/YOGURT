// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "DataPoint.h"
#include "DataPointHeatmap2D.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class YOGURTRUNTIME_API UDataPointHeatmap2D : public UDataPoint
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite)
		FIntPoint mCoordinate;

	UPROPERTY(BlueprintReadWrite)
		FIntPoint mRadius;

	UPROPERTY(BlueprintReadWrite)
		float mStrength;
	
public:
	UDataPointHeatmap2D(const FObjectInitializer& init = FObjectInitializer::Get());
	
	virtual void SerializeData(FArchive& archive, UVersion* version) override;

	UDataPointHeatmap2D* Clone();

};
