// Fill out your copyright notice in the Description page of Project Settings.

#include "GridEffect.h"

/*
*/
UGridEffect::UGridEffect()
	: Super()
{
}


void UGridEffect::SetGridEffectType(const FGridEffectType& EffectType)
{
	GridEffectType = FGridEffectType(EffectType);
}


const FGridEffectType UGridEffect::GetGridEffectType()
{
	return ((const FGridEffectType) GridEffectType);
}


bool UGridEffect::RequiresSelection()
{
	return GridEffectType.bRequiresSelection;
}


bool UGridEffect::CanPerform_Implementation(const TArray<FIntPoint>& PerformCoords)
{
	return true;
}


bool UGridEffect::Perform_Implementation(const TArray<FIntPoint>& PerformCoords)
{
	return true;
}