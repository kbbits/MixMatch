// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BlockMatch.h"
#include "MMEnums.h"
#include "GameEffect/GameEffect.h"
#include "MatchAction.generated.h"


USTRUCT(BlueprintType)
struct FMatchActionType
{
	GENERATED_BODY()

public:

	// Name of this action type. Must be unique.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	FName Name;

	// The GameEffects to trigger for this match
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	TArray<FGameEffectContext> GameEffects;

	// Class of the action to instantiate and call Perform().
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (DeprecatedProperty))
	//TSubclassOf<class UMatchAction> MatchActionClass;

	// How to apply this action. ex once per match, once per block, etc.
	// Game effects will be applied accordingly
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	EMMBlockQuantity ActionQuantityType = EMMBlockQuantity::PerMatch;

	// Quantity applied to the action. Not all actions use this.
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (DeprecatedProperty))
	//float Quantity = 1.f;

	// Parameter string passed to the action. Not all actions use this.
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (DeprecatedProperty))
	//FString ActionParam;

	/** Used to determine when the action is performed. */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	//EMMMatchActionCategory ActionCategory;
};


USTRUCT(BlueprintType)
struct FMatchActionTrigger
{
	GENERATED_BODY()

public:

	// Minimum number of blocks in the match beyond the MinimumMatchSize (i.e. TotalBlocksInMatch - MinimumMatchSize) required to trigger the ActionName
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		int32 MinBonusSizeToTrigger = 1;

	// Maximum number of blocks in the match beyond the MinimumMatchSize (i.e. TotalBlocksInMatch - MinimumMatchSize) required to trigger the ActionName
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		int32 MaxBonusSizeToTrigger = 1;

	//The MatchActionType to perform.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		FMatchActionType ActionType;
};

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class MIXMATCH_API UMatchAction : public UObject
{
	GENERATED_BODY()

public:
	// Constructor
	UMatchAction();

	// Perform this action. Default implementation just returns true.
	// Returns: true if action operation was successful.
	//UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	//bool Perform(const UBlockMatch* Match, const FMatchActionType& MatchActionType, const AMMBlock* TriggeringBlock);

	// Perform the given GameEffect associated with this action. 
	// Returns: true if action operation was successful.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	bool PerformGameEffect(const FGameEffectContext EffectContext, const AMMBlock* TriggeringBlock);
};
