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
    virtual void Tick(float DeltaTime) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Product Box")
    UStaticMeshComponent* BoxMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Product Box")
    USceneComponent* ProductSpawnPoint;

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

    UFUNCTION(BlueprintCallable, Category = "Product Box")
    bool IsEmpty() const { return Products.Num() == 0; }

    UFUNCTION(BlueprintCallable, Category = "Product Box")
    void AttachToCamera(UCameraComponent* Camera);

    UFUNCTION(BlueprintCallable, Category = "Product Box")
    void DetachFromCamera();

    UFUNCTION(BlueprintCallable, Category = "Product Box")
    bool IsAttachedToCamera() const { return bIsAttachedToCamera; }

    UFUNCTION(BlueprintCallable, Category = "Product Box")
    TSubclassOf<AProduct> GetProductClass() const { return ProductClass; }

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY()
    TArray<AProduct*> Products;

    void ArrangeProducts();

    bool bIsAttachedToCamera;

    UPROPERTY()
    UCameraComponent* AttachedCamera;

    FVector CameraOffset;
};