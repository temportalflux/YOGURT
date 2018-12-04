// Fill out your copyright notice in the Description page of Project Settings.

#include "Version.h"
#include "Regex.h"

UVersion* UVersion::Parse(FString string)
{
	const FRegexPattern pattern = FRegexPattern(TEXT("([0-9]+)\\.*"));
	FRegexMatcher match = FRegexMatcher(pattern, string);

	UVersion* version = NewObject<UVersion>();

	int32* versions[3];
	versions[0] = &(version->Major);
	versions[1] = &(version->Minor);
	versions[2] = &(version->Patch);

	int32 i = 0;

	while (match.FindNext())
	{
		int32 iBegin = match.GetMatchBeginning();
		int32 iEnd = match.GetMatchEnding();
		FString fullMatch = match.GetCaptureGroup(0);
		FString versionStr = match.GetCaptureGroup(1);
		//UE_LOG(LogYogurtRuntime, Log, TEXT("%s %s"), *string, *versionStr);
		*(versions[i]) = FCString::Atoi(*versionStr);
		i++;
	}

	//UE_LOG(LogYogurtRuntime, Log, TEXT("%s"), *(version.ToString()));

	return version;
}

FString UVersion::ToString()
{
	TArray<FStringFormatArg> args = TArray<FStringFormatArg>();
	args.Add(FStringFormatArg(this->Major));
	args.Add(FStringFormatArg(this->Minor));
	args.Add(FStringFormatArg(this->Patch));
	return FString::Format(TEXT("{0}.{1}.{2}"), args);
}

bool UVersion::IsValid()
{
	return Major > 0 || Minor > 0 || Patch > 0;
}


