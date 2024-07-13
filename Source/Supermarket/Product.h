// Product.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Product.generated.h"

USTRUCT(BlueprintType)
struct FProductData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Product Info")
    FString Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Product Info")
    float Price;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Product Info")
    FVector Scale;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Product Box")
    FIntVector BoxGridSize;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Product Box")
    FVector BoxGridSpacing;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Product Box")
    FVector ProductOffsetInBox;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Product Box")
    int32 MaxProductsInBox;


   

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Product Box")
    TSubclassOf<class AProductBox> ProductBoxClass;

    FProductData()
        : Name("")
        , Price(0.0f)
        , Scale(FVector(1.0f, 1.0f, 1.0f))
        , BoxGridSize(FIntVector(1, 1, 1))
        , BoxGridSpacing(FVector(20.0f, 20.0f, 20.0f))
        , ProductOffsetInBox(FVector::ZeroVector)
        , MaxProductsInBox(20)
        , ProductBoxClass(nullptr)
    {}

    FProductData(const FString& InName, float InPrice, const FVector& InScale)
        : Name(InName)
        , Price(InPrice)
        , Scale(InScale)
        , BoxGridSize(FIntVector(1, 1, 1))
        , BoxGridSpacing(FVector(20.0f, 20.0f, 20.0f))
        , ProductOffsetInBox(FVector::ZeroVector)
        , MaxProductsInBox(20)
        , ProductBoxClass(nullptr)
    {}
};

UCLASS(BlueprintType, Blueprintable)
class SUPERMARKET_API AProduct : public AActor
{
    GENERATED_BODY()

public:
    AProduct();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Product")
    UStaticMeshComponent* ProductMesh;

    UFUNCTION(BlueprintCallable, Category = "Product")
    void InitializeProduct(const FProductData& InProductData);

    UFUNCTION(BlueprintCallable, Category = "Product")
    FString GetProductName() const;

    UFUNCTION(BlueprintCallable, Category = "Product")
    float GetPrice() const;

    UFUNCTION(BlueprintCallable, Category = "Product")
    FProductData GetProductData() const;
  

protected:
    virtual void BeginPlay() override;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Product")
    FProductData ProductData;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};