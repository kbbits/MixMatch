
#include "GoodsTypeQuantity.h"

UGoodsTypeQuantity::UGoodsTypeQuantity()
	: Super()
{
}

FGoodsQuantity UGoodsTypeQuantity::GetGoodsQuantity()
{
	return FGoodsQuantity(GoodsType.Name, Quantity);
}