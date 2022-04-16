#pragma once

#include "SimpleNamedTypes.generated.h"

// 
// A collection of named primitive types and some templated functions for manipulating them. Useful for serialization.
//
// Named Primitive types as structs
//

USTRUCT(BlueprintType)
struct FSimpleNamedFloat
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		float Quantity;

public:
	FSimpleNamedFloat()
	{
		Name = FName();
		Quantity = 0.0f;
	}

	FSimpleNamedFloat(const FName& NewName, const float NewQuantity)
	{
		Name = NewName;
		Quantity = NewQuantity;
	}

	FORCEINLINE bool operator==(const FSimpleNamedFloat& Other) const
	{
		if (Name != Other.Name) return false;
		if (Quantity != Other.Quantity) return false;
		return true;
	}

	FORCEINLINE bool operator==(const FSimpleNamedFloat& Other)
	{
		if (Name != Other.Name) return false;
		if (Quantity != Other.Quantity) return false;
		return true;
	}

	FORCEINLINE bool operator==(FSimpleNamedFloat& Other)
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
struct FSimpleNamedInt
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		int32 Quantity;

public:
	FSimpleNamedInt()
	{
		Name = FName();
		Quantity = 0;
	}

	FSimpleNamedInt(const FName& NewName, const int32 NewQuantity)
	{
		Name = NewName;
		Quantity = NewQuantity;
	}

	FORCEINLINE bool operator==(const FSimpleNamedInt& Other) const
	{
		if (Name != Other.Name) return false;
		if (Quantity != Other.Quantity) return false;
		return true;
	}

	FORCEINLINE bool operator==(const FSimpleNamedInt& Other)
	{
		if (Name != Other.Name) return false;
		if (Quantity != Other.Quantity) return false;
		return true;
	}

	FORCEINLINE bool operator==(FSimpleNamedInt& Other)
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
struct FSimpleNamedBool
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		bool Value;

public:
	FSimpleNamedBool()
	{
		Name = FName();
		Value = false;
	}

	FSimpleNamedBool(const FName& NewName, const bool NewValue)
	{
		Name = NewName;
		Value = NewValue;
	}

	FORCEINLINE bool operator==(const FSimpleNamedBool& Other) const
	{
		if (Name != Other.Name) return false;
		if (Value != Other.Value) return false;
		return true;
	}

	FORCEINLINE bool operator==(const FSimpleNamedBool& Other)
	{
		if (Name != Other.Name) return false;
		if (Value != Other.Value) return false;
		return true;
	}

	FORCEINLINE bool operator==(FSimpleNamedBool& Other)
	{
		if (Name != Other.Name) return false;
		if (Value != Other.Value) return false;
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
struct FSimpleNamedVector
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		FVector Value;

public:
	FSimpleNamedVector()
	{
		Name = FName();
		Value = FVector(0.f);
	}

	FSimpleNamedVector(const FName& NewName, const FVector& NewValue)
	{
		Name = NewName;
		Value = NewValue;
	}

	FORCEINLINE bool operator==(const FSimpleNamedVector& Other) const
	{
		if (Name != Other.Name) return false;
		if (Value != Other.Value) return false;
		return true;
	}

	FORCEINLINE bool operator==(const FSimpleNamedVector& Other)
	{
		if (Name != Other.Name) return false;
		if (Value != Other.Value) return false;
		return true;
	}

	FORCEINLINE bool operator==(FSimpleNamedVector& Other)
	{
		if (Name != Other.Name) return false;
		if (Value != Other.Value) return false;
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

// Helpful functions:

// Create a map from an array. Each element in the array must have a Name (FName) property. (ex: FSimpleNamedFloat)
// New map has Name as key and element as value.  Collisions of Name in array are overwritten, last one wins.
template<class T>
static FORCEINLINE TMap<FName, T> NamedItemsToMap(const TArray<T>& NamedItems)
{
	TMap<FName, T> NamedItemMap;
	for (T& NamedItem : NamedItems)
	{
		NamedItemMap.Add(NamedItem.Name, NamedItem);
	}
	return NamedItemMap;
}


// Create a map from an array. Each element in array must have a Name (FName) and a Quantity (int) property. (ex: FSimpleNamedFloat)
// New map will have Name as key and Quantity as value. Collisions of Name in array are overwritten, last one wins.
// ex: Useful for transforming an array of NamedFloats or GoodsQuantities to a map with each key as the Name and the Quantity as the value.
template<class T>
static FORCEINLINE TMap<FName, int32> NamedQuantitiesToCountMap(const TArray<T>& NamedItems)
{
	TMap<FName, int32> NamedCountMap;
	for (const T& NamedItem : NamedItems)
	{
		NamedCountMap.Add(NamedItem.Name, static_cast<int32>(NamedItem.Quantity));
	}
	return NamedCountMap;
}


// Create a map from an array. Each element in array must have a Name (FName) and a Quantity (float) property. (ex: FGoodsQuantity)
// New map will have Name as key and Quantity as value. Collisions of Name in array are overwritten, last one wins.
// ex: Useful for transforming an array of GoodsQuantities to a map with each key as the GoodsQuantity.Name and the Quantity as the value.
template<class T>
static FORCEINLINE TMap<FName, float> NamedQuantitiesToFloatMap(const TArray<T>& NamedItems)
{
	TMap<FName, float> NamedFloatMap;
	for (const T& NamedItem : NamedItems)
	{
		NamedFloatMap.Add(NamedItem.Name, static_cast<float>(NamedItem.Quantity));
	}
	return NamedFloatMap;
}


// Convert a map of FNames to floats into an array of FSimpleNamedFloats.
// Returns the number of items in the new array.
static FORCEINLINE int32 FloatMapToNamedArray(const TMap<FName, float>& NamedMap, TArray<FSimpleNamedFloat>& NamedArray)
{
	NamedArray.Empty(NamedMap.Num());
	for (const TPair<FName, float> MapItem : NamedMap)
	{
		NamedArray.Emplace(FSimpleNamedFloat(MapItem.Key, MapItem.Value));
	}
	return NamedArray.Num();
}

// Convert a map of FNames to ints into an array of FSimpleNamedInts.
// Returns the number of items in the new array.
static FORCEINLINE int32 IntMapToNamedArray(const TMap<FName, int32>& NamedMap, TArray<FSimpleNamedInt>& NamedArray)
{
	NamedArray.Empty(NamedMap.Num());
	for (const TPair<FName, int32> MapItem : NamedMap)
	{
		NamedArray.Emplace(FSimpleNamedInt(MapItem.Key, MapItem.Value));
	}
	return NamedArray.Num();
}


// Find the first item in the array that has a "Name" property who's value is == NameToFind.
// Returns a pointer to the found array entry, or nullptr if not found.
template<class T>
static FORCEINLINE T* FindInNamedArray(TArray<T, FDefaultAllocator>& NamedItems, FName NameToFind)
{
	for (T& NamedItem : NamedItems)
	{
		if (NamedItem.Name == NameToFind)
		{
			return &NamedItem;
		}
	}
	return nullptr;
}

// const version of FindInNamedArray
template<class T>
static FORCEINLINE const T* FindInNamedArray(const TArray<T, FDefaultAllocator>& NamedItems, FName NameToFind)
{
	for (const T& NamedItem : NamedItems)
	{
		if (NamedItem.Name == NameToFind)
		{
			return &NamedItem;
		}
	}
	return nullptr;
}


// Given an array of items that each have a "Name" property, this will search the array for an item with the given name and:
//   if found - will overwrite that array entry with the ValueToSet
//   if not found - will add the ValueToSet to the array
// Returns true if an existing entry was found or false if not found.
template<class T>
static FORCEINLINE bool AddUniqueToNamedArray(TArray<T>& NamedItems, T& ValueToSet)
{
	for (int i = 0; i < NamedItems.Num(); i++)
	{
		if (NamedItems.IsValidIndex(i) && NamedItems[i].Name == ValueToSet.Name)
		{
			NamedItems[i] = ValueToSet;
			return true;
		}
	}
	NamedItems.Emplace(ValueToSet);
	return false;
}