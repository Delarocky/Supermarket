// AICustomerPawn.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "AICustomerPawn.generated.h"

class ACheckout;
class AShelf;
class AProduct;
class UShoppingBag;

UCLASS()
class SUPERMARKET_API AAICustomerPawn : public ACharacter
{
    GENERATED_BODY()

public:
    AAICustomerPawn();

    virtual void Tick(float DeltaTime) override;
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shopping")
    UShoppingBag* ShoppingBag;

    UFUNCTION(BlueprintCallable, Category = "Shopping")
    void OnCheckoutComplete();

    UFUNCTION(BlueprintCallable, Category = "Shopping")
    UShoppingBag* GetShoppingBag() const { return ShoppingBag; }

private:
    UPROPERTY()
    int32 TotalItemsToPickUp;

    UPROPERTY()
    AAIController* AIController;

    UPROPERTY()
    AShelf* CurrentShelf;

    FTimerHandle ActionTimerHandle;

    void DecideNextAction();
    void MoveToRandomShelf();
    void MoveToCheckout();
    void LeaveStore();
    void MoveToLocation(const FVector& Destination);
    void TryPickUpProduct();
    void ProcessCheckout();

    UFUNCTION()
    void OnMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result);
    ACheckout* FindOpenCheckout();
    AShelf* FindRandomStockedShelf();
    void DestroySelf();
};