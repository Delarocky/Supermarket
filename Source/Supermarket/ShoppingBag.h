// ShoppingBag.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Product.h"
#include "ShoppingBag.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SUPERMARKET_API UShoppingBag : public UActorComponent
{
    GENERATED_BODY()

public:
    UShoppingBag();

    UFUNCTION(BlueprintCallable, Category = "Shopping")
    void AddProduct(AProduct* Product);
    UFUNCTION(BlueprintCallable, Category = "Shopping")
    void DestroyProducts();
    UFUNCTION(BlueprintCallable, Category = "Shopping")
    TArray<AProduct*> GetProducts() const;

    UFUNCTION(BlueprintCallable, Category = "Shopping")
    int32 GetProductCount() const;

    UFUNCTION(BlueprintCallable, Category = "Shopping")
    void EmptyBag();

    UFUNCTION(BlueprintCallable, Category = "Shopping")
    float GetTotalCost() const;

    UFUNCTION(BlueprintCallable, Category = "Shopping")
    void DebugPrintContents() const;
private:
    UPROPERTY()
    TArray<AProduct*> Products;
};