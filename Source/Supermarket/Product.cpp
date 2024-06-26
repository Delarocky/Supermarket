// Product.cpp
#include "Product.h"

AProduct::AProduct()
{
    PrimaryActorTick.bCanEverTick = true;
    Price = 5.0f; // Set default price to $5
}

FProductData AProduct::GetProductData() const
{
    return FProductData(ProductName, Price);
}