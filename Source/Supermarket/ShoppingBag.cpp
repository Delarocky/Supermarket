// ShoppingBag.cpp
#include "ShoppingBag.h"

UShoppingBag::UShoppingBag()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UShoppingBag::AddProduct(const FProductData& ProductData)
{
    Products.Add(ProductData);
}

TArray<FProductData> UShoppingBag::GetProducts() const
{
    return Products;
}

int32 UShoppingBag::GetProductCount() const
{
    return Products.Num();
}

void UShoppingBag::EmptyBag()
{
    Products.Empty();
}

float UShoppingBag::GetTotalCost() const
{
    float TotalCost = 0.0f;
    for (const auto& ProductData : Products)
    {
        TotalCost += ProductData.Price;
    }
    return TotalCost;
}