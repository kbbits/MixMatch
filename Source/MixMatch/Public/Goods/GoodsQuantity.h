// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GoodsQuantity.generated.h"

/*
* Represents a quantity of a given goods type.
* The GoodsQuantity.Name much match a valid GoodsType.Name.
*/
USTRUCT(BlueprintType)
struct FGoodsQuantity 
{
	GENERATED_BODY()

public:

	// The name of the GoodsType this quantity represents.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		float Quantity;

public:
	FGoodsQuantity()
	{
		Name = FName();
		Quantity = 0.0f;
	}

	FGoodsQuantity(const FName& NewName, const float NewQuantity)
	{
		Name = NewName;
		Quantity = NewQuantity;
	}

	FORCEINLINE bool operator==(const FGoodsQuantity& Other) const
	{
		if (Name != Other.Name) return false;
		if (Quantity != Other.Quantity) return false;
		return true;
	}

	FORCEINLINE bool operator==(const FGoodsQuantity& Other)
	{
		if (Name != Other.Name) return false;
		if (Quantity != Other.Quantity) return false;
		return true;
	}

	FORCEINLINE bool operator==(FGoodsQuantity& Other)
	{
		if (Name != Other.Name) return false;
		if (Quantity != Other.Quantity) return false;
		return true;
	}

	FORCEINLINE bool operator==(const FName& OtherName) const
	{
		if (Name != OtherName) return false;
		return true;
	}

	FORCEINLINE bool operator==(const FName& OtherName)
	{
		if (Name != OtherName) return false;
		return true;
	}

	FORCEINLINE bool operator==(FName& OtherName)
	{
		if (Name != OtherName) return false;
		return true;
	}
};


USTRUCT(BlueprintType)
struct FGoodsQuantitySet
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		TArray<FGoodsQuantity> Goods;
};


USTRUCT(BlueprintType)
struct FNamedGoodsQuantitySet
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		FGoodsQuantitySet GoodsQuantitySet;
};


USTRUCT(BlueprintType)
struct FGoodsQuantityRange
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		FName GoodsName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		float QuantityMin;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		float QuantityMax;

};


USTRUCT(BlueprintType)
struct FGoodsQuantityRangeSet
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		TArray<FGoodsQuantityRange> GoodsQuantityRanges;

};