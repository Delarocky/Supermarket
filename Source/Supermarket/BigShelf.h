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


    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* RootSceneComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shelves")
    TArray<AShelf*> Shelves;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shelves")
    FVector ShelfSpacing = FVector(150.0f, 0.0f, 200.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shelves")
    FVector2D GridSize = FVector2D(2, 4);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shelves")
    TSubclassOf<AShelf> ShelfClass;

    UFUNCTION(BlueprintCallable, Category = "BigShelf")
    void PreloadShelvesWithProduct();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Products")
    TSubclassOf<AProduct> PreloadProductClass;


#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
    void CreateShelves();
    void ClearShelves();
};