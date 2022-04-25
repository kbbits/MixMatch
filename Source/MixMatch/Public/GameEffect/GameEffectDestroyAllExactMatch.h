// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BlockMatch.h"
#include "GameEffect/GameEffectDestroyBlocksBase.h"
#include "GameEffectDestroyAllExactMatch.generated.h"


/**
 * Destroys all blocks in the gried where selected block (from submitted coords) MatchCode == other block MatchCode.
 * FloatParams[0] : BlockDamage applied to damage-able blocks. Truncated before applied. Defaults to 1.
 */
UCLASS(BlueprintType, Blueprintable)
class MIXMATCH_API UGameEffectDestroyAllExactMatch : public UGameEffectDestroyBlocksBase
{
	GENERATED_BODY()


public:
	// Constructor
	UGameEffectDestroyAllExactMatch();

	virtual void SetEffectParams_Implementation(const FGameEffectContext& EffectContext) override;

	virtual TArray<FIntPoint> GetEffectedCoords_Implementation(const FIntPoint SelectedCoords) override;

	//virtual bool BeginEffect_Implementation(const TArray<FIntPoint>& PerformCoords) override;

};