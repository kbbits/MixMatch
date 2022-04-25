// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameEffect/GameEffect.h"
#include "MMEnums.h"
#include "GameEffectSpawnBlocksBase.generated.h"


/**
 * Spawns NumToSpawn blocks within the indicated grid EffectCoords.
 * 
 * StringParam[0] = BlockType.Name to spawn
 * FloatParams[0] = Number of blocks to spawn. Value truncated first.
 */
UCLASS(BlueprintType, Blueprintable)
class MIXMATCH_API UGameEffectSpawnBlocksBase : public UGameEffect
{
	GENERATED_BODY()

public:

	/** The BeginEffect method will fill this in so that subclasses can access. */
	UPROPERTY(BlueprintReadOnly)
		TArray<FIntPoint> SpawnedBlockCoords;

protected:

	UPROPERTY()
	FName SpawnBlockName;

	UPROPERTY()
	int32 NumToSpawn;

public:
	// Constructor
	UGameEffectSpawnBlocksBase();

	virtual void SetEffectParams_Implementation(const FGameEffectContext& EffectContext) override;

	virtual bool CanTrigger_Implementation(const TArray<FIntPoint>& PerformCoords) override;

	/** This implementation attempts to destroy the blocks at the given coords. */
	virtual bool BeginEffect_Implementation(const TArray<FIntPoint>& PerformCoords) override;

};