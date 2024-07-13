#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Product.h"
#include "ProductBox.h"
#include "Components/TextRenderComponent.h"
#include "StorageRack.generated.h"

UCLASS()
class SUPERMARKET_API AStorageRack : public AActor
{
    GENERATED_BODY()

public:
    AStorageRack();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storage")
    TSubclassOf<AProduct> ProductType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storage")
    int32 MaxCapacity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storage")
    int32 CurrentStock;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storage")
    UStaticMeshComponent* RackMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storage")
    UTextRenderComponent* QuantityText;

    UFUNCTION(BlueprintCallable, Category = "Storage")
    AProductBox* CreateProductBox(int32 Quantity);

    UFUNCTION(BlueprintCallable, Category = "Storage")
    bool ReplenishStock(int32 Quantity);

    UFUNCTION(BlueprintCallable, Category = "Storage")
    int32 GetAvailableSpace() const;

    UFUNCTION(BlueprintCallable, Category = "Storage")
    bool CanCreateProductBox() const;

    UFUNCTION(BlueprintCallable, Category = "Storage")
    void UpdateQuantityText();
    UFUNCTION(BlueprintCallable, Category = "Storage")
    int32 GetCurrentStock() const { return CurrentStock; }
protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
};