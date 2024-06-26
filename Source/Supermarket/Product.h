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

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector Scale;

    FProductData() : Name(""), Price(0.0f), Scale(FVector(1.0f, 1.0f, 1.0f)) {}
    FProductData(const FString& InName, float InPrice, const FVector& InScale)
        : Name(InName), Price(InPrice), Scale(InScale) {}
};

UCLASS(BlueprintType, Blueprintable)
class SUPERMARKET_API AProduct : public AActor
{
    GENERATED_BODY()

public:
    AProduct();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Product")
    UStaticMeshComponent* ProductMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Product")
    FProductData ProductData;

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

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};