// Shelf.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Product.h"
#include "ProductBox.h"
#include "IMovableObject.h"
#include "Shelf.generated.h"



UCLASS()
class SUPERMARKET_API AShelf : public AActor
{
    GENERATED_BODY()

public:


    AShelf();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* AccessPoint2;
    UFUNCTION(BlueprintCallable, Category = "Shelf")
    TArray<FVector> GetAllAccessPointLocations() const;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* AccessPoint3;
    UFUNCTION(BlueprintCallable, Category = "Shelf")
    void ContinueStocking();
    UFUNCTION(BlueprintCallable, Category = "Shelf")
    int32 GetRemainingCapacity() const;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shelf")
    bool bStartFullyStocked;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* ProductSpawnPoint;

    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shelf")
    TSubclassOf<AProduct> ProductClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shelf")
    int32 MaxProducts;
    UFUNCTION(BlueprintCallable, Category = "Shelf")
    void SetProductBox(AProductBox* InProductBox) { ProductBox = InProductBox; }
    UPROPERTY()
    TSubclassOf<AProduct> CurrentProductClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shelf")
    FVector ProductSpacing;

    UFUNCTION(BlueprintCallable, Category = "Shelf")
    void RotateShelf(FRotator NewRotation);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shelf")
    FRotator ProductSpawnPointRelativeRotation;
    UFUNCTION(BlueprintCallable, Category = "Shelf")
    FVector GetAccessPointLocation() const;

    UFUNCTION(BlueprintCallable, Category = "Shelf")
    void StartStockingShelf(TSubclassOf<AProduct> ProductToStock);
    UFUNCTION(BlueprintCallable, Category = "Shelf")
    TSubclassOf<AProduct> GetCurrentProductClass() const { return ProductClass; }
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
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Access Point")
    FVector AccessPoint2Offset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Access Point")
    FVector AccessPoint3Offset;

    UFUNCTION(BlueprintCallable, Category = "Shelf")
    bool GetNextProductLocation(FVector& OutLocation) const;
protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* ShelfMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Outline")
    class UMaterialInterface* OutlineMaterial;

    UPROPERTY()
    class UMaterialInstanceDynamic* OutlineMaterialInstance;

private:
    UPROPERTY()
    TArray<AProduct*> Products;
    UPROPERTY()
    AProductBox* ProductBox;
    void SetupAccessPoint();
    bool AddProduct(const FVector& RelativeLocation);
    void InitializeShelf();
    FTimerHandle ContinuousStockingTimerHandle;
    FTimerHandle StockingTimerHandle;
    void StockNextProduct();

    bool bIsStocking;

    UPROPERTY()
    UMaterialInterface* OriginalMaterial;

    UPROPERTY()
    UMaterialInterface* ValidPlacementMaterial;

    UPROPERTY()
    UMaterialInterface* InvalidPlacementMaterial;
};