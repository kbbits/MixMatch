#pragma once


UENUM(BlueprintType)
enum class EMMGridState : uint8
{
	Normal		UMETA(DispalyName = "Normal"),
	Moving		UMETA(DisplayName = "Moving"),
	Matching	UMETA(DisplayName = "Matching"),
	Settling	UMETA(DispalyName = "Settling")
};


UENUM(BlueprintType)
enum class EMMGridLockState : uint8
{
	// Grid has not been checked for locked state
	Unchecked	UMETA(DisplayName = "Unchecked"),
	// Currently in process of checking the grid for locked state
	Checking	UMETA(DisplayName = "Checking"),
	// At least one matching move exists in the grid
	NotLocked	UMETA(DisplayName = "Not Locked"),
	// No moves that result in a match exist
	Locked		UMETA(DisplayName = "Locked")
};

UENUM(BlueprintType)
enum class EMMDirection : uint8
{
	North		UMETA(DisplayName = "North"),
	NorthEast	UMETA(DispalyName = "NorthEast"),
	East		UMETA(DisplayName = "East"),
	SouthEast	UMETA(DispalyName = "SouthEast"),
	South		UMETA(DisplayName = "South"),
	SouthWest	UMETA(DispalyName = "SouthWest"),
	West		UMETA(DisplayName = "West"),
	NorthWest	UMETA(DispalyName = "NorthWest")
}; 


UENUM(BlueprintType)
enum class EMMOrientation : uint8
{
	Horizontal	UMETA(DisplayName = "Horizontal"),
	Vertical	UMETA(DisplayName = "Vertical"),
	Unknown		UMETA(DisplayName = "Unknown")
};


UENUM(BlueprintType)
enum class EMMBlockQuantity : uint8
{
	PerMatch 		UMETA(DisplayName = "Per Match"),
	PerBlock		UMETA(DisplayName = "Per Block"),
	PerBlockOverMin	UMETA(DisplayName = "Per Block Over Min")
};


UENUM(BlueprintType)
enum class EMMBlockLocation : uint8
{
	/* Spawn block above top row */
	TopRow			UMETA(DisplayName = "Top Row"),
	/* Spawn block in center of a block match */
	InMatch			UMETA(DisplayName = "In Match"),
	/* Spawn block in cell that contained the block moved by player. If match contains no block moved by player, use InMatch behavior. */
	OnMovedBlock	UMETA(DisplayName = "On Moved Block"),
	/* Spawn block in a random, unoccupied cell. */
	//RandomOpen		UMETA(DisplayName = "Random Open"),
	/* Spawn block in a random cell, excluding cells with immobile blocks. New block replaces any existing block. */
	//RandomReplace	UMETA(DisplayName = "Random Replace")
};
