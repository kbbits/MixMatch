// Fill out your copyright notice in the Description page of Project Settings.

#include "GameEffect/GameEffect.h"

/*
*/
UGameEffect::UGameEffect()
	: Super()
{
	EffectPreviewClass = AGameEffectPreviewActor::StaticClass();
}


void UGameEffect::SetEffectParams_Implementation(const FGameEffectContext& EffectContext)
{

}


bool UGameEffect::RequiresSelection()
{
	return NumSelections > 0;
}


EMMBlockHandling UGameEffect::GetBlockHandling()
{
	return BlockHandling;
}


bool UGameEffect::CanTrigger_Implementation(const TArray<FIntPoint>& PerformCoords)
{
	return true;
}


TArray<FIntPoint> UGameEffect::GetEffectedCoords_Implementation(const FIntPoint SelectedCoords)
{
	TArray<FIntPoint> EffectedCoords;
	EffectedCoords.Add(SelectedCoords);
	return EffectedCoords;
}


bool UGameEffect::BeginEffect_Implementation(const TArray<FIntPoint>& PerformCoords)
{
	return true;
}


int32 UGameEffect::GetRemainingDuration()
{
	return FMath::Max(0, TurnDuration - TurnsInEffect);
}


bool UGameEffect::IncrementTurn_Implementation()
{
	TurnsInEffect++;
	return TurnDuration > TurnsInEffect;
}


void UGameEffect::EndEffect_Implementation()
{
}