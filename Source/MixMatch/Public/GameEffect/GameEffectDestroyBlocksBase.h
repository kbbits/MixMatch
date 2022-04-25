// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameEffect/GameEffect.h"
#include "MMEnums.h"
#include "GameEffectDestroyBlocksBase.generated.h"


/**
 * FloatParams[0] : Number of blocks to destroy from list of coords provided.
 * FloatParams[1] : BlockDamage applied to damage-able blocks. Truncated before applied. Defaults to 1.
 */
UCLASS(BlueprintType, Blueprintable)
class MIXMATCH_API UGameEffectDestroyBlocksBase : public UGameEffect
{
	GENERATED_BODY()

public:

	UPROPERTY()
	int32 NumToDestroy;

	/** Blocks that CanBeDamaged() will not be destroyed directly. They will be damaged.
	 *  This is the amount of damage to be dealt to each of these blocks. */
	UPROPERTY()
	int32 BlockDamage = 1;

	/** The BeginEffect method will fill this in so that subclasses can access. */
	UPROPERTY(BlueprintReadOnly)
	TArray<FIntPoint> DestroyedBlockCoords;


public:
	// Constructor
	UGameEffectDestroyBlocksBase();

	virtual void SetEffectParams_Implementation(const FGameEffectContext& EffectContext) override;

	virtual bool CanTrigger_Implementation(const TArray<FIntPoint>& PerformCoords) override;

	/** This implementation attempts to destroy the blocks at the given coords. */
	virtual bool BeginEffect_Implementation(const TArray<FIntPoint>& PerformCoords) override;

};