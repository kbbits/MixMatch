// Fill out your copyright notice in the Description page of Project Settings.


#include "MMMath.h"

const TArray<EMMDirection> UMMMath::OrthogonalDirections = { EMMDirection::North, EMMDirection::East, EMMDirection::South, EMMDirection::West };
const TArray<EMMDirection> UMMMath::DiagonalDirections = { EMMDirection::NorthEast, EMMDirection::SouthEast, EMMDirection::SouthWest, EMMDirection::NorthWest };

bool UMMMath::IsOrthogonal(const EMMDirection Direction)
{
	return OrthogonalDirections.Contains(Direction);
}


bool UMMMath::IsDiagonal(const EMMDirection Direction)
{
	return DiagonalDirections.Contains(Direction);
}


FVector2D UMMMath::DirectionToOffsetVector(const EMMDirection Direction)
{
	switch (Direction)
	{
	case EMMDirection::North:
		return FVector2D(0.f, 1.f);
	case EMMDirection::NorthEast:
		return FVector2D(1.f, 1.f);
	case EMMDirection::East:
		return FVector2D(1.f, 0.f);
	case EMMDirection::SouthEast:
		return FVector2D(1.f, -1.f);
	case EMMDirection::South:
		return FVector2D(0.f, -1.f);
	case EMMDirection::SouthWest:
		return FVector2D(-1.f, -1.f);
	case EMMDirection::West:
		return FVector2D(-1.f, 0.f);
	case EMMDirection::NorthWest:
		return FVector2D(-1.f, 1.0f);
	default:
		return FVector2D(0.0f, 0.0f);
	}
	return FVector2D(0.f, 0.f);
}


FIntPoint UMMMath::DirectionToOffset(const EMMDirection Direction)
{
	switch (Direction)
	{
	case EMMDirection::North:
		return FIntPoint(0, 1);
	case EMMDirection::NorthEast:
		return FIntPoint(1, 1);
	case EMMDirection::East:
		return FIntPoint(1, 0);
	case EMMDirection::SouthEast:
		return FIntPoint(1, -1);
	case EMMDirection::South:
		return FIntPoint(0, -1);
	case EMMDirection::SouthWest:
		return FIntPoint(-1, -1);
	case EMMDirection::West:
		return FIntPoint(-1, 0);
	case EMMDirection::NorthWest:
		return FIntPoint(-1, -1);
	default:
		return FIntPoint(0, 0);
	}
	return FIntPoint(0, 0);
}


EMMDirection UMMMath::OppositeDirection(const EMMDirection Direction)
{
	switch (Direction)
	{
	case EMMDirection::North :
		return EMMDirection::South;
	case EMMDirection::NorthEast :
		return EMMDirection::SouthWest;
	case EMMDirection::East:
		return EMMDirection::West;
	case EMMDirection::SouthEast:
		return EMMDirection::NorthWest;
	case EMMDirection::South:
		return EMMDirection::North;
	case EMMDirection::SouthWest:
		return EMMDirection::NorthEast;
	case EMMDirection::West:
		return EMMDirection::East;
	case EMMDirection::NorthWest:
		return EMMDirection::SouthEast;
	}
	return EMMDirection::North;
}

bool UMMMath::DirectionIsHorizontal(const EMMDirection Direction)
{
	return (Direction == EMMDirection::East || Direction == EMMDirection::West);
}


bool UMMMath::DirectionIsVertical(const EMMDirection Direction)
{
	return (Direction == EMMDirection::South || Direction == EMMDirection::North);
}


bool UMMMath::CoordsAdjacent(const FIntPoint CoordsA, const FIntPoint CoordsB)
{
	FIntPoint DistSquare = (CoordsA - CoordsB) * (CoordsA - CoordsB);
	return ((DistSquare.X == 1 && DistSquare.Y == 0) || (DistSquare.X == 0 && DistSquare.Y == 1));	
}