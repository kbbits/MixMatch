#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "MMEnums.h"
#include "SimpleNamedTypes.h"
#include "GameEffect/GameEffectPreviewActor.h"
#include "GameEffect.generated.h"


USTRUCT(BlueprintType)
struct FGameEffectContext
{
	GENERATED_BODY()

public:

	/** The GameEffect class to instantiate.  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	TSubclassOf<class UGameEffect> GameEffectClass;

	/** The maximum number of block selections associated with the effect. */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	//int32 MaxSelections = 0;

	/** Param values used by this GameEffect. Subclasses implement differently.
	 *  See subclasses for details. */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (TitleProperty= "Name"))
	//TArray<FSimpleNamedFloat> NamedFloatParams;

	/** How these params are used differes by GameEffect subclass implementation.
	 *  See each subclass for details. */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (TitleProperty = "Name"))
	//TArray<FSimpleNamedString> NamedStringParams;

	/** How these params are used differes by GameEffect subclass implementation.
	 *  See each subclass for details. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	TArray<float> FloatParams;

	/** How these params are used differes by GameEffect subclass implementation.
	 *  See each subclass for details. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	TArray<FString> StringParams;
};


/**
 * Effects applied to the grid, player, etc.
 * Currently used by UsableGoods items.
 * TODO: Re-work MatchEffects to use GameEffects?
 */
UCLASS(BlueprintType, Blueprintable)
class MIXMATCH_API UGameEffect : public UObject
{
	GENERATED_BODY()

public:
	// Thumbnail for GUI use
	UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame)
	TSoftObjectPtr<UTexture2D> Thumbnail;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame)
	TSubclassOf<AGameEffectPreviewActor> EffectPreviewClass;

protected:

	// The number of selections required. Default is 0, i.e. no selection required.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame)
	int32 NumSelections = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame)
	EMMBlockHandling BlockHandling;

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

	/** Set this effects parameters from those contained in the GameEffectContext */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void SetEffectParams(const FGameEffectContext& EffectContext);

	/** Does this effect require the user to make at least one selection ? */
	UFUNCTION(BlueprintCallable)
	bool RequiresSelection();

	UFUNCTION(BlueprintCallable)
	EMMBlockHandling GetBlockHandling();

	/** Checks if the effect can be triggered, optionally at the given grid coords. Default implementation just returns true.
	 * Returns: true if effect can be triggered. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	bool CanTrigger(const TArray<FIntPoint>& PerformCoords);

	/** Returns the list of cell coords that would be effected by applying the effect at the selected coords. 
	 *  Base class just returns SelectedCoords. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	TArray<FIntPoint> GetEffectedCoords(const FIntPoint SelectedCoords);

	/** Apply the effect to the game, optionally providing a list of grid coords to apply the effect to.Default implementation just returns true.
	 * Subclasses override for FX, etc.
	 * Returns: true if effect was triggered successfully. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	bool BeginEffect(const TArray<FIntPoint>& PerformCoords);

	/** Get the number of turns remaining for this effect. */
	UFUNCTION(BlueprintPure)
	int32 GetRemainingDuration();

	/** Increment the TurnsInEffectand perform any per - turn effects.
	 * Base class implementation just increments TurnsInEffect.
	 * Return true if the effect has remaining duration. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	bool IncrementTurn();

	/** Remove the effect from the game.
	 * Base class implementation does nothing. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void EndEffect();
};


