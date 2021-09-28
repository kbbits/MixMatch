#pragma once

#include "MMEnums.h"
//#include "MMBlock.h"
#include "BlockMatch.generated.h"

USTRUCT(BlueprintType)
struct FBlockMatch
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	TArray<class AMMBlock*> Blocks;
	
	UPROPERTY(BluePrintReadOnly)
	EMMOrientation Orientation = EMMOrientation::Unknown;

	UPROPERTY(BlueprintReadOnly)
	FIntPoint StartCoords;

	UPROPERTY(BlueprintReadOnly)
	FIntPoint EndCoords;

	bool bSorted = false;

	bool bMatchFinished = false;
};

