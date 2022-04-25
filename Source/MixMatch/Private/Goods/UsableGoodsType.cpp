#include "Goods/UsableGoodsType.h"


/*
*/
UUsableGoods::UUsableGoods()
	: Super()
{
}


bool UUsableGoods::IsUsable()
{
	return UsableGoodsType.Name != NAME_None; 
}


FName UUsableGoods::GetName()
{
	return GoodsType.Name;
}