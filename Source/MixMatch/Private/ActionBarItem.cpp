#include "ActionBarItem.h"
#include "Goods/UsableGoodsType.h"

/*
*/
UActionBarItem::UActionBarItem()
	: Super()
{
}


void UActionBarItem::Cleanup()
{
	UsableGoods = nullptr;
}


FName UActionBarItem::GetName()
{
	if (UsableGoods) {
		return UsableGoods->GetName();
	}
	return NAME_None;
}

