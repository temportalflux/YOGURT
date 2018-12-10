// Fill out your copyright notice in the Description page of Project Settings.

#include "Base/DataProcessingWorker.h"

//***********************************************************
//Thread Worker Starts as NULL, prior to being instanced
//	This line is essential! Compiler error without it
FRunnableThread* ThreadHolder::Thread = NULL;
//***********************************************************
