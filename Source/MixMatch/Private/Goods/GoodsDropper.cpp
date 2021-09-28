// Fill out your copyright notice in the Description page of Project Settings.

#include "Goods/GoodsDropper.h"
#include "Goods/GoodsFunctionLibrary.h"
//#include "MixMatch.h"

/*
*/
UGoodsDropper::UGoodsDropper()
	: Super()
{
	RandStream.GenerateNewSeed();
}

/*
*/
bool UGoodsDropper::AddDropTableDataToLibrary(UDataTable* GoodsDropSetsData)
{
	if (!GoodsDropSetsData->IsValidLowLevel())
	{
		//UE_LOG(LogMMGame, Warning, TEXT("GoodsDropper AddDropTableToLibrary goods drop table is not valid"));
		return false;
	}
	TArray<FName> RowNames = GoodsDropSetsData->GetRowNames();
	if (RowNames.Num() > 0)
	{
		// Check that this is a table of FGoodsDropSet rows.
		FGoodsDropSet* FirstTable = GoodsDropSetsData->FindRow<FGoodsDropSet>(RowNames[0], "", false);
		if (FirstTable == nullptr) {
			return false;
		}
		DropTableLibrary.AddUnique(GoodsDropSetsData);
	}
	return true;
}

/*
*/
bool UGoodsDropper::RemoveDropTableDataFromLibrary(UDataTable * GoodsDropSetsData)
{
	int32 NumRemoved = DropTableLibrary.Remove(GoodsDropSetsData);
	return NumRemoved > 0;
}

/*
*/
void UGoodsDropper::ClearDropTableLibrary()
{
	DropTableLibrary.Empty();
}

/*
*/
void UGoodsDropper::SeedRandomStream(const int32 NewSeed)
{
	RandStream.Initialize(NewSeed);
}

/*
*/
TArray<FGoodsQuantity> UGoodsDropper::EvaluateGoodsDropSet(const FGoodsDropSet& GoodsSet, const float QuantityScale)
{
	TArray<FGoodsQuantity> AllGoods;
	int32 TotalPicks;
	if (GoodsSet.bAsWeightedList)
	{
		// Make a number of picks
		TotalPicks = RandStream.RandRange(GoodsSet.MinWeightedPicks, GoodsSet.MaxWeightedPicks);
		// Each pick is one item from the weighted list of items.
		for (int i = 1; i <= TotalPicks; i++)
		{
			AllGoods.Append(EvaluateGoodsDropChanceWeighted(GoodsSet.GoodsChances, QuantityScale));
		}
	}
	else
	{
		// Each entry has a percent chance to be included.
		for (FGoodsDropChance DropChance : GoodsSet.GoodsChances)
		{
			if (DropChance.Chance > 0.0f) // Don't bother evaluating if Chance is not > 0
			{
				AllGoods.Append(EvaluateGoodsDropChancePercent(DropChance, QuantityScale));
			}
		}
	}
	return AllGoods;
}
 
TArray<FGoodsQuantity> UGoodsDropper::EvaluateGoodsDropSetByName(const FName & DropSetName, const float QuantityScale)
{
	TArray<FGoodsQuantity> AllGoods;
	const FGoodsDropSet* FoundDropSet = FindDropSetInLibrary(DropSetName);

	if (FoundDropSet != nullptr)
	{
		AllGoods = EvaluateGoodsDropSet(*FoundDropSet, QuantityScale);
	}
	else
	{
		AllGoods = TArray<FGoodsQuantity>();
		// TODO: report missed drop table hit
	}
	return AllGoods;
}

/*
*/
const FGoodsDropSet* UGoodsDropper::FindDropSetInLibrary(const FName DropSetName) const
{
	FGoodsDropSet* FoundDropSet = nullptr;

	// Look in each data table until we find one containing a row with the given name. Then return that row.
	for (UDataTable* Table : DropTableLibrary)
	{
		FoundDropSet = Table->FindRow<FGoodsDropSet>(DropSetName, "", false);
		// If we found the table, stop looking through library
		if (FoundDropSet != nullptr) { break; }
	}
	return FoundDropSet;
}

/*
*/
TArray<FGoodsQuantity> UGoodsDropper::EvaluateGoodsDropChanceWeighted(const TArray<FGoodsDropChance>& DropChances, const float QuantityScale)
{
	TArray<FGoodsQuantity> AllGoods;
	float TotalWeight = 0.0f;
	float AccumWeight = 0.0f;
	float Pick = 0.0f;

	for (const FGoodsDropChance& DropChance : DropChances)
	{
		TotalWeight += FMath::Abs<float>(DropChance.Chance);
	}
	if (TotalWeight <= 0.0f) { return AllGoods; }
	Pick = RandStream.FRandRange(0.0f, TotalWeight);
	for (const FGoodsDropChance& DropChance : DropChances)
	{
		if (DropChance.Chance <= 0.0f) { continue; }
		AccumWeight += DropChance.Chance;
		if (Pick <= AccumWeight)
		{
			AllGoods.Append(GoodsForDropChance(DropChance, QuantityScale));
			break;
		}
	}
	return AllGoods;
}

/*
*/
TArray<FGoodsQuantity> UGoodsDropper::EvaluateGoodsDropChancePercent(const FGoodsDropChance& DropChance, const float QuantityScale)
{
	TArray<FGoodsQuantity> AllGoods;
	if (DropChance.Chance <= 0.0f) { return AllGoods; }
	// Determine if this drop chance evaluates to a successful drop
	if (RandStream.FRandRange(0.0f, 1.0f) <= DropChance.Chance)
	{
		AllGoods = GoodsForDropChance(DropChance, QuantityScale);
	}
	return AllGoods;
}

/*
*/
TArray<FGoodsQuantity> UGoodsDropper::GoodsForDropChance(const FGoodsDropChance & DropChance, const float QuantityScale)
{
	TArray<FGoodsQuantity> AllGoods;
	FGoodsQuantity TmpGoods;

	if (DropChance.GoodsQuantities.Num() > 0)
	{
		// Evaluate all GoodsQuantities and add them to our collection
		AllGoods.Append(UGoodsFunctionLibrary::GoodsQuantitiesFromRanges(RandStream, DropChance.GoodsQuantities, QuantityScale));
	}
	
	// Evaluate any other GoodsDropSets and add them to our collection (if any)
	for (const FName& DropSetName : DropChance.OtherGoodsDrops)
	{
		// Find drop set in library
		const FGoodsDropSet* DropSet = FindDropSetInLibrary(DropSetName);
		if (DropSet) {
			AllGoods.Append(EvaluateGoodsDropSet(*DropSet, QuantityScale));
		}
	}
	return AllGoods;
}
