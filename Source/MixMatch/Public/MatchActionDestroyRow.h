// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BlockMatch.h"
#include "MatchAction.h"
#include "MMEnums.h"
#include "MatchActionDestroyRow.generated.h"


/**
 */
UCLASS(BlueprintType, Blueprintable)
class MIXMATCH_API UMatchActionDestroyRow : public UMatchAction
{
	GENERATED_BODY()

public:

	/** The Perform() method will fill this in so that subclasses can access. */
	UPROPERTY(BlueprintReadOnly)
		TArray<FIntPoint> DestroyedBlockCoords;

	
public:
	// Constructor
	UMatchActionDestroyRow();

	virtual bool Perform_Implementation(const UBlockMatch* Match, const FMatchActionType& MatchActionType, const AMMBlock* TriggeringBlock) override;
};