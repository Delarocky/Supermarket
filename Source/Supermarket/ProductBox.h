// ProductBox.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Product.h"
#include "ProductBox.generated.h"

UCLASS()
class SUPERMARKET_API AProductBox : public AActor
{
    GENERATED_BODY()

public:
    AProductBox();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Product Box")
    UStaticMeshComponent* BoxMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Product Box")
    TSubclassOf<AProduct> ProductClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Product Box")
    int32 MaxProducts;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Product Box")
    FVector ProductSpacing;

    UFUNCTION(BlueprintCallable, Category = "Product Box")
    void FillBox();

    UFUNCTION(BlueprintCallable, Category = "Product Box")
    AProduct* RemoveProduct();

    UFUNCTION(BlueprintCallable, Category = "Product Box")
    int32 GetProductCount() const { return Products.Num(); }

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY()
    TArray<AProduct*> Products;

    void ArrangeProducts();
};