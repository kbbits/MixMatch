// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryActorComponent.h"
#include "GameFramework/PlayerController.h"

// Sets default values for this component's properties
UInventoryActorComponent::UInventoryActorComponent()
{
	//SetIsReplicated(false);
	SetIsReplicatedByDefault(false);
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


//void UInventoryActorComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
//{
//	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
//
//	DOREPLIFETIME(UInventoryActorComponent, );
//}


// Called when the game starts
void UInventoryActorComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


bool UInventoryActorComponent::ShouldUpdateClient()
{
	bool bUpdateClient = false;
	APlayerController* TmpPC = Cast<APlayerController>(GetOwner());
	if (TmpPC)
	{
		bUpdateClient = !TmpPC->IsLocalController();
	} 
	else 
	{
		ENetMode NetMode = GetNetMode();
		bUpdateClient = NetMode != NM_Client && NetMode != NM_Standalone;
	}
	//UE_LOG(LogTRGame, Log, TEXT("InventoryActorComponent - ShouldUpdateClient: %s."), bUpdateClient ? TEXT("True") : TEXT("False"));
	return bUpdateClient;
}


// Called every frame
void UInventoryActorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}


bool UInventoryActorComponent::AddSubtractGoods(const FGoodsQuantity& GoodsDelta, const bool bNegateGoodsQuantities, float& CurrentQuantity, const bool bAddToSnapshot)
{
	FGoodsQuantity GoodsQuantity;
	float NetQuantity;
	int32 Index = Inventory.IndexOfByKey(GoodsDelta.Name);
	if (Index == INDEX_NONE)
	{
		GoodsQuantity.Name = GoodsDelta.Name;
		GoodsQuantity.Quantity = 0.0f;
	}
	else
	{
		FGoodsQuantity& TmpGoodsQuantity = Inventory[Index];
		GoodsQuantity = TmpGoodsQuantity;
	}
	if (GoodsDelta.Quantity == 0.0f) 
	{ 
		CurrentQuantity = GoodsQuantity.Quantity;
		return true; 
	}
	NetQuantity = GoodsQuantity.Quantity + (bNegateGoodsQuantities ? GoodsDelta.Quantity * -1.0f : GoodsDelta.Quantity);
	if (NetQuantity < 0.0f) 
	{ 
		CurrentQuantity = GoodsQuantity.Quantity;
		return false; 
	}
	else
	{
		CurrentQuantity = NetQuantity;
		ServerAddSubtractGoods(GoodsDelta, bNegateGoodsQuantities, bAddToSnapshot);
		return true;
	}
}


void UInventoryActorComponent::ServerAddSubtractGoods_Implementation(const FGoodsQuantity& GoodsDelta, const bool bNegateGoodsQuantities, const bool bAddToSnapshot)
{
	if (GoodsDelta.Quantity == 0.0f) { return; }

	FGoodsQuantity GoodsQuantity;
	FGoodsQuantity SnapshotDelta;
	TArray<FGoodsQuantity> TmpSnapshotArray;
	float NetQuantity;
	int32 Index = Inventory.IndexOfByKey(GoodsDelta.Name);
	if (Index == INDEX_NONE)
	{
		GoodsQuantity.Name = GoodsDelta.Name;
		GoodsQuantity.Quantity = 0.0f;
	}
	else
	{
		FGoodsQuantity& TmpGoodsQuantity = Inventory[Index];
		GoodsQuantity = TmpGoodsQuantity;
	}
	//UE_LOG(LogTRGame, Log, TEXT("InventoryActorComponent - ServerAddSubtractGoods current: %s: %d."), *GoodsQuantity.Name.ToString(), (int32)GoodsQuantity.Quantity);
	NetQuantity = GoodsQuantity.Quantity + (bNegateGoodsQuantities ? GoodsDelta.Quantity * -1.0f : GoodsDelta.Quantity);
	if (NetQuantity < 0.0f) { return; }
	else
	{
		GoodsQuantity.Quantity = NetQuantity;
		if (Index == INDEX_NONE)
		{
			Inventory.Add(GoodsQuantity);
		}
		else
		{
			Inventory[Index].Quantity = GoodsQuantity.Quantity;
		}
		if (bAddToSnapshot)
		{
			SnapshotDelta.Name = GoodsDelta.Name;
			SnapshotDelta.Quantity = bNegateGoodsQuantities ? GoodsDelta.Quantity * -1.0f : GoodsDelta.Quantity;
			Index = SnapshotInventory.IndexOfByKey(SnapshotDelta.Name);
			if (Index == INDEX_NONE)
			{
				SnapshotInventory.Add(SnapshotDelta);
				TmpSnapshotArray.Add(SnapshotDelta);
			}
			else
			{
				SnapshotInventory[Index].Quantity = SnapshotInventory[Index].Quantity + SnapshotDelta.Quantity;
				TmpSnapshotArray.Add(SnapshotInventory[Index]);
			}
		}
		TArray<FGoodsQuantity> TmpTotalGoodsArray;
		TmpTotalGoodsArray.Add(GoodsQuantity);
		TArray<FGoodsQuantity> TmpDeltaGoodsArray;
		TmpDeltaGoodsArray.Add(FGoodsQuantity(GoodsDelta.Name, (bNegateGoodsQuantities ? GoodsDelta.Quantity * -1.0f : GoodsDelta.Quantity)));
		OnInventoryChanged.Broadcast(TmpDeltaGoodsArray, TmpTotalGoodsArray, TmpSnapshotArray);
		if (ShouldUpdateClient())
		{
			// Update client
			ClientUpdateInventoryQuantity(GoodsQuantity, SnapshotDelta);
		}
	}
	//UE_LOG(LogTRGame, Log, TEXT("InventoryActorComponent - ServerAddSubtractGoods new: %s: %d."), *GoodsQuantity.Name.ToString(), (int32)GoodsQuantity.Quantity);
}


bool UInventoryActorComponent::ServerAddSubtractGoods_Validate(const FGoodsQuantity& GoodsDelta, const bool bNegateGoodsQuantities, const bool bAddToSnapshot)
{
	return true;
}


bool UInventoryActorComponent::AddSubtractGoodsArray(const TArray<FGoodsQuantity>& GoodsDeltas, const bool bNegateGoodsQuantities, TArray<FGoodsQuantity>& CurrentQuantities, const bool bAddToSnapshot)
{
	TArray<FGoodsQuantity> CurrentGoods;
	TArray<FGoodsQuantity> NewGoods;
	FGoodsQuantity GoodsQuantity;
	float NetQuantity;
	int32 Index = INDEX_NONE;
	bool bCanMakeUpdate = true;
	for (FGoodsQuantity TmpGoodsDelta : GoodsDeltas)
	{
		Index = Inventory.IndexOfByKey(TmpGoodsDelta.Name);
		if (Index == INDEX_NONE)
		{
			GoodsQuantity.Name = TmpGoodsDelta.Name;
			GoodsQuantity.Quantity = 0.0f;
		}
		else
		{
			FGoodsQuantity& TmpGoodsQuantity = Inventory[Index];
			GoodsQuantity = TmpGoodsQuantity;
		}
		CurrentGoods.Add(GoodsQuantity);
		//UE_LOG(LogTRGame, Log, TEXT("InventoryActorComponent - AddSubtractGoodsArray current: %s: %d."), *GoodsQuantity.Name.ToString(), (int32)GoodsQuantity.Quantity);
		NetQuantity = GoodsQuantity.Quantity + (bNegateGoodsQuantities ? TmpGoodsDelta.Quantity * -1.0f : TmpGoodsDelta.Quantity);
		if (NetQuantity < 0.0f) {
			bCanMakeUpdate = false;
			NewGoods.Add(GoodsQuantity);
		}
		else
		{
			NewGoods.Add(FGoodsQuantity(GoodsQuantity.Name, NetQuantity));
			//UE_LOG(LogTRGame, Log, TEXT("InventoryActorComponent - AddSubtractGoodsArray new: %s: %d."), *GoodsQuantity.Name.ToString(), (int32)NetQuantity);
		}
	}
	if (bCanMakeUpdate)
	{
		CurrentQuantities = NewGoods;
		ServerAddSubtractGoodsArray(GoodsDeltas, bNegateGoodsQuantities, bAddToSnapshot);
		return true;
	}
	else
	{
		CurrentQuantities = CurrentGoods;
		return false;
	}
}


void UInventoryActorComponent::ServerAddSubtractGoodsArray_Implementation(const TArray<FGoodsQuantity>& GoodsDeltas, const bool bNegateGoodsQuantities, const bool bAddToSnapshot)
{
	TArray<FGoodsQuantity> NewGoods;
	TArray<FGoodsQuantity> SnapshotDeltas;
	TMap<FName, FGoodsQuantity> SnapshotGoodsMap;
	TArray<FGoodsQuantity> NewGoodsDeltas;
	FGoodsQuantity GoodsQuantity;
	float NetQuantity;
	int32 Index = INDEX_NONE;

	for (FGoodsQuantity TmpGoodsDelta : GoodsDeltas)
	{
		Index = Inventory.IndexOfByKey(TmpGoodsDelta.Name);
		if (Index == INDEX_NONE)
		{
			GoodsQuantity.Name = TmpGoodsDelta.Name;
			GoodsQuantity.Quantity = 0.0f;
		}
		else
		{
			FGoodsQuantity& TmpGoodsQuantity = Inventory[Index];
			GoodsQuantity = TmpGoodsQuantity;
		}
		//UE_LOG(LogTRGame, Log, TEXT("InventoryActorComponent - ServerAddSubtractGoodsArray curret New item: %s, %s: %d."), Index == INDEX_NONE ? TEXT("True") : TEXT("False"), *GoodsQuantity.Name.ToString(), (int32)GoodsQuantity.Quantity);
		NetQuantity = GoodsQuantity.Quantity + (bNegateGoodsQuantities ? TmpGoodsDelta.Quantity * -1.0f : TmpGoodsDelta.Quantity);
		if (NetQuantity < 0.0f) {
			return;
		}
		else
		{
			NewGoods.Add(FGoodsQuantity(GoodsQuantity.Name, NetQuantity));
		}
		NewGoodsDeltas.Add(FGoodsQuantity(TmpGoodsDelta.Name, (bNegateGoodsQuantities ? TmpGoodsDelta.Quantity * -1.0f : TmpGoodsDelta.Quantity)));
	}
	for (FGoodsQuantity NewGoodsItem : NewGoods)
	{
		Index = Inventory.IndexOfByKey(NewGoodsItem.Name);
		if (Index == INDEX_NONE)
		{
			Inventory.Add(NewGoodsItem);
		}
		else
		{
			Inventory[Index].Quantity = NewGoodsItem.Quantity;
		}
		//UE_LOG(LogTRGame, Log, TEXT("InventoryActorComponent - ServerAddSubtractGoodsArray updated New item: %s, %s: %d."), Index == INDEX_NONE ? TEXT("True") : TEXT("False"), *NewGoodsItem.Name.ToString(), (int32)NewGoodsItem.Quantity);
		
	}
	if (bAddToSnapshot)
	{
		for (FGoodsQuantity GoodsDeltaItem : GoodsDeltas)
		{
			FGoodsQuantity SnapshotDelta = FGoodsQuantity(GoodsDeltaItem.Name, bNegateGoodsQuantities ? GoodsDeltaItem.Quantity * -1.0f : GoodsDeltaItem.Quantity);
			SnapshotDeltas.Add(SnapshotDelta);
			Index = SnapshotInventory.IndexOfByKey(SnapshotDelta.Name);
			float NewQuantity = 0.f;
			if (Index == INDEX_NONE)
			{
				SnapshotInventory.Add(SnapshotDelta);
				NewQuantity = SnapshotDelta.Quantity;
			}
			else
			{
				SnapshotInventory[Index].Quantity = SnapshotInventory[Index].Quantity + SnapshotDelta.Quantity;
				NewQuantity = SnapshotInventory[Index].Quantity;
			}
			if (SnapshotGoodsMap.Contains(SnapshotDelta.Name)) {
				SnapshotGoodsMap[SnapshotDelta.Name].Quantity = NewQuantity;
			}
			else {
				SnapshotGoodsMap.Add(SnapshotDelta.Name, FGoodsQuantity(SnapshotDelta.Name, NewQuantity));
			}
		}
	}
	TArray<FGoodsQuantity> TmpSnapshotArray;
	SnapshotGoodsMap.GenerateValueArray(TmpSnapshotArray);
	OnInventoryChanged.Broadcast(NewGoodsDeltas, NewGoods, TmpSnapshotArray);
	if (ShouldUpdateClient())
	{
		ClientUpdateInventoryQuantities(NewGoods, SnapshotDeltas);
	}
}


bool UInventoryActorComponent::ServerAddSubtractGoodsArray_Validate(const TArray<FGoodsQuantity>& GoodsDeltas, const bool bNegateGoodsQuantities, const bool bAddToSnapshot)
{
	return true;
}


void UInventoryActorComponent::ServerSetInventory_Implementation(const TArray<FGoodsQuantity>& NewGoods, const TArray<FGoodsQuantity>& NewSnapshotGoods)
{
	Inventory.Empty(NewGoods.Num());
	if (NewGoods.Num() > 0) 
	{
		Inventory.Append(NewGoods);
	}
	SnapshotInventory.Empty(NewSnapshotGoods.Num());
	if (NewSnapshotGoods.Num() > 0) 
	{
		SnapshotInventory.Append(NewSnapshotGoods);
	}
	if (ShouldUpdateClient())
	{
		ClientSetInventory(NewGoods, NewSnapshotGoods);
	}
}


bool UInventoryActorComponent::ServerSetInventory_Validate(const TArray<FGoodsQuantity>& NewGoods, const TArray<FGoodsQuantity>& NewSnapshotGoods)
{
	return true;
}


void UInventoryActorComponent::ClientSetInventory_Implementation(const TArray<FGoodsQuantity>& NewGoods, const TArray<FGoodsQuantity>& NewSnapshotGoods)
{
	Inventory.Empty(NewGoods.Num());
	if (NewGoods.Num() > 0) 
	{
		Inventory.Append(NewGoods);
	}
	SnapshotInventory.Empty(NewSnapshotGoods.Num());
	if (NewSnapshotGoods.Num() > 0) 
	{
		SnapshotInventory.Append(NewSnapshotGoods);
	}
}


bool UInventoryActorComponent::ClientSetInventory_Validate(const TArray<FGoodsQuantity>& NewGoods, const TArray<FGoodsQuantity>& NewSnapshotGoods)
{
	return true;
}


void UInventoryActorComponent::ClientUpdateInventoryQuantity_Implementation(const FGoodsQuantity NewQuantity, const FGoodsQuantity SnapshotDelta)
{
	FGoodsQuantity GoodsDelta(NewQuantity.Name, 0.f);
	int32 Index = Inventory.IndexOfByKey(NewQuantity.Name);
	TArray<FGoodsQuantity> TmpSnapshotArray;
	//UE_LOG(LogTRGame, Log, TEXT("InventoryActorComponent - ClientUpdateInventoryQuantity New item: %s, %s: %d."), Index == INDEX_NONE ? TEXT("True") : TEXT("False"), *NewQuantity.Name.ToString(), (int32)NewQuantity.Quantity);
	if (Index == INDEX_NONE)
	{
		Inventory.Add(NewQuantity);
		GoodsDelta.Quantity = NewQuantity.Quantity;
	}
	else
	{
		GoodsDelta.Quantity = NewQuantity.Quantity - Inventory[Index].Quantity;
		Inventory[Index].Quantity = NewQuantity.Quantity;
	}
	if (!SnapshotDelta.Name.IsNone() && SnapshotDelta.Quantity != 0.0f)
	{
		Index = SnapshotInventory.IndexOfByKey(SnapshotDelta.Name);
		if (Index == INDEX_NONE)
		{
			SnapshotInventory.Add(SnapshotDelta);
			TmpSnapshotArray.Add(SnapshotDelta);
		}
		else
		{
			SnapshotInventory[Index].Quantity = SnapshotInventory[Index].Quantity + SnapshotDelta.Quantity;
			TmpSnapshotArray.Add(SnapshotInventory[Index]);
		}
	}
	TArray<FGoodsQuantity> TmpGoodsArray;
	TmpGoodsArray.Add(NewQuantity);
	TArray<FGoodsQuantity> TmpDeltaGoodsArray;
	TmpDeltaGoodsArray.Add(GoodsDelta);
	OnInventoryChanged.Broadcast(TmpDeltaGoodsArray, TmpGoodsArray, TmpSnapshotArray);
}


bool UInventoryActorComponent::ClientUpdateInventoryQuantity_Validate(const FGoodsQuantity NewQuantity, const FGoodsQuantity SnapshotDelta)
{
	return true;
}


void UInventoryActorComponent::ClientUpdateInventoryQuantities_Implementation(const TArray<FGoodsQuantity>& NewQuantities, const TArray<FGoodsQuantity>& SnapshotDeltas)
{
	int32 Index = INDEX_NONE;
	TArray<FGoodsQuantity> GoodsDeltas;
	TMap<FName, FGoodsQuantity> SnapshotGoodsMap;
	for (FGoodsQuantity NewGoodsItem : NewQuantities)
	{
		Index = Inventory.IndexOfByKey(NewGoodsItem.Name);
		//UE_LOG(LogTRGame, Log, TEXT("InventoryActorComponent - ClientUpdateInventoryQuantities New item: %s, %s: %d."), Index == INDEX_NONE ? TEXT("True") : TEXT("False"), *NewGoodsItem.Name.ToString(), (int32)NewGoodsItem.Quantity);
		if (Index == INDEX_NONE)
		{
			Inventory.Add(NewGoodsItem);
			GoodsDeltas.Add(FGoodsQuantity(NewGoodsItem.Name, NewGoodsItem.Quantity));
		}
		else
		{
			GoodsDeltas.Add(FGoodsQuantity(NewGoodsItem.Name, NewGoodsItem.Quantity - Inventory[Index].Quantity));
			Inventory[Index].Quantity = NewGoodsItem.Quantity;
		}
	}
	for (FGoodsQuantity SnapshotDelta : SnapshotDeltas)
	{
		if (!SnapshotDelta.Name.IsNone() && SnapshotDelta.Quantity != 0.0f)
		{
			float NewQuantity = 0.f;
			Index = SnapshotInventory.IndexOfByKey(SnapshotDelta.Name);
			if (Index == INDEX_NONE)
			{
				SnapshotInventory.Add(SnapshotDelta);
				NewQuantity = SnapshotDelta.Quantity;
			}
			else
			{
				SnapshotInventory[Index].Quantity = SnapshotInventory[Index].Quantity + SnapshotDelta.Quantity;
				NewQuantity = SnapshotInventory[Index].Quantity;
			}
			if (SnapshotGoodsMap.Contains(SnapshotDelta.Name)) {
				SnapshotGoodsMap[SnapshotDelta.Name].Quantity = NewQuantity;
			}
			else {
				SnapshotGoodsMap.Add(SnapshotDelta.Name, FGoodsQuantity(SnapshotDelta.Name, NewQuantity));
			}
		}
	}
	TArray<FGoodsQuantity> TmpSnapshotArray;
	SnapshotGoodsMap.GenerateValueArray(TmpSnapshotArray);
	OnInventoryChanged.Broadcast(GoodsDeltas, NewQuantities, TmpSnapshotArray);
}


bool UInventoryActorComponent::ClientUpdateInventoryQuantities_Validate(const TArray<FGoodsQuantity>& NewQuantities, const TArray<FGoodsQuantity>& SnapshotDeltas)
{
	return true;
}


float UInventoryActorComponent::GetGoodsCount(const FName GoodsName)
{
	int32 Index = Inventory.IndexOfByKey(GoodsName);
	if (Index == INDEX_NONE)
	{
		return 0.0;
	}
	else
	{
		return Inventory[Index].Quantity;
	}
}


void UInventoryActorComponent::GetAllGoods(TArray<FGoodsQuantity>& AllGoods)
{
	AllGoods.Empty(Inventory.Num());
	AllGoods.Append(Inventory);
}


void UInventoryActorComponent::GetSaveableGoods(TArray<FGoodsQuantity>& AllSaveableGoods)
{	
	if (UnsaveableGoodsFilters.Num() == 0)
	{
		AllSaveableGoods.Empty(Inventory.Num());
		AllSaveableGoods.Append(Inventory);
	}
	else
	{
		bool bSaveable;
		AllSaveableGoods.Empty();
		for (FGoodsQuantity Goods : Inventory)
		{
			bSaveable = true;
			for (FString FilterStr : UnsaveableGoodsFilters)
			{
				if (Goods.Name.ToString().Contains(FilterStr))
				{
					bSaveable = false;
					break;
				}
			}
			if (bSaveable) { AllSaveableGoods.Add(Goods); }
		}
	}
}


bool UInventoryActorComponent::HasGoods(const FGoodsQuantity Goods, float& CurrentQuantity)
{
	CurrentQuantity = GetGoodsCount(Goods.Name);
	return CurrentQuantity >= Goods.Quantity;
}


bool UInventoryActorComponent::HasAllGoods(const TArray<FGoodsQuantity> Goods, TArray<FGoodsQuantity>& CurrentQuantities)
{
	bool bHasAll = true;
	float TmpCurQuantity = 0.0f;
	CurrentQuantities.Empty(Goods.Num());
	for (FGoodsQuantity TmpGQ : Goods)
	{
		if (!HasGoods(TmpGQ, TmpCurQuantity)) { bHasAll = false; }
		CurrentQuantities.Add(FGoodsQuantity(TmpGQ.Name, TmpCurQuantity));
	}
	return bHasAll;
}


bool UInventoryActorComponent::IsEmpty()
{
	return Inventory.Num() == 0;
}


void UInventoryActorComponent::ServerClearSnapshotInventory_Implementation()
{
	SnapshotInventory.Empty();
	if (ShouldUpdateClient())
	{
		// Update client
		ClientClearSnapshotInventory();
	}
}


bool UInventoryActorComponent::ServerClearSnapshotInventory_Validate()
{
	return true;
}


void UInventoryActorComponent::ClientClearSnapshotInventory_Implementation()
{
	SnapshotInventory.Empty();
}


bool UInventoryActorComponent::ClientClearSnapshotInventory_Validate()
{
	return true;
}


void UInventoryActorComponent::GetSnapshotGoods(TArray<FGoodsQuantity>& AllSnapshotGoods)
{
	AllSnapshotGoods.Append(SnapshotInventory);
}


bool UInventoryActorComponent::SnapshotInventoryIsEmpty()
{
	return SnapshotInventory.Num() == 0;
}