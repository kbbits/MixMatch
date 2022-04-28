#include "Goods/UsableGoodsContext.h"
#include "Kismet/GameplayStatics.h"
#include "MixMatch/MixMatch.h"
#include "Goods/UsableGoodsType.h"
#include "MMGameMode.h"
#include "GameEffect/GameEffect.h"


UUsableGoodsContext::UUsableGoodsContext()
	: Super()
{
}


UUsableGoods* UUsableGoodsContext::GetUsableGoods() const
{
	return UsableGoods;
}


void UUsableGoodsContext::SetUsableGoods(const UUsableGoods* NewUsableGoods)
{
	if (UsableGoods != NewUsableGoods) {
		GameEffectsCache.Empty();
	}
	// UPARAM(ref) does not work since UObjects must be passed as pointers.
	UsableGoods = const_cast<UUsableGoods*>(NewUsableGoods);
}


void UUsableGoodsContext::GetGameEffects(TArray<UGameEffect*>& GameEffects)
{
	// Init our game effects cache if it is empty
	if (GameEffectsCache.Num() == 0 && IsValid(UsableGoods))
	{
		AMMGameMode* GameMode = Cast<AMMGameMode>(UGameplayStatics::GetGameMode(this));
		if (GameMode == nullptr) { return; }
		for (const FGameEffectContext EffectContext : UsableGoods->UsableGoodsType.GameEffects)
		{
			UGameEffect* Effect = NewObject<UGameEffect>(GameMode, EffectContext.GameEffectClass);
			if (Effect) {
				Effect->SetEffectParams(EffectContext);
				if (Effect->Thumbnail == nullptr) {
					Effect->Thumbnail = UsableGoods->GoodsType.Thumbnail;
				}
				GameEffectsCache.Add(Effect);
			}
		}
	}
	// Append cached effects to outgoing array
	GameEffects.Empty(GameEffectsCache.Num());
	if (GameEffectsCache.Num() > 0) {
		GameEffects.Append(GameEffectsCache);
	}
}


bool UUsableGoodsContext::RequiresSelection()
{
	UE_LOG(LogMMGame, Log, TEXT("UsableGoodsContext::RequiresSelection - checking if effects require selection:"));
	bool bRequires = false;
	TArray<UGameEffect*> GameEffects;
	GetGameEffects(GameEffects);
	for (UGameEffect* Effect : GameEffects)
	{
		if (Effect == nullptr) { continue; }
		bRequires = Effect->RequiresSelection();
		UE_LOG(LogMMGame, Log, TEXT("    Effect %s : %d"), *Effect->GetClass()->GetName(), bRequires);
		if (bRequires) {
			return true;
		}
	}
	return false;
}


void UUsableGoodsContext::Cleanup()
{
	GameEffectsCache.Empty();
	UsableGoods = nullptr;
}