// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BlockMatch.h"
#include "GameEffect/GameEffectDestroyBlocksBase.h"
#include "GameEffectDestroyRowColAligned.generated.h"


/**
 * This effect destroys either a row or column depending on the orientation of the coords passed.
 * If the coords are vertically aligned it will destroy a column, otherwise it will destroy a row.
 */
UCLASS(BlueprintType, Blueprintable)
class MIXMATCH_API UGameEffectDestroyRowColAligned : public UGameEffectDestroyBlocksBase
{
	GENERATED_BODY()


public:
	// Constructor
	UGameEffectDestroyRowColAligned();

	virtual bool BeginEffect_Implementation(const TArray<FIntPoint>& PerformCoords) override;

};