// Fill out your copyright notice in the Description page of Project Settings.

#include "DataPoint.h"
#include "Version.h"

UDataPoint::UDataPoint(FObjectInitializer const & init)
{

}

UDataPoint* UDataPoint::MakeDataPoint(FIntPoint coordinate, FIntPoint radius, float strength)
{
	UDataPoint* point = NewObject<UDataPoint>();
	//point->mTimestamp;
	point->mCoordinate = coordinate;
	point->mRadius = radius;
	point->mStrength = strength;
	return point;
}

UDataPoint* UDataPoint::Clone() const
{
	auto other = NewObject<UDataPoint>();
	this->CopyInto(other);
	return other;
}

void UDataPoint::CopyInto(UDataPoint* other) const
{
	other->mTimestamp = this->mTimestamp;
	other->mCoordinate = this->mCoordinate;
	other->mRadius = this->mRadius;
	other->mStrength = this->mStrength;
}

void UDataPoint::Serialize(FArchive& archive, UVersion* version)
{
	// 0.0.0
	if (!version->IsValid())
	{
		archive << this->mTimestamp;
		archive << this->mCoordinate;
		return;
	}

	// 0.0.1
	if (version->Major == 0 && version->Minor == 0 && version->Patch == 1)
	{
		archive << this->mTimestamp;
		archive << this->mCoordinate;
		archive << this->mRadius;
		archive << this->mStrength;
		return;
	}

}
