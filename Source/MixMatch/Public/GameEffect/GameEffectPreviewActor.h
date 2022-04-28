#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameEffectPreviewActor.generated.h"

/** The visual/audio FX for a game effect. */
UCLASS(minimalapi)
class AGameEffectPreviewActor : public AActor
{
	GENERATED_BODY()

	/** Scene root component */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class USceneComponent* SceneRoot;

public:
	AGameEffectPreviewActor();
	
	/** Grid that owns us */
	UPROPERTY(BlueprintReadOnly)
	class AMMPlayGrid* OwningGrid;
		

public:
	/** Returns Root subobject **/
	FORCEINLINE class USceneComponent* GetSceneRoot() const { return SceneRoot; }

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void StartPreview();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void DestroyPreviewActor();
};



