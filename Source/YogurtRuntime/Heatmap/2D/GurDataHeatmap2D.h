// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GurData.h"
#include "GurDataHeatmap2D.generated.h"

class UDataPointHeatmap2D;

/**
 * 
 */
UCLASS()
class YOGURTRUNTIME_API AGurDataHeatmap2D : public AGurData
{
	GENERATED_BODY()
	
public:

	UFUNCTION(BlueprintCallable)
		UDataPointHeatmap2D* MakeData(FIntPoint coordinate, FIntPoint radius, float strength);
	
	
};
