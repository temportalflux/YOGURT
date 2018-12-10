// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Version.generated.h"

/**
 * 
 */
UCLASS()
class YOGURTRUNTIME_API UVersion : public UObject
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "General")
		int32 Major;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "General")
		int32 Minor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "General")
		int32 Patch;

public:

	static UVersion* Parse(FString string);

	UVersion(int32 major, int32 minor, int32 patch)
		: Major(major)
		, Minor(minor)
		, Patch(patch)
	{
	}

	UVersion() : UVersion(0, 0, 0)
	{
	}

	FString ToString();

	bool IsValid();
	
};
