
#include "BlockMatch.h"
#include "..\MixMatch.h"
#include "MMBlock.h"

/*
*/
UBlockMatch::UBlockMatch()
	: Super()
{
}


void UBlockMatch::Sort(const bool bForceSort)
{
	if ((bSorted && !bForceSort) || Blocks.Num() <= 1) { return; }
	FIntPoint MinX = FIntPoint(-1, -1);
	FIntPoint MinY = FIntPoint(-1, -1);
	int32 MatchSize = Blocks.Num();
	TArray<AMMBlock*> TmpBlocks;
	int32 StartIndex = -1;
	for (AMMBlock* Block : Blocks)
	{
		FIntPoint Coords = Block->GetCoords();
		if (Orientation == EMMOrientation::Unknown && MinX.X >= 0 && MinY.Y >= 0)
		{
			Orientation = Coords.Y != MinY.Y ? EMMOrientation::Vertical : EMMOrientation::Horizontal;
		}
		if (MinX.X < 0 || Coords.X < MinX.X) {
			MinX = Coords;
		}
		if (MinY.Y < 0 || Coords.Y < MinY.Y) {
			MinY = Coords;
		}
	}
	StartCoords = Orientation == EMMOrientation::Horizontal ? MinX : MinY;
	EndCoords = Orientation == EMMOrientation::Horizontal ? StartCoords + FIntPoint(MatchSize - 1, 0) : StartCoords + FIntPoint(0, MatchSize - 1);
	TmpBlocks.Empty();
	TmpBlocks.SetNum(Blocks.Num());
	for (int32 i = 0; i < Blocks.Num(); i++)
	{
		if (Orientation == EMMOrientation::Horizontal) {
			TmpBlocks[Blocks[i]->GetCoords().X - MinX.X] = Blocks[i];
		}
		else {
			TmpBlocks[Blocks[i]->GetCoords().Y - MinY.Y] = Blocks[i];
		}

	}
	if (TmpBlocks.Num() > Blocks.Num()) {
		UE_LOG(LogMMGame, Log, TEXT("BlockMatch::Sort - TmpBlocks length %d"), TmpBlocks.Num());
	}
	Blocks.Empty();
	for (AMMBlock* TmpBlock : TmpBlocks) {
		if (TmpBlock) {
			Blocks.Add(TmpBlock);
		}
		else {
			break;
		}
	}
	bSorted = true;
}
