// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ComponentVisualizer.h"

class UHeatmap3DCell;

// https://wiki.unrealengine.com/index.php?title=Component_Visualizers
// http://api.unrealengine.com/INT/API/Runtime/Engine/FPrimitiveDrawInterface/DrawMesh/index.html
// http://api.unrealengine.com/INT/API/Runtime/Engine/FMeshBatch/index.html
// https://answers.unrealengine.com/questions/713509/fstaticprimitivedrawinterfacedrawmesh-draws-only-2.html
// https://answers.unrealengine.com/questions/108766/pdi-drawmesh-crash.html
// 

/**
 * 
 */
class YOGURTEDITOR_API FHeatmap3DCellVisualizer : public FComponentVisualizer
{

public:
	FHeatmap3DCellVisualizer();
	virtual ~FHeatmap3DCellVisualizer();

	// Begin FComponentVisualizer interface
	virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
	virtual void OnRegister() override {}
	virtual void DrawVisualizationHUD(const UActorComponent* Component, const FViewport* Viewport, const FSceneView* View, FCanvas* Canvas) {}
	virtual bool IsVisualizingArchetype() const { return false; }
	virtual bool VisProxyHandleClick(FEditorViewportClient* InViewportClient, HComponentVisProxy* VisProxy, const FViewportClick& Click) { return false; }
	virtual void EndEditing() override {}
	virtual bool GetWidgetLocation(const FEditorViewportClient* ViewportClient, FVector& OutLocation) const override { return false; }
	virtual bool GetCustomInputCoordinateSystem(const FEditorViewportClient* ViewportClient, FMatrix& OutMatrix) const override { return false; }
	virtual bool HandleInputDelta(FEditorViewportClient* ViewportClient, FViewport* Viewport, FVector& DeltaTranslate, FRotator& DeltaRotate, FVector& DeltaScale) override { return false; }
	virtual bool HandleInputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event) override { return false; }
	virtual TSharedPtr<SWidget> GenerateContextMenu() const override { return TSharedPtr<SWidget>(); }
	// End FComponentVisualizer interface

private:
	/**Index of target in selected component*/
	//int32 CurrentlySelectedTarget;

	/**Output log commands*/
	//TSharedPtr<FUICommandList> TargetingComponentVisualizerActions;

};
