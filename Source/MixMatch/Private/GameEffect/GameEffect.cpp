// Fill out your copyright notice in the Description page of Project Settings.

#include "GameEffect/GameEffect.h"

/*
*/
UGameEffect::UGameEffect()
	: Super()
{
}


bool UGameEffect::RequiresSelection()
{
	return NumSelections == 0;
}


bool UGameEffect::CanTrigger_Implementation(const TArray<FIntPoint>& PerformCoords)
{
	return true;
}


bool UGameEffect::BeginEffect_Implementation(const TArray<FIntPoint>& PerformCoords)
{
	return true;
}


bool UGameEffect::IncrementTurn_Implementation()
{
	TurnsInEffect++;

	return TurnDuration > TurnsInEffect;
}


void UGameEffect::EndEffect_Implementation()
{
}