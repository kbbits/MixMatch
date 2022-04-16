// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Delegates/Delegate.h"
#include "PlayerSaveData.h"
#include "PersistentDataComponent.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerDataLoaded);

/* 
* This class manages the persistent data and save/load of player data.
*/
UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MIXMATCH_API UPersistentDataComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UPersistentDataComponent();

public:

	// Delegate event when player data has been loaded.
	UPROPERTY(BlueprintAssignable, Category = "EventDispatchers")
		FOnPlayerDataLoaded OnPlayerDataLoaded;

protected:

	static const FString LocalPlayerFilenameSuffix;

// ##### Functions

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	FString GetLocalPlayerSaveFilename(const FGuid& PlayerGuid);

public:	

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


	// Player Save Data

	FString GetPlayerSaveFilename(); 

	// returns a list of all save game filenames for local profiles in /Saved/SaveGames folder.
	UFUNCTION(BlueprintPure)
		static TArray<FString> GetAllSaveProfileFilenames();

	// returns all save games for local profiles in /Saved/SaveGames folder as FPlayerSaveData structs
	UFUNCTION(BlueprintCallable)
		static TArray<FPlayerSaveData> GetAllSaveProfileData();

	// Save the player's data.
	UFUNCTION(BlueprintCallable)
		void ServerSavePlayerData();

	// Load the player's data using the PlayerController PlayerGuid.
	UFUNCTION(BlueprintCallable)
		void ServerLoadPlayerData();

	// Load the player's data.
	UFUNCTION(BlueprintCallable)
		void ServerLoadPlayerDataByGuid(const FGuid ForcePlayerGuid);

	// [Client]
	// Echo loaded player data back to client
	//UFUNCTION(Client, Reliable, BlueprintCallable, WithValidation)
	//	void ClientEchoLoadPlayerData(const FPlayerSaveData PlayerSaveData);

};
