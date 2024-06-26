// Checkout.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/TextRenderComponent.h"
#include "Product.h"
#include "Checkout.generated.h"

class AAICustomerPawn;
class UShoppingBag;
class USceneComponent;

UCLASS()
class SUPERMARKET_API ACheckout : public AActor
{
    GENERATED_BODY()

public:
    ACheckout();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintCallable, Category = "Checkout")
    void AddCustomerToQueue(AAICustomerPawn* Customer);

    UFUNCTION(BlueprintCallable, Category = "Checkout")
    void RemoveCustomerFromQueue(AAICustomerPawn* Customer);

    UFUNCTION(BlueprintCallable, Category = "Checkout")
    void ProcessCustomer(AAICustomerPawn* Customer);

    UFUNCTION(BlueprintCallable, Category = "Checkout")
    FVector GetNextQueuePosition() const;

    UFUNCTION(BlueprintCallable, Category = "Checkout")
    bool IsQueueFull() const;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Checkout")
    UTextRenderComponent* TotalProcessedText;

    UFUNCTION(BlueprintCallable, Category = "Checkout")
    bool IsCustomerFirstInQueue(const AAICustomerPawn* Customer) const;

    UFUNCTION(BlueprintCallable, Category = "Checkout")
    int32 GetCustomerQueuePosition(const AAICustomerPawn* Customer) const;

    UFUNCTION(BlueprintCallable, Category = "Checkout")
    bool IsCustomerBeingProcessed(const AAICustomerPawn* Customer) const;

    UFUNCTION(BlueprintCallable, Category = "Checkout")
    FVector GetQueuePosition(int32 Index) const;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Checkout")
    USceneComponent* ScannerLocation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Checkout")
    int32 GridSizeX;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Checkout")
    int32 GridSizeY;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Checkout")
    float GridSpacing;

private:
    UPROPERTY(VisibleAnywhere, Category = "Checkout")
    USceneComponent* RootSceneComponent;

    UPROPERTY(VisibleAnywhere, Category = "Checkout")
    TArray<USceneComponent*> QueuePositions;

    UPROPERTY(VisibleAnywhere, Category = "Checkout")
    UStaticMeshComponent* ConveyorBelt;

    UPROPERTY()
    TArray<AAICustomerPawn*> CustomerQueue;

    UPROPERTY()
    float TotalProcessed;

    UPROPERTY()
    float ProcessingTime;

    UPROPERTY()
    float CurrentProcessingTime;

    UPROPERTY()
    TArray<AProduct*> CurrentProducts;

    UPROPERTY()
    int32 CurrentProductIndex;

    void UpdateQueue();
    void SetupQueuePositions();
    FTimerHandle ProcessingTimerHandle;
    UPROPERTY()
    AAICustomerPawn* CurrentlyProcessingCustomer;

    void UpdateTotalProcessedText();
    void ProcessNextProduct();
    void FinishProcessingCustomer();
    void SpawnProductsOnGrid(const TArray<FProductData>& Products);
};