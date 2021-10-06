// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Goods/GoodsQuantity.h"
#include "InventoryActorComponent.generated.h"

// Event dispatcher for when CurrentValue changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnInventoryChanged, const TArray<FGoodsQuantity>&, GoodsDeltas, const TArray<FGoodsQuantity>&, ChangedTotals, const TArray<FGoodsQuantity>&, SnapshotChangedTotals);

/*
* Manages an actor's inventory of goods. 
* Replication of inventory content is managed by all changes going through server. Each of those changes is 
* replicated to owning client via RPCs called from the ServerXxxx functions.
*/
UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MIXMATCH_API UInventoryActorComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UInventoryActorComponent();

	// Delegate event when inventory has changed.
	UPROPERTY(BlueprintAssignable, Category = "EventDispatchers")
	FOnInventoryChanged OnInventoryChanged;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (TitleProperty = "Name"))
	TArray<FGoodsQuantity> Inventory;

	UPROPERTY(BlueprintReadOnly, meta = (TitleProperty = "Name"))
	TArray<FGoodsQuantity> SnapshotInventory;

	// Any goods name that matches one of these strings will be filtered out of saveable goods in GeSaveableGoods()
	// TODO: This should probably be moved to a single location, e.g. GameMode.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FString> UnsaveableGoodsFilters;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	bool ShouldUpdateClient();

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// [Any]
	// Call this one to Add (or subtract) a quantity of goods from inventory. Returns true if adjustment could be made, false otherwise (ex: if amount to remove is > current inventory)
	//  bNegateGoodsQuantities - Set this to true to have each goods quantity multiplied by -1.0. (to simplify removing goods using postitive goods quantities)
	// Note: actual changes to inventory are made by replicated functions that this calls. (ServerAddSubtractGoods, ClientUpdateInventoryQuantity).
	UFUNCTION(BlueprintCallable)
	bool AddSubtractGoods(const FGoodsQuantity& GoodsDelta, const bool bNegateGoodsQuantities, float& CurrentQuantity, const bool bAddToSnapshot = true);

	// [Server]
	// Called from AddSubtractGoods()
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerAddSubtractGoods(const FGoodsQuantity& GoodsDelta, const bool bNegateGoodsQuantities, const bool bAddtoSnapshot = true);

	// [Any]
	// Call this to Add (or subtract) quantities of goods from inventory. Returns true if all adjustments could be made, false otherwise (ex: if amount to remove is > current inventory)
	//  bNegateGoodsQuantities - Set this to true to have each goods quantity multiplied by -1.0. (to simplify removing goods using postitive goods quantities)
	// Note: actual changes to inventory are made by replicated functions that this calls. (ServerAddSubtractGoodsArray, ClientUpdateInventoryQuantity).
	UFUNCTION(BlueprintCallable)
	bool AddSubtractGoodsArray(const TArray<FGoodsQuantity>& GoodsDeltas, const bool bNegateGoodsQuantities, TArray<FGoodsQuantity>& CurrentQuantities, const bool bAddToSnapshot = true);
	bool AddSubtractGoodsArray(const TArray<FGoodsQuantity>& GoodsDeltas, const bool bNegateGoodsQuantities, const bool bAddToSnapshot = true);

	// [Server]
	// Called from AddSubtractGoods()
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerAddSubtractGoodsArray(const TArray<FGoodsQuantity>& GoodsDeltas, const bool bNegateGoodsQuantities,  const bool bAddToSnapshot = true);

	// [Server]
	// Sets the contents of inventory. Usually don't need to call this manually - useful to reset state after a load. 
	// This will call client if needed to handle replication.
	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation)
	void ServerSetInventory(const TArray<FGoodsQuantity>& NewGoods, const TArray<FGoodsQuantity>& NewSnapshotGoods);

	// [Client]
	// Called from ServerSetInventory()
	// Updates client side quantities - sets quantity to new quantity also sets snapshot inventory to new snapshot quantities.
	UFUNCTION(Client, Reliable, WithValidation)
	void ClientSetInventory(const TArray<FGoodsQuantity>& NewGoods, const TArray<FGoodsQuantity>& NewSnapshotGoods);


	// [Client]
	// Called from ServerAddSubtractGoods()
	// Updates client side quantity - sets quantity to new quantity. Snapshot delta is added to existing snapshot quantities.
	UFUNCTION(Client, Reliable, WithValidation)
	void ClientUpdateInventoryQuantity(const FGoodsQuantity NewQuantity, const FGoodsQuantity SnapshotDelta);

	// [Client]
	// Called from ServerAddSubtractGoodsArray()
	// Updates client side quantities - sets quantity to new quantity. Snapshot deltas are added to existing snapshot quantities.
	UFUNCTION(Client, Reliable, WithValidation)
	void ClientUpdateInventoryQuantities(const TArray<FGoodsQuantity>& NewQuantities, const TArray<FGoodsQuantity>& SnapshotDeltas);

	// [Any]
	// Get the current quantity of the given goods.
	UFUNCTION(BlueprintPure)
	float GetGoodsCount(const FName GoodsName);

	// [Any]
	// Get the current quantities of all goods in inventory.
	UFUNCTION(BlueprintPure)
	void GetAllGoods(TArray<FGoodsQuantity>& AllGoods);

	// [Any]
	// Get all goods that pass the save block filters. See UnsaveableGoodsFilters.
	UFUNCTION(BlueprintPure)
	void GetSaveableGoods(TArray<FGoodsQuantity>& AllSaveableGoods);

	// [Any]
	// Check if the inventory contains the given goods
	UFUNCTION(BlueprintPure)
	bool HasGoods(const FGoodsQuantity Goods, float& CurrrentQuantity);

	// [Any]
	// Check that the inventory contains all of the given goods
	UFUNCTION(BlueprintPure)
	bool HasAllGoods(const TArray<FGoodsQuantity> Goods, TArray<FGoodsQuantity>& CurrrentQuantities);

	// [Any]
	// Returns true if inventory is empty.
	UFUNCTION(BlueprintPure)
	bool IsEmpty();
	
	// [Server]
	// Empties the snapshot inventory. 
	// This will call client if needed to handle replication.
	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation)
	void ServerClearSnapshotInventory();

	// [Client]
	// Should not need to call directly. Called by ServerClearSnapshotInventory.
	UFUNCTION(Client, Reliable, WithValidation)
	void ClientClearSnapshotInventory();

	// [Any]
	// Get goods quantities since last snapshot was started.
	UFUNCTION(BlueprintPure)
	void GetSnapshotGoods(TArray<FGoodsQuantity>& AllSnapshotGoods);

	// [Any]
	// Returns true if snapshot inventory is empty.
	UFUNCTION(BlueprintPure)
	bool SnapshotInventoryIsEmpty();

};
