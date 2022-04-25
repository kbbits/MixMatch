#pragma once

#include "ActionBarItem.generated.h"

class UUsableGoods;

/*
* Contains the full details for goods types that are "usable" by the player.
* Each record row name and Name must match a GoodsType.
*/
UCLASS(BlueprintType)
class MIXMATCH_API UActionBarItem : public UObject
{
	GENERATED_BODY()

public:

	/** Instance of UsableGoods occupying this slot. */
	UPROPERTY(BlueprintReadOnly)
	UUsableGoods* UsableGoods;

public:

	UActionBarItem();

	/** Clears out references, etc. "emptying" this object. */
	UFUNCTION(BlueprintCallable)
	void Cleanup();

	/** Gets the goods name of this action bar item.
	 *  Returns NAME_None if there is no valid UsableGoods associated with this item. */
	UFUNCTION(BlueprintPure)
	FName GetName();
};