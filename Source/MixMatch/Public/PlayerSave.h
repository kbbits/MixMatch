// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "PlayerSaveData.h"
#include "PlayerSave.generated.h"

/**
 * 
 */
UCLASS()
class MIXMATCH_API UPlayerSave : public USaveGame
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
		FPlayerSaveData PlayerSaveData;
	
};
