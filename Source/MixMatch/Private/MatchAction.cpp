// Fill out your copyright notice in the Description page of Project Settings.

#include "MatchAction.h"

/*
*/
UMatchAction::UMatchAction()
	: Super()
{
}


bool UMatchAction::Perform_Implementation(const UBlockMatch* Match, const FMatchActionType& MatchActionType, const AMMBlock* TriggeringBlock)
{
	return true;
}