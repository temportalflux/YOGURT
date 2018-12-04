// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "QuadPackingSolver.generated.h"

USTRUCT(BlueprintType)
struct YOGURTRUNTIME_API FPackingNode
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadOnly)
		FVector2D mPosition;

	UPROPERTY(BlueprintReadOnly)
		FVector2D mSize;
};

class YOGURTRUNTIME_API QuadNode
{
public:
	QuadNode();
	~QuadNode();
	FIntPoint mPosition;
	FIntPoint mSize;
	bool mIsFilled;
	FIntPoint mFilledSize;
	QuadNode* mpChildRight;
	QuadNode* mpChildBelow;

	bool PackQuad(FIntPoint quad, FIntPoint &uvCoord);
	QuadNode* GetSubnodeAt(FIntPoint position);
	bool ContainsPoint(FIntPoint position);
	bool ContainsPointInFilledArea(FIntPoint position);
	bool ContainsPointInArea(FIntPoint position, FIntPoint size);

};

/**
 * 
 */
class YOGURTRUNTIME_API QuadPackingSolver
{

private:

	FVector2D mMaxSize;
	TSharedPtr<QuadNode> mpRootNode;

public:
	QuadPackingSolver();
	~QuadPackingSolver();

	void SetMaxSize(FIntPoint size);

	bool TryPack(FIntPoint quad, FIntPoint &successfulUvCoordinate);

	TSharedPtr<QuadNode> GetRootNode();

};
