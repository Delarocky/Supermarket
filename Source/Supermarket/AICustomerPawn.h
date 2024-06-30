#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Navigation/PathFollowingComponent.h"
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
    void GrabProduct(AProduct* Product);

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

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shopping")
    UShoppingBag* ShoppingBag;
    FTimerHandle CheckReachedShelfTimerHandle;
    FTimerHandle RetryTimerHandle;
protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    class UAnimMontage* GrabProductAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    class UAnimMontage* PutInBagAnimation;

    UPROPERTY()
    class AAIController* AIController;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shopping")
    int32 MaxItems;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shopping")
    float RetryDelay = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shopping")
    float ShoppingTime;
    UFUNCTION(BlueprintCallable, Category = "Shopping")
    void SetCurrentShelf(AShelf* Shelf);
private:
    FTimerHandle ChooseProductTimerHandle;
    void RetryEnterCheckoutQueue();
    int32 RetryCount;
    void InitializeAIController();
    void PutCurrentProductInBag();
    void GoToCheckoutWhenDone();
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
};