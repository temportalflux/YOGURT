// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DataPoint.generated.h"

class FObjectInitializer;
class UVersion;

/**
 * 
 */
UCLASS(BlueprintType)
class YOGURTRUNTIME_API UDataPoint : public UObject
{
	GENERATED_BODY()

public:

	double mTimestamp;

	UPROPERTY(BlueprintReadWrite)
		FIntPoint mCoordinate;

	UPROPERTY(BlueprintReadWrite)
		FIntPoint mRadius;

	UPROPERTY(BlueprintReadWrite)
		float mStrength;

public:
	UDataPoint(const FObjectInitializer& init = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable)
		static UDataPoint* MakeDataPoint(FIntPoint coordinate, FIntPoint radius, float strength);

	UDataPoint* Clone() const;
	void CopyInto(UDataPoint* other) const;

	void Serialize(FArchive& archive, UVersion* version);

};
