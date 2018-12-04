// Fill out your copyright notice in the Description page of Project Settings.

#include "GurDataHeatmap2D.h"
#include "Heatmap/2D/DataPointHeatmap2D.h"

UDataPointHeatmap2D* AGurDataHeatmap2D::MakeData(FIntPoint coordinate, FIntPoint radius, float strength)
{
	UDataPointHeatmap2D* point = NewObject<UDataPointHeatmap2D>();
	//point->mTimestamp;
	point->mCoordinate = coordinate;
	point->mRadius = radius;
	point->mStrength = strength;
	return point;
}


