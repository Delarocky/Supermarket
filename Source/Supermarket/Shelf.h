// Shelf.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Product.h"
#include "Shelf.generated.h"

UCLASS()
class SUPERMARKET_API AShelf : public AActor
{
    GENERATED_BODY()

public:
    AShelf();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* ProductSpawnPoint;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* ShelfMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shelf")
    int32 MaxProducts;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shelf")
    TSubclassOf<AProduct> DefaultProductClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shelf")
    FVector ProductSpacing;

    UFUNCTION(BlueprintCallable, Category = "Shelf")
    void RotateShelf(FRotator NewRotation);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shelf")
    FRotator ProductSpawnPointRelativeRotation;
    UFUNCTION(BlueprintCallable, Category = "Shelf")
    FVector GetAccessPointLocation() const;

    UFUNCTION(BlueprintCallable, Category = "Shelf")
    void StartStockingShelf();
    UFUNCTION(BlueprintCallable, Category = "Shelf")
    void UpdateProductSpawnPointRotation();
    UFUNCTION(BlueprintCallable, Category = "Shelf")
    void StopStockingShelf();

    UFUNCTION(BlueprintCallable, Category = "Shelf")
    bool IsSpotEmpty(const FVector& RelativeLocation) const;

    UFUNCTION(BlueprintCallable, Category = "Shelf")
    AProduct* RemoveNextProduct();

    UFUNCTION(BlueprintCallable, Category = "Shelf")
    int32 GetProductCount() const;

    UFUNCTION(BlueprintCallable, Category = "Shelf")
    bool IsFullyStocked() const;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* AccessPoint;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Access Point")
    FVector AccessPointOffset;

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY()
    TArray<AProduct*> Products;
    void SetupAccessPoint();
    bool AddProduct(const FVector& RelativeLocation);
    AProduct* RemoveProduct(const FVector& RelativeLocation);

    FTimerHandle StockingTimerHandle;
    void StockNextProduct();

    bool bIsStocking;
};