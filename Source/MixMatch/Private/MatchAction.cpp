// Fill out your copyright notice in the Description page of Project Settings.

#include "MatchAction.h"

/*
*/
UMatchAction::UMatchAction()
	: Super()
{
}


bool UMatchAction::Perform_Implementation(const FBlockMatch& Match, const FMatchActionType& MatchActionType)
{
	return true;
}