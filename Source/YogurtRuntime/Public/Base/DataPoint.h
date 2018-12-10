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

public:
	UDataPoint(const FObjectInitializer& init = FObjectInitializer::Get());

	template<typename TUDataPoint>
	TUDataPoint* Clone()
	{
		return NewObject<TUDataPoint>((UObject*)GetTransientPackage(), NAME_None, RF_NoFlags, this);
	}

	virtual void SerializeData(FArchive& archive, UVersion* version);

};
