// Product.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Product.generated.h"

USTRUCT(BlueprintType)
struct FProductData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Price;

    FProductData() : Name(""), Price(5.0f) {}
    FProductData(const FString& InName, float InPrice) : Name(InName), Price(5.0f) {}
};

UCLASS()
class SUPERMARKET_API AProduct : public AActor
{
    GENERATED_BODY()

public:
    AProduct();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Product")
    FString ProductName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Product")
    float Price;

    UFUNCTION(BlueprintCallable, Category = "Product")
    FProductData GetProductData() const;

protected:

public:

};