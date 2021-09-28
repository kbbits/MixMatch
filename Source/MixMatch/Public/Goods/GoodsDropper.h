// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/DataTable.h"
#include "GoodsQuantity.h"
#include "GoodsDropChance.h"
#include "GoodsDropSet.h"
#include "GoodsDropper.generated.h"

/**
 * Provides functionaly to evaluate groups of drop sets (FGoodsDropSet).
 * Each goods drop set is a collection of goods drop chances.
 * Note: when evaluated a goods drop chance in a goods drop set may reference other goods drop set. This allows the designer to 
 * set up some goods drop sets as categories of goods and other goods drop sets that have chances of including results from those
 * categorized goods drop sets.
 */
UCLASS(BlueprintType, Blueprintable)
class MIXMATCH_API UGoodsDropper : public UObject
{
	GENERATED_BODY()
	
public:
	// Constructor
	UGoodsDropper();

public:

	// Add this DataTable of GoodsDropSets to our known list of goods drop data.
	// The DataTable rows _must_ be GoodsDropSet structs.
	UFUNCTION(BlueprintCallable, Category = "Goods")
	bool AddDropTableDataToLibrary(UDataTable* GoodsDropSetsData);

	UFUNCTION(BlueprintCallable, Category = "Goods")
	bool RemoveDropTableDataFromLibrary(UDataTable* GoodsDropSetsData);

	UFUNCTION(BlueprintCallable, Category = "Goods")
	void ClearDropTableLibrary();

	UFUNCTION(BlueprintCallable, Category = "Goods")
	void SeedRandomStream(const int32 NewSeed);
		
	// Evaluate this drop set and return all Goods droppped.
	UFUNCTION(BlueprintCallable, Category = "Goods")
	TArray<FGoodsQuantity> EvaluateGoodsDropSet(const FGoodsDropSet& GoodsSet, const float QuantityScale = -1.0f);

	// Evaluate the named drop set and return all Goods droppped.
	UFUNCTION(BlueprintCallable, Category = "Goods")
	TArray<FGoodsQuantity> EvaluateGoodsDropSetByName(const FName& DropSetName, const float QuantityScale = -1.0f);

	// Evaluate this list of DropChances as a weighted list of items.
	// Returns the goods dropped by the single selected item from the weighted list.
	TArray<FGoodsQuantity> EvaluateGoodsDropChanceWeighted(const TArray<FGoodsDropChance>& DropChances, const float QuantityScale = -1.0f);

	// Evanuate this single DropChance by a % chance according to it's Chance value: 0.0 - 1.0
	TArray<FGoodsQuantity> EvaluateGoodsDropChancePercent(const FGoodsDropChance& DropChance, const float QuantityScale = -1.0f);

	// Get a random drop of the goods from this GoodsDropChance.
	TArray<FGoodsQuantity> GoodsForDropChance(const FGoodsDropChance& DropChance, const float QuantityScale = -1.0f);

private:
	// Our random stream.  Use SeedRandomStream to set this if needed.
	FRandomStream RandStream;

	// Our collection of DataTables, each containing GoodsDropSet rows
	TArray<UDataTable*> DropTableLibrary;
	
	// Find the GoodsDropTable data in the DropTableLibrary that has the given name.
	const FGoodsDropSet* FindDropSetInLibrary(const FName DropSetName) const;

};
