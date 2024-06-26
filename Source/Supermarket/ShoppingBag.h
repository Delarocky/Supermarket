// ShoppingBag.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Product.h" // Make sure this includes the FProductData struct
#include "ShoppingBag.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SUPERMARKET_API UShoppingBag : public UActorComponent
{
    GENERATED_BODY()

public:
    UShoppingBag();

    UFUNCTION(BlueprintCallable, Category = "Shopping")
    void AddProduct(const FProductData& ProductData);

    UFUNCTION(BlueprintCallable, Category = "Shopping")
    TArray<FProductData> GetProducts() const;

    UFUNCTION(BlueprintCallable, Category = "Shopping")
    int32 GetProductCount() const;

    UFUNCTION(BlueprintCallable, Category = "Shopping")
    void EmptyBag();

    UFUNCTION(BlueprintCallable, Category = "Shopping")
    float GetTotalCost() const;

private:
    UPROPERTY()
    TArray<FProductData> Products;
};