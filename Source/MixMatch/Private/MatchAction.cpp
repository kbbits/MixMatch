// Fill out your copyright notice in the Description page of Project Settings.

#include "MatchAction.h"
#include "MMBlock.h"

/*
*/
UMatchAction::UMatchAction()
	: Super()
{
}


//bool UMatchAction::Perform_Implementation(const UBlockMatch* Match, const FMatchActionType& MatchActionType, const AMMBlock* TriggeringBlock)
//{
//	return true;
//}


bool UMatchAction::PerformGameEffect_Implementation(const FGameEffectContext EffectContext, const AMMBlock* TriggeringBlock)
{
	if (!IsValid(TriggeringBlock)) { return false; }
	
	return false;
}