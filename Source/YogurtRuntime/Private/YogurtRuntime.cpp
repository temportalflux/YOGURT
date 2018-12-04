// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "YogurtRuntime.h"

DEFINE_LOG_CATEGORY(LogYogurtRuntime);

#define LOCTEXT_NAMESPACE "FYogurtRuntimeModule"

void FYogurtRuntimeModule::StartupModule()
{
	UE_LOG(LogYogurtRuntime, Log, TEXT("Starting Yogurt:Runtime"));

	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FYogurtRuntimeModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FYogurtRuntimeModule, YogurtRuntime)