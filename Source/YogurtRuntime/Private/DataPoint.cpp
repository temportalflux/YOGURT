// Fill out your copyright notice in the Description page of Project Settings.

#include "DataPoint.h"
#include "Version.h"

UDataPoint::UDataPoint(FObjectInitializer const & init)
{
	this->mTimestamp = 0;
}

void UDataPoint::SerializeData(FArchive& archive, UVersion* version)
{
	// 0.0.0
	if (!version->IsValid())
	{
		archive << this->mTimestamp;
		return;
	}

	// 0.0.1
	if (version->Major == 0 && version->Minor == 0 && version->Patch == 1)
	{
		archive << this->mTimestamp;
		return;
	}

}
