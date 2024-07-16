// BigShelf.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Shelf.h"
#include "BigShelf.generated.h"

UCLASS()
class SUPERMARKET_API ABigShelf : public AActor
{
    GENERATED_BODY()

public:
    ABigShelf();

    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* RootSceneComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shelves")
    TArray<AShelf*> Shelves;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shelves")
    TSubclassOf<AShelf> ShelfClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shelves")
    FVector ShelfSpacing;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shelves")
    FVector2D GridSize;

    UFUNCTION(BlueprintCallable, Category = "BigShelf")
    void InitializeShelves();

    UFUNCTION(BlueprintCallable, Category = "BigShelf")
    AShelf* GetShelf(int32 Index) const;

    UFUNCTION(BlueprintCallable, Category = "BigShelf")
    void StartStockingAllShelves(TSubclassOf<AProduct> ProductToStock);

    UFUNCTION(BlueprintCallable, Category = "BigShelf")
    void StopStockingAllShelves();

    UFUNCTION(BlueprintCallable, Category = "BigShelf")
    int32 GetTotalProductCount() const;

    UFUNCTION(BlueprintCallable, Category = "BigShelf")
    bool AreAllShelvesFullyStocked() const;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
    void CreateShelves();
    void ClearShelves();
};