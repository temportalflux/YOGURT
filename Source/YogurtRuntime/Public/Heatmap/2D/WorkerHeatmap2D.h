// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "DataProcessingWorker.h"
#include "YogurtRuntime.h"
#include "Heatmap/2D/DataPointHeatmap2D.h"
//#include "WorkerHeatmap2D.generated.h"

class UTextureRenderTarget2D;
class QuadNode; // from QuadPackingSolver
class FRHITexture2D;

/**
 * 
 */
class YOGURTRUNTIME_API FWorkerHeatmap2D : public FDataProcessingWorker<UDataPointHeatmap2D>
{

private:

	UTextureRenderTarget2D* RenderTarget;
	// the root node for the BST of quads - represents the texture size node
	TSharedPtr<QuadNode> mpRootQuadNode;

private:

	void RenderSplatToTarget(TArray<UDataPointHeatmap2D*>& data);
	void RenderDataPointsToSplat(TArray<UDataPointHeatmap2D*>& data, FIntPoint size, TArray<float>& pixels, float& maxIntensity);
	void DrawSmoothSplat(FIntPoint size, TArray<float>& pixels, float& maxIntensity, FIntPoint uvCenter, FIntPoint radius, float strength);
	void CopyPixelsToTexture(FRHITexture2D* texture, TArray<float>& colors);

	template<typename T>
	float AddColor(TArray<T>& pixels, FIntPoint size, FIntPoint position, T value)
	{
		int32 index = position.Y * size.Y + position.X;
		if (index >= pixels.Num())
		{
			UE_LOG(LogYogurtRuntime, Warning, TEXT("Coordinate (%i, %i) out of bounds of size (%i, %i) (Len: %i)"),
				position.X, position.Y,
				size.X, size.Y,
				pixels.Num());
			return 0.0f;
		}
		pixels[index] += value;
		return pixels[index];
	}

protected:

	virtual void ReadAndProcess() override;

public:

	static bool ProcessRead(FString& rootPath, TArray<FString>& subPaths, FVector2D timeRange, UTextureRenderTarget2D* renderTarget, TSharedPtr<QuadNode> quadMapRoot);

	FWorkerHeatmap2D(FString& rootPath, TArray<FString>& subPaths, FVector2D timeRange, UTextureRenderTarget2D* renderTarget, TSharedPtr<QuadNode> quadMapRoot);
	FWorkerHeatmap2D(TSharedPtr<FString> filePath, TArray<DataPoint>& data);

};
