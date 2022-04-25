#include "Goods/UsableGoodsContext.h"
#include "Goods/UsableGoodsType.h"
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
	// Init our game effects cache
	if (GameEffectsCache.Num() == 0 && IsValid(UsableGoods))
	{
		for (const FGameEffectContext EffectContext : UsableGoods->UsableGoodsType.GameEffects)
		{
			UGameEffect* Effect = NewObject<UGameEffect>(this, EffectContext.GameEffectClass);
			if (Effect) {
				Effect->SetEffectParams(EffectContext);
				GameEffectsCache.Add(Effect);
			}
		}
	}
	// Append cached effects to outgoing array
	GameEffects.Empty(GameEffectsCache.Num());
	GameEffects.Append(GameEffectsCache);
}


void UUsableGoodsContext::Cleanup()
{
	GameEffectsCache.Empty();
	UsableGoods = nullptr;
}