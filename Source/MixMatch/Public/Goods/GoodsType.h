#pragma once

#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "GoodsType.generated.h"

/*
* Represents a type of commoditiy that could be collected by the player.
* (or used as currency, crafting ingredients, etc.)
* Not all goods types will have specific actors if/when they are placed into the world.
*/
USTRUCT(BlueprintType)
struct FGoodsType : public FTableRowBase
{
	GENERATED_BODY()

public:
	// Internal name of this GoodsType. Must be unique.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		FName Name;

	// Name displayed to player
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		FText DisplayName;

	// Thumbnail for GUI use
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		TAssetPtr<UTexture2D> Thumbnail;

	// If this value is present it will be used during auto-value calculation instead of calculating it.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		float OverrideValue;

	// Optional actor class to spawn when placing into the world.
	// If valid, this will be the actor used to place the goods in the world. If not valid, it will be placed as a GoodsPickup using this DefaultMesh. See: GoodsPickup_BP.
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	//	TSoftClassPtr<AActor> ActorClass;

	// The default mesh to use when placing these goods into the world if the ActorClass property is null. See: GoodsPickup_BP
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	//	TAssetPtr<UStaticMesh> DefaultMesh;

	// Various tags related to this GoodsType.
	// Also allows other specifiers:
	//    ValueIgnoreProducedFrom:<InputGoodsName> -- Ignore the given input goods (do not recurse it) when calculating auto-values. Prevents infinite loops with circular recipes.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		TArray<FName> GoodsTags;

	FORCEINLINE bool operator==(const FGoodsType& OtherType) const
	{
		if (Name != OtherType.Name) return false;
		return true;
	}

	FORCEINLINE bool operator==(const FGoodsType& OtherType)
	{
		if (Name != OtherType.Name) return false;
		return true;
	}

	FORCEINLINE bool operator==(FGoodsType& OtherType)
	{
		if (Name != OtherType.Name) return false;
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