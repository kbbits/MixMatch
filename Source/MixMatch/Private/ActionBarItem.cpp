#include "ActionBarItem.h"
#include "Goods/UsableGoodsContext.h"

/*
*/
UActionBarItem::UActionBarItem()
	: Super()
{
}


void UActionBarItem::Cleanup()
{
	if (UsableGoodsContext)
	{
		UsableGoodsContext->Cleanup();
		UsableGoodsContext = nullptr;
	}
}


FName UActionBarItem::GetName()
{
	if (UsableGoodsContext && UsableGoodsContext->GetUsableGoods()) {
		return UsableGoodsContext->GetUsableGoods()->GetName();
	}
	return NAME_None;
}

