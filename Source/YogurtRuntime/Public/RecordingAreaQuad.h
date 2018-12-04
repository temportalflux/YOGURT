// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RecordingAreaQuad.generated.h"

UCLASS()
class YOGURTRUNTIME_API ARecordingAreaQuad : public AActor
{
	GENERATED_BODY()
	
public:	

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FVector2D SizeRatio;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		float SizeToUvScale;

	// Sets default values for this actor's properties
	ARecordingAreaQuad();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
		FVector2D LocalToUvScaleRatio() const;

	UFUNCTION(BlueprintNativeEvent)
		void SetMeshUvCoordinate(FVector2D uvCoordinate, FVector2D textureSize, UMaterialInstanceDynamic* MaterialRenderToUvTexture);

};
