
#include "BlockMatch.h"
#include "..\MixMatch.h"
#include "MMPlayGrid.h"
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
	TArray<AMMBlock*> SortedBlocks;
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
	TmpBlocks = Blocks;
	SortedBlocks.SetNum(TmpBlocks.Num());
	for (int32 i = 0; i < TmpBlocks.Num(); i++)
	{
		if (Orientation == EMMOrientation::Horizontal) {
			SortedBlocks[TmpBlocks[i]->GetCoords().X - MinX.X] = TmpBlocks[i];
		}
		else {
			SortedBlocks[TmpBlocks[i]->GetCoords().Y - MinY.Y] = TmpBlocks[i];
		}
	}
	Blocks.Empty();
	for (AMMBlock* TmpBlock : SortedBlocks) {
		if (TmpBlock) {
			Blocks.Add(TmpBlock);
		}
		else {
			break;
		}
	}
	bSorted = true;
}


void UBlockMatch::Reset()
{
	Blocks.Empty();
	StartCoords = FIntPoint::NoneValue;
	EndCoords = FIntPoint::NoneValue;
	bSorted = false;
	TotalScore = 0;
	TotalGoods.Empty();
	Orientation = EMMOrientation::Unknown;
}


FVector UBlockMatch::GetWorldLocation()
{
	if (Blocks.Num() == 0) return FVector(-1.f, -1.f, -1.f);
	AMMPlayGrid* Grid = Blocks[0]->Grid();
	if (Grid == nullptr) return FVector(-1.f, -1.f, -1.f);
	Sort();
	return Grid->GridFloatCoordsToWorldLocation(FVector2D((float)(StartCoords.X + EndCoords.X) / 2.f, (float)(StartCoords.Y + EndCoords.Y) / 2.f));
}