// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GoodsQuantity.h"
#include "GoodsDropChance.h"
#include "GoodsDropSet.h"
#include "GoodsFunctionLibrary.generated.h"

/**
 * A collection of helper functions to manipulate GoodsQuantity and related structs.
 */
UCLASS()
class MIXMATCH_API UGoodsFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
		
public:

	// Multiply the quantity of each good by the given multiplier.
	UFUNCTION(BlueprintPure, Category = "Goods")
		static TArray<FGoodsQuantity> MultiplyGoodsQuantities(const TArray<FGoodsQuantity>& GoodsQuantities, const float Multiplier, const bool bTruncateQuantities = true);


	// Adds the quantities of the two GoodsQuantities arrays.
	// bNegateGoodsQuantitesTwo - if true, this will subtract GoodsQuantitiesTwo from GoodQuantitiesOne.
	UFUNCTION(BlueprintPure, Category = "Goods")
		static TArray<FGoodsQuantity> AddGoodsQuantities(const TArray<FGoodsQuantity>& GoodsQuantitiesOne, const TArray<FGoodsQuantity>& GoodsQuantitiesTwo, const bool bNegateGoodsQuantitiesTwo = false);


	// Transform a goods quantity range to a goods quantity. 
	// If optional QuantityScale is provided, the quantity will be mapped from min to max according to the scale (0-1) instead of determining randomly.
	UFUNCTION(BlueprintCallable, Category = "Goods")
		static FGoodsQuantity GoodsQuantityFromRange(UPARAM(ref) FRandomStream& RandStream, const FGoodsQuantityRange& QuantityRange, const float QuantityScale = -1.0f /* 0.0 - 1.0 */);


	// Transform an array of goods quantity ranges to an array of goods quantities. 
	// If optional QuantityScale is provided, each quantity will be mapped from min to max according to the scale instead of determining randomly.
	UFUNCTION(BlueprintCallable, Category = "Goods")
		static TArray<FGoodsQuantity> GoodsQuantitiesFromRanges(UPARAM(ref) FRandomStream& RandStream, const TArray<FGoodsQuantityRange>& QuantityRanges, const float QuantityScale = -1.0f /* 0.0 - 1.0 */);


	// Create a map of Name -> FGoodsQuantity from an array of FGoodsQuantities.
	UFUNCTION(BlueprintPure, Category = "Utilities| Goods")
	static FORCEINLINE TMap<FName, FGoodsQuantity> GoodsQuantityArrayToMap(TArray<FGoodsQuantity> GoodsQuantityArray)
	{
		TMap<FName, FGoodsQuantity> GoodsMap;
		for (FGoodsQuantity GQ : GoodsQuantityArray)
		{
			GoodsMap.Add(GQ.Name, GQ);
		}
		return GoodsMap;
	}


	// Returns the quantity of goods of the given name contained in the supplied goods quantity array
	UFUNCTION(BlueprintPure, Category = "Goods")
	static FORCEINLINE float CountInGoodsQuantityArray(const FName GoodsName, const TArray<FGoodsQuantity>& GoodsQuantities, bool& bFound)
	{
		const FGoodsQuantity* FoundGoods = GoodsQuantities.FindByKey<FName>(GoodsName);
		if (FoundGoods)
		{
			bFound = true;
			return FoundGoods->Quantity;
		}
		bFound = false;
		return 0.f;
	}
	static inline float CountInGoodsQuantityArray(const FName& GoodsName, const TArray<FGoodsQuantity>& GoodsQuantities)
	{
		bool bFound;
		return CountInGoodsQuantityArray(GoodsName, GoodsQuantities, bFound);
	}

	// Convenience to allow getting counts using GoodsQuantities as a filter of types to count.
	// Returns the quantities of goods in GoodsToCount that match the goods types in GoodsTypesToCount.
	// Note: the quantities in GoodsTypesToCount are ignored.
	UFUNCTION(BlueprintPure, Category = "Goods")
	static TArray<FGoodsQuantity> CountsInGoodsQuantities(const TArray<FGoodsQuantity>& GoodsTypesToCount, const TArray<FGoodsQuantity>& GoodsToCount);
	

	// Returns the quantity of goods of the given name contained in the supplied goods quantity set
	UFUNCTION(BlueprintPure, Category = "Goods")
	static FORCEINLINE float CountInGoodsQuantitySet(const FName GoodsName, const FGoodsQuantitySet& GoodsQuantitySet, bool& bFound)
	{
		return CountInGoodsQuantityArray(GoodsName, GoodsQuantitySet.Goods, bFound);
	}

	// For an array where each item has a Chance (float) property that is > 0.0, this will pick one item from the array.
	// Item picked is a weighted random selection in the weighted list. 
	// Can return nullptr if list is empty or results in negative total weight.
	// ex: useful for ad-hock picking a GoodsDropChance from an array of GoodsDropChances.
	template<class T>
	static FORCEINLINE T* PickOneFromWeightedList(FRandomStream& RandStream, TArray<T>& WeightedItems, const float TotalWeight = -1.0f)
	{
		float TotalWeightedChance = TotalWeight;
		float PickedWeight = 0.0f;
		float CumulativeWeight = 0.0;
		// Sum our total weights if it was not passed in.
		if (TotalWeightedChance < 0.0f)
		{
			TotalWeightedChance = 0.0f;
			for (T& WeightedItem : WeightedItems)
			{
				TotalWeightedChance += FMath::Abs(WeightedItem.Chance);
			}
		}
		if (TotalWeightedChance <= 0.0f)
		{
			return nullptr;
		}
		PickedWeight = FMath::FRandRange(0.0, TotalWeightedChance);
		//UE_LOG(LogMMGame, Log, TEXT("PickOne total weight: %f  picked weight %f"), TotalWeightedChance, PickedWeight);

		// Iterate through our list of items until we find the first one where the overall PickedWeight is less than our cumulative total weight of items iterated so far.
		for (T& WeightedItem : WeightedItems)
		{
			CumulativeWeight += WeightedItem.Chance;
			if (PickedWeight <= CumulativeWeight)
			{
				//UE_LOG(LogMMGame, Log, TEXT("Picked: %s"), *WeightedItem.UniqueNameBase.ToString())
				return &WeightedItem;
			}
		}
		return nullptr;
	}

};
