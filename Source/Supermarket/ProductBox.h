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

    UPROPERTY(EditDefaultsOnly, Category = "Product Box")
    UStaticMeshComponent* BoxMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Product Box")
    USceneComponent* ProductSpawnPoint;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Product Box")
    TSubclassOf<AProduct> ProductClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Product Box")
    TSubclassOf<AProduct> DefaultProductClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Product Box")
    int32 MaxProducts;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Product Box")
    FVector ProductSpacing;

    UFUNCTION(BlueprintCallable, Category = "Product Box")
    void FillBox(TSubclassOf<AProduct> ProductToFill);

    UFUNCTION(BlueprintCallable, Category = "Product Box")
    void SetProductClass(TSubclassOf<AProduct> NewProductClass);

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

    UFUNCTION(BlueprintCallable, Category = "Product Box")
    static AProductBox* SpawnProductBox(UObject* WorldContextObject, TSubclassOf<AProductBox> ProductBoxClass, TSubclassOf<AProduct> ProductToSpawn, FVector SpawnLocation);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Product Box")
    FIntVector GridSize;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment")
    FVector CameraOffset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment")
    FRotator CameraRotation;


    UFUNCTION(BlueprintCallable, Category = "Attachment")
    void AttachToComponent(USceneComponent* Parent);
    UFUNCTION(BlueprintCallable, Category = "Product Box")
    AProduct* GetNextProduct() const;
    UFUNCTION(BlueprintCallable, Category = "Product Box")
    int32 GetProductCountt() const;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Product Box")
    FVector ProductOffsetInBox;

    void FillBox(TSubclassOf<AProduct> ProductToFill, int32 Quantity);
    void MakeProductsVisible();
protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY()
    TArray<AProduct*> Products;

    void ArrangeProducts();

    bool bIsAttachedToCamera;

    UPROPERTY()
    UCameraComponent* AttachedCamera;


};