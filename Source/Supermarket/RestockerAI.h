// RestockerAI.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Shelf.h"
#include "ProductBox.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "StorageRack.h"
#include "RestockerAI.generated.h"

class AAIController;

UCLASS()
class SUPERMARKET_API ARestockerAI : public ACharacter
{
    GENERATED_BODY()

public:
    ARestockerAI();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintCallable, Category = "Restocking")
    void StartRestocking();
    UFUNCTION()
    void TurnToFaceTarget();

    UFUNCTION()
    void OnRotationComplete();
    UFUNCTION(BlueprintCallable, Category = "Restocking")
    bool IsTargetingProductBox(AProductBox* ProductBox) const;


protected:
    UPROPERTY()
    AAIController* AIController;

    UPROPERTY()
    AShelf* TargetShelf;

    UPROPERTY()
    AProductBox* TargetProductBox;

    UPROPERTY()
    bool bIsHoldingProductBox;

    UFUNCTION()
    void FindShelfToRestock();

    UFUNCTION()
    void FindProductBox();

    UFUNCTION()
    void MoveToTarget(AActor* Target);

    UFUNCTION()
    void PickUpProductBox();

    UFUNCTION()
    void RestockShelf();

    UFUNCTION()
    void OnMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result);

private:
    enum class ERestockerState
    {
        Idle,
        MovingToProductBox,
        MovingToShelf,
        Restocking,
        MovingToStorageRack,
        ReplenishingStorageRack,
        ReturningToStorageRack
    };
    
    ERestockerState CurrentState;

    void SetState(ERestockerState NewState);
    FString GetStateName(ERestockerState State);
    UPROPERTY()
    USceneComponent* AttachmentPoint;
    FVector CurrentAccessPoint;
    FRotator TargetRotation;
    bool bIsRotating;

    void MoveToAccessPoint();
    FVector FindNearestAccessPoint(AShelf* Shelf);
    void CheckShelfStocked();
    FTimerHandle RestockTimerHandle;
    FTimerHandle RotationTimerHandle;
    bool IsBoxHeldByPlayer(AProductBox* Box);
    int32 RemainingProducts;
    TSubclassOf<AProduct> CurrentProductClass;

    void HandleEmptyBox();
    void FindNewBoxWithSameProduct();
    void CheckRestockProgress();
    float RotationSpeed;
    float CurrentRotationAlpha;
    void UpdateRotation(float DeltaTime);
    void DropCurrentBox();
    UFUNCTION()
    void FindNextShelfForCurrentProduct();
    UFUNCTION()
    bool IsShelfSufficientlyStocked(AShelf* Shelf);
    FTimerHandle RetryTimerHandle;
    UPROPERTY()
    TArray<AShelf*> CheckedShelves;
    void RetryMove();
    void RetryMoveImpl();
    static TMap<AShelf*, ARestockerAI*> ReservedShelves;
    static TMap<AProductBox*, ARestockerAI*> ReservedProductBoxes;
    static FCriticalSection ReservationLock;

    bool ReserveShelf(AShelf* Shelf);
    void ReleaseShelf(AShelf* Shelf);
    bool ReserveProductBox(AProductBox* ProductBox);
    void ReleaseProductBox(AProductBox* ProductBox);
    bool IsShelfReserved(AShelf* Shelf) const;
    bool IsProductBoxReserved(AProductBox* ProductBox) const;

    void ReleaseAllReservations();
    UFUNCTION(BlueprintCallable, Category = "Restocking")
    AProductBox* GetTargetProductBox() const { return TargetProductBox; }
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    bool ReserveTargetProductBox(AProductBox* ProductBox);
    void ReleaseTargetProductBox();
    static TMap<AProductBox*, ARestockerAI*> TargetedProductBoxes;
    static FCriticalSection TargetedProductBoxesLock;

    static TMap<AProductBox*, ARestockerAI*> LockedProductBoxes;
    static FCriticalSection LockedProductBoxesLock;

    bool LockProductBox(AProductBox* ProductBox);
    void UnlockProductBox();
    bool IsProductBoxLocked(AProductBox* ProductBox) const;

    void AbortCurrentTask();

    FTimerHandle ShelfCheckTimerHandle;

    UFUNCTION()
    void PeriodicShelfCheck();

    void StartPeriodicShelfCheck();
    void StopPeriodicShelfCheck();
    void ReleaseProductBox();
    UPROPERTY()
    AStorageRack* TargetStorageRack;

    UPROPERTY(EditAnywhere, Category = "AI")
    float StorageRackSearchRadius;

    // Add these new methods to the ARestockerAI class
    void FindStorageRack();
    void MoveToStorageRack();
    void CreateProductBoxFromStorageRack();
    void ReplenishStorageRack();
    void ReturnToStorageRack();
    AStorageRack* FindNearestStorageRack(TSubclassOf<AProduct> ProductType);

    AStorageRack* FindAppropriateStorageRack(TSubclassOf<AProduct> ProductType);

    void DeleteEmptyBox();

    void DropBox();
    bool ShouldReplenishStorageRack();
    bool IsStorageRackFull(AStorageRack* StorageRack);
    AProductBox* FindNearestProductBox();
    AStorageRack* FindMatchingStorageRack(TSubclassOf<AProduct> ProductType);
};
