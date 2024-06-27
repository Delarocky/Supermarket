// Checkout.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/TextRenderComponent.h"
#include "Components/SceneComponent.h"
#include "AICustomerPawn.h"
#include "ShoppingBag.h"
#include "Product.h"
#include "Checkout.generated.h"

UCLASS()
class SUPERMARKET_API ACheckout : public AActor
{
    GENERATED_BODY()

public:
    ACheckout();

    virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintCallable, Category = "Checkout")
    void ProcessCustomer(AAICustomerPawn* Customer);

    UFUNCTION(BlueprintCallable, Category = "Checkout")
    bool IsQueueFull() const;

    UFUNCTION(BlueprintCallable, Category = "Checkout")
    FVector GetNextQueuePosition() const;

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(VisibleAnywhere, Category = "Components")
    UStaticMeshComponent* CounterMesh;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UTextRenderComponent* TotalDisplay;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    USceneComponent* ItemSpawnPoint;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    USceneComponent* ItemEndPoint;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    TArray<USceneComponent*> QueuePositions;

    UPROPERTY()
    TArray<AAICustomerPawn*> CustomerQueue;

    UPROPERTY()
    AAICustomerPawn* CurrentCustomer;

    UPROPERTY()
    TArray<AProduct*> ItemsOnCounter;

    float ScanDuration;
    float CurrentScanTime;
    int32 CurrentItemIndex;
    float TotalAmount;

    void ProcessNextItem();
    void FinishCheckout();
    void UpdateQueuePositions();
    void ResetCheckout();

    static const int32 MaxQueueSize = 10;
};