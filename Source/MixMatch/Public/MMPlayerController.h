// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InventoryActorComponent.h"
#include "RecipeManagerComponent.h"
#include "MMPlayerController.generated.h"

/** PlayerController class used to enable cursor */
UCLASS()
class AMMPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMMPlayerController();

	// Player Goods inventory
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	UInventoryActorComponent* GoodsInventory;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	URecipeManagerComponent* RecipeManager;

};


