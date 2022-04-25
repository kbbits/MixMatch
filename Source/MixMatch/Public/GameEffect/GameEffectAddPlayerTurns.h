// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameEffect/GameEffect.h"
#include "MMEnums.h"
#include "GameEffectAddPlayerTurns.generated.h"


/**
 * Adds additional player turns to the player's remaining turns in the grid.
 * 
 * FloatParams[0] = Number of turns to add. Value is truncated first.
 */
UCLASS(BlueprintType, Blueprintable)
class MIXMATCH_API UGameEffectAddPlayerTurns : public UGameEffect
{
	GENERATED_BODY()

protected:

	UPROPERTY()
	int32 NumToAdd;

public:
	// Constructor
	UGameEffectAddPlayerTurns();

	virtual void SetEffectParams_Implementation(const FGameEffectContext& EffectContext) override;

	virtual TArray<FIntPoint> GetEffectedCoords_Implementation(const FIntPoint SelectedCoords) override;

	virtual bool CanTrigger_Implementation(const TArray<FIntPoint>& PerformCoords) override;

	virtual bool BeginEffect_Implementation(const TArray<FIntPoint>& PerformCoords) override;

};