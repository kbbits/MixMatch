#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "GameEffect.generated.h"


/**
 * Effects applied to the grid, player, etc.
 * Currently used by UsableGoods items.
 * TODO: Re-work MatchEffects to use GameEffects?
 */
UCLASS(BlueprintType, Blueprintable)
class MIXMATCH_API UGameEffect : public UObject
{
	GENERATED_BODY()

protected:

	// The number of selections required. Default is 0, i.e. no selection required.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame)
	int32 NumSelections = 0;

	// Thumbnail for GUI use
	UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame)
	TSoftObjectPtr<UTexture2D> Thumbnail;

	// How many player turns (block moves) this effect lasts.
	// Default = 0 , meaning instant.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame)
	int32 TurnDuration = 0;

private:

	// Tracks how many turns this effect has been in duration.
	// Only relevant if TurnDuration is > 0.
	int32 TurnsInEffect = 0;

public:
	// Constructor
	UGameEffect();

	// Does this effect require the user to make at least one selection?
	UFUNCTION(BlueprintCallable)
	bool RequiresSelection();

	// Checks if the effect can be triggered, optionally at the given grid coords. Default implementation just returns true.
	// Returns: true if effect can be triggered.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	bool CanTrigger(const TArray<FIntPoint>& PerformCoords);

	// Apply the effect to the game, optionally providing a list of grid coords to apply the effect to. Default implementation just returns true.
	// Returns: true if effect was triggered successfully.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	bool BeginEffect(const TArray<FIntPoint>& PerformCoords);

	// Increment the TurnsInEffect and perform any per-turn effects.
	// Base class implementation only increments TurnsInEffect.
	// Return true if the effect has remaining duration. 
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	bool IncrementTurn();

	// Remove the effect from the game.
	// Base class implementation does nothing.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void EndEffect();
};