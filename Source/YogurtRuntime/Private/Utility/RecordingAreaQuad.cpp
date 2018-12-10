// Fill out your copyright notice in the Description page of Project Settings.

#include "Utility/RecordingAreaQuad.h"


// Sets default values
ARecordingAreaQuad::ARecordingAreaQuad()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ARecordingAreaQuad::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ARecordingAreaQuad::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

FVector2D ARecordingAreaQuad::LocalToUvScaleRatio() const
{
	return this->SizeRatio * this->SizeToUvScale;
}

void ARecordingAreaQuad::SetMeshUvCoordinate_Implementation(FVector2D uvCoordinate, FVector2D textureSize, UMaterialInstanceDynamic* MaterialRenderToUvTexture)
{

}

