// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BlockMatch.h"
#include "MatchAction.h"
#include "MMEnums.h"
#include "MatchActionSpawnBlock.generated.h"


/**
 */
UCLASS(BlueprintType, Blueprintable)
class MIXMATCH_API UMatchActionSpawnBlock : public UMatchAction
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EMMBlockLocation LocationType;

public:
	// Constructor
	UMatchActionSpawnBlock();

	virtual bool Perform_Implementation(const UBlockMatch* Match, const FMatchActionType& MatchActionType) override;
};