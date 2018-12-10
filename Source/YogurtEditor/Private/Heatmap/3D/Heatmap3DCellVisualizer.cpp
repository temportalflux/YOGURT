// Fill out your copyright notice in the Description page of Project Settings.

#include "Heatmap3DCellVisualizer.h"
#include "Heatmap/3D/Heatmap3DCell.h"
#include "GameFramework/Actor.h"
#include "Engine/Public/SceneManagement.h"

FHeatmap3DCellVisualizer::FHeatmap3DCellVisualizer() : FComponentVisualizer()
{
}

FHeatmap3DCellVisualizer::~FHeatmap3DCellVisualizer()
{
}

void FHeatmap3DCellVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	//cast the component into the expected component type
	if (const UHeatmap3DCell* component = Cast<const UHeatmap3DCell>(Component))
	{
		//get colors for selected and unselected targets
		//This is an editor only uproperty of our targeting component, that way we can change the colors if we can't see them against the background
		const FLinearColor SelectedColor = FLinearColor(0.0f, 1.0f, 0.0f, 1.0f);// component->EditorSelectedColor;
		const FLinearColor UnselectedColor = FLinearColor(0.0f, 0.0f, 1.0f, 1.0f);// component->EditorUnselectedColor;

		const FVector Locaction = component->GetOwner()->GetActorLocation();

		//Iterate over each target drawing a line and dot
		/*
		for (int i = 0; i < component->Targets.Num(); i++)
		{
			FLinearColor Color = (i == SelectedTargetIndex) ? SelectedColor : UnselectedColor;

			//Set our hit proxy
			PDI->SetHitProxy(new HTargetProxy(Component, i));
			PDI->DrawLine(Locaction, component->Targets[i], Color, SDPG_Foreground);
			PDI->DrawPoint(component->Targets[i], Color, 20.f, SDPG_Foreground);
			PDI->SetHitProxy(NULL);
		}
		//*/
		PDI->DrawPoint(Locaction, SelectedColor, 20.f, SDPG_Foreground);

	}
}
