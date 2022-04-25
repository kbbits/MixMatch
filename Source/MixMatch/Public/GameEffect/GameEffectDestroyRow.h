// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BlockMatch.h"
#include "GameEffect/GameEffectDestroyBlocksBase.h"
#include "GameEffectDestroyRow.generated.h"


/**
 * FloatParams[0] : Number of blocks to destroy from list of coords provided.
 * FloatParams[1] : BlockDamage applied to damage-able blocks. Truncated before applied. Defaults to 1.
 */
UCLASS(BlueprintType, Blueprintable)
class MIXMATCH_API UGameEffectDestroyRow : public UGameEffectDestroyBlocksBase
{
	GENERATED_BODY()


public:
	// Constructor
	UGameEffectDestroyRow();

	virtual TArray<FIntPoint> GetEffectedCoords_Implementation(const FIntPoint SelectedCoords) override;

	//virtual bool BeginEffect_Implementation(const TArray<FIntPoint>& PerformCoords) override;

};