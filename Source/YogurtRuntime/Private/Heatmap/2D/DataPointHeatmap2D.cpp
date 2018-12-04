// Fill out your copyright notice in the Description page of Project Settings.

#include "Heatmap/2D/DataPointHeatmap2D.h"
#include "Version.h"

UDataPointHeatmap2D::UDataPointHeatmap2D(FObjectInitializer const & init)
	: UDataPoint(init)
{
	this->mCoordinate = FIntPoint(0, 0);
	this->mRadius = FIntPoint(0, 0);
	this->mStrength = 0;
}

UDataPointHeatmap2D* UDataPointHeatmap2D::Clone()
{
	return UDataPoint::Clone<UDataPointHeatmap2D>();
}

void UDataPointHeatmap2D::SerializeData(FArchive& archive, UVersion* version)
{
	Super::SerializeData(archive, version);

	// 0.0.0
	if (!version->IsValid())
	{
		archive << this->mCoordinate;
		return;
	}

	// 0.0.1
	if (version->Major == 0 && version->Minor == 0 && version->Patch == 1)
	{
		archive << this->mCoordinate;
		archive << this->mRadius;
		archive << this->mStrength;
		return;
	}

}
