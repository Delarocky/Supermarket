// AICustomerPawn.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ShoppingBag.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "AICustomerPawn.generated.h"

class ACheckout;
class AShelf;
class AProduct;

UENUM(BlueprintType)
enum class ECustomerState : uint8
{
    Idle,
    MovingToShelf,
    PickingUpProduct,
    MovingToCheckout,
    CheckingOut,
    Roaming,
    Leaving
};

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

    void MoveToCheckoutPosition(int32 QueuePosition);

private:
    UPROPERTY()
    ECustomerState CurrentState;

    UPROPERTY()
    AShelf* TargetShelf;

    UPROPERTY()
    ACheckout* TargetCheckout;

    UPROPERTY()
    int32 MaxProductsToCollect;

    UPROPERTY()
    AAIController* AIController;

    FTimerHandle StateTimerHandle;

    void UpdateState();
    void FindAndMoveToShelf();
    void TryPickUpProduct();
    void FindAndMoveToCheckout();
    void ProcessCheckout();
    void LeaveStore();
    void FinishShopping();
    void StartRoaming();
    void MoveToRandomLocation();
    void DestroySelf();
    int32 ItemsToPickUp;
    FTimerHandle PickupTimerHandle;

    UFUNCTION()
    void OnMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result);

    void MoveToActor(AActor* TargetActor);
    AShelf* FindRandomStockedShelf();

    void SimpleMoveTo(const FVector& Destination);
};