#pragma once

#include "CoreMinimal.h"
//#include "MMEnums.h"
#include "GridEffect.generated.h"


USTRUCT(BlueprintType)
struct FGridEffectType
{
	GENERATED_BODY()

public:

	// Class of the effect to instantiate.
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	//	TSubclassOf<class UGridEffect> GridEffectClass;

	// Does this effect require blocks to be selected?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		bool bRequiresSelection = false;

	// If selections are required, this is the number of selections required.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		int32 NumSelections = 1;

	// Thumbnail for GUI use
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		TSoftObjectPtr<UTexture2D> Thumbnail;
};



/**
 */
UCLASS(BlueprintType, Blueprintable)
class MIXMATCH_API UGridEffect : public UObject
{
	GENERATED_BODY()

protected:
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadonly, SaveGame)
		FGridEffectType GridEffectType;

public:
	// Constructor
	UGridEffect();

	UFUNCTION(BlueprintCallable)
		void SetGridEffectType(const FGridEffectType& EffectType);

	UFUNCTION(BlueprintCallable)
		const FGridEffectType GetGridEffectType();

	UFUNCTION(BlueprintCallable)
		bool RequiresSelection();

	// Checks if the action can be triggered. Default implementation just returns true.
	// Returns: true if effect can be triggered.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
		bool CanPerform(const TArray<FIntPoint>& PerformCoords);

	// Apply the effect to the game, optionally providing a list of grid coords to apply the effect to. Default implementation just returns true.
	// Returns: true if effect was triggered successfully.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
		bool Perform(const TArray<FIntPoint>& PerformCoords);
};