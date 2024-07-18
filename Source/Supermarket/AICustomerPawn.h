#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Navigation/PathFollowingComponent.h"
#include "ParkingSpace.h"
#include "AICustomerPawn.generated.h"

class UShoppingBag;
class AProduct;
class AShelf;
class ACheckout;
class AAIController;

UCLASS()
class SUPERMARKET_API AAICustomerPawn : public ACharacter
{
    GENERATED_BODY()

public:
    AAICustomerPawn();

    virtual void Tick(float DeltaTime) override;
    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable)
    void StartShopping();

    UFUNCTION(BlueprintCallable)
    void ChooseProduct();

    UFUNCTION(BlueprintCallable)
    void PutProductInBag(AProduct* Product);

    UFUNCTION(BlueprintCallable)
    void GoToCheckout();

    UFUNCTION(BlueprintCallable)
    void FinishShopping();

    UFUNCTION(BlueprintCallable)
    void MoveTo(const FVector& Location);

    UFUNCTION(BlueprintCallable)
    void LeaveCheckout();
    UPROPERTY(BlueprintReadWrite, Category = "Shopping")
    bool bRaiseArm;

    UPROPERTY(BlueprintReadWrite, Category = "Shopping")
    bool bKneelDown;
    
    UPROPERTY(BlueprintReadWrite, Category = "Shopping")
    bool bPutInBag;

    UFUNCTION(BlueprintCallable, Category = "AI")
    FVector FindMostAccessiblePoint(const TArray<FVector>& Points);

    UPROPERTY(BlueprintReadWrite, Category = "Shopping")
    bool bReachUp;

    UPROPERTY(BlueprintReadWrite, Category = "Shopping")
    bool bNormalGrab;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shopping")
    UShoppingBag* ShoppingBag;
    FTimerHandle CheckReachedShelfTimerHandle;
    FTimerHandle RetryTimerHandle;
    void SetInitialSpawnLocation(const FVector& Location) { InitialSpawnLocation = Location; }
    void SetAssignedParkingSpace(AParkingSpace* ParkingSpace) { AssignedParkingSpace = ParkingSpace; }
    void CheckReachedParkingSpace();
    void NotifyParkingSpaceAndDestroy();
  
protected:

    UPROPERTY()
    class AAIController* AIController;
    void DetermineShelfPosition();
    void ResetGrabAnimationFlags();

 
    int32 MaxItems;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shopping")
    float RetryDelay = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shopping")
    float ShoppingTime;
    UFUNCTION(BlueprintCallable, Category = "Shopping")
    void SetCurrentShelf(AShelf* Shelf);

private:
    FTimerHandle ChooseProductTimerHandle;
    void PickUpProduct();
    void LowerArm();
    void RetryEnterCheckoutQueue();
    int32 RetryCount;
    void InitializeAIController();
    void PutCurrentProductInBag();
    void GoToCheckoutWhenDone();
    void DetachAllItems();
    void CheckShoppingComplete();
    FRotator TargetRotation;
    FRotator DeltaRotation;
    float RotationTime;
    float ElapsedTime;
    bool bIsRotating;
    void DebugShoppingState();
    UFUNCTION()
    void CheckReachedShelf();
    void TurnToFaceShelf();
    void TryPickUpProduct();
    AShelf* FindRandomStockedShelf();
    UPROPERTY()
    AProduct* CurrentTargetProduct;
    void StartProductInterpolation();
    void InterpolateProduct();
    FTimerHandle ProductInterpolationTimerHandle;
    FTimerHandle LowerArmTimerHandle;
    FVector GetRandomLocationInStore();
    void DestroyAI();
    UFUNCTION(BlueprintCallable)
    void LeaveStore();
    FTimerHandle LeaveStoreTimerHandle;
    FVector CurrentTargetLocation;
    FTimerHandle PutInBagTimerHandle;
    FTimerHandle ShoppingTimerHandle;
    FTimerHandle CheckoutTimerHandle;
    int32 CurrentItems;
    UPROPERTY()
    AShelf* CurrentShelf;


    UPROPERTY()
    ACheckout* CurrentCheckout;
    FTimerHandle RetryPickUpTimerHandle;
    void CheckReachedAccessPoint();
    int32 FailedNavigationAttempts;
    static const int32 MaxFailedNavigationAttempts = 3;
    void ResetFailedNavigationAttempts() { FailedNavigationAttempts = 0; }
    FVector InitialSpawnLocation;
    AParkingSpace* AssignedParkingSpace;

    FTimerHandle PickUpTimeoutHandle;
};