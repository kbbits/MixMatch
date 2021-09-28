// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MMEnums.h"
#include "MMMath.generated.h"

/**
 *
 */
UCLASS()
class UMMMath : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintPure)
        static FVector2D DirectionToOffsetVector(const EMMDirection Direction);

    UFUNCTION(BlueprintPure)
        static FIntPoint DirectionToOffset(const EMMDirection Direction);

    UFUNCTION(BlueprintPure)
        static EMMDirection OppositeDirection(const EMMDirection Direction);

    UFUNCTION(BlueprintPure)
        static bool DirectionIsHorizontal(const EMMDirection Direction);

    UFUNCTION(BlueprintPure)
        static bool DirectionIsVertical(const EMMDirection Direction);

    /* Returns true if coords are orthagonally adjacent*/
    UFUNCTION(BlueprintPure)
        static bool CoordsAdjacent(const FIntPoint CoordsA, const FIntPoint CoordsB);
};