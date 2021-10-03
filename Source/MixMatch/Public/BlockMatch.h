#pragma once

#include "MMEnums.h"
#include "BlockMatch.generated.h"

UCLASS(BlueprintType, Blueprintable)
class MIXMATCH_API UBlockMatch : public UObject
{
	GENERATED_BODY()

public:

	UBlockMatch();

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	TArray<class AMMBlock*> Blocks;
	
	UPROPERTY(BluePrintReadOnly)
	EMMOrientation Orientation = EMMOrientation::Unknown;

	UPROPERTY(BlueprintReadOnly)
	FIntPoint StartCoords;

	UPROPERTY(BlueprintReadOnly)
	FIntPoint EndCoords;

	UPROPERTY(BlueprintReadOnly)
	bool bSorted = false;

	UPROPERTY(BlueprintReadOnly)
	bool bMatchFinished = false;

	/** FUNCTIONS **/

public:

	UFUNCTION(BlueprintCallable)
	void Sort(const bool bForceSort = false);
};

