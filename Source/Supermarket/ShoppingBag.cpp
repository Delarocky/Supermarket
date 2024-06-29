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
        UE_LOG(LogTemp, Display, TEXT("Added product to bag: %s"), *Product->GetProductName());
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

void UShoppingBag::DebugPrintContents() const
{
    UE_LOG(LogTemp, Display, TEXT("Shopping Bag Contents:"));
    for (const auto& Product : Products)
    {
        if (Product)
        {
            UE_LOG(LogTemp, Display, TEXT("- %s (Price: %.2f)"), *Product->GetProductName(), Product->GetPrice());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("- Null product in bag"));
        }
    }
}