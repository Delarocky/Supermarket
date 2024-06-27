// ShoppingBag.cpp
#include "ShoppingBag.h"

UShoppingBag::UShoppingBag()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UShoppingBag::AddProduct(AProduct* Product)
{
    if (Product)
    {
        Products.Add(Product);
    }
}

TArray<AProduct*> UShoppingBag::GetProducts() const
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
    for (const auto& Product : Products)
    {
        if (Product)
        {
            TotalCost += Product->GetPrice();
        }
    }
    return TotalCost;
}