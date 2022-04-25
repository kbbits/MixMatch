// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BlockMatch.h"
#include "GameEffect/GameEffectDestroyBlocksBase.h"
#include "GameEffectDestroyArea.generated.h"


/**
 * Destroys block at submitted coords and all destructible blocks in an area around it.
 * Default distance values (of 1) result in a 3x3 set of blocks destroyed, with submitted coord in the center.
 * 
 * FloatParams[0] : X distance around coord to destroy blocks. Defaults to 1.
 * FloatParams[1] : Y distance around coord to destroy blocks. Defaults to 1.
 * FloatParams[2] : BlockDamage applied to damage-able blocks. Truncated before applied. Defaults to 1.
 */
UCLASS(BlueprintType, Blueprintable)
class MIXMATCH_API UGameEffectDestroyArea : public UGameEffectDestroyBlocksBase
{
	GENERATED_BODY()

	UPROPERTY()
	int32 DistanceX = 1;

	UPROPERTY()
	int32 DistanceY = 1;

public:
	// Constructor
	UGameEffectDestroyArea();

	virtual void SetEffectParams_Implementation(const FGameEffectContext& EffectContext) override;

	virtual TArray<FIntPoint> GetEffectedCoords_Implementation(const FIntPoint SelectedCoords) override;

	//virtual bool BeginEffect_Implementation(const TArray<FIntPoint>& PerformCoords) override;

};