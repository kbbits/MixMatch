// Fill out your copyright notice in the Description page of Project Settings.

#include "GameEffect/GameEffectAddPlayerTurns.h"
#include "Kismet/GameplayStatics.h"
#include "MMPlayerController.h"
#include "MMPlayGrid.h"

/*
*/
UGameEffectAddPlayerTurns::UGameEffectAddPlayerTurns()
	: Super()
{
	BlockHandling = EMMBlockHandling::General;
}


void UGameEffectAddPlayerTurns::SetEffectParams_Implementation(const FGameEffectContext& EffectContext)
{
	NumToAdd = 1;
	if (EffectContext.FloatParams.Num() > 0) {
		NumToAdd = (int32)EffectContext.FloatParams[0];
	}
}


TArray<FIntPoint> UGameEffectAddPlayerTurns::GetEffectedCoords_Implementation(const FIntPoint SelectedCoords)
{
	return TArray<FIntPoint>();
}


bool UGameEffectAddPlayerTurns::CanTrigger_Implementation(const TArray<FIntPoint>& PerformCoords)
{
	AMMPlayerController* PC = Cast<AMMPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (PC == nullptr) { return false; }
	AMMPlayGrid* Grid = PC->GetCurrentGrid();
	if (Grid == nullptr) { return false; }
	return true;
}


bool UGameEffectAddPlayerTurns::BeginEffect_Implementation(const TArray<FIntPoint>& PerformCoords)
{
	if (NumToAdd == 0) { return true; }
	AMMPlayerController* PC = Cast<AMMPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (!IsValid(PC)) { return false; }
	AMMPlayGrid* Grid = PC->GetCurrentGrid();
	if (!IsValid(Grid)) { return false;	}
	Grid->MaxPlayerMovesCount += NumToAdd;
	return true;
}
