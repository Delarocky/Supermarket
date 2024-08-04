// RestockerAI.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Shelf.h"
#include "ProductBox.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "RestockerAI.generated.h"

class AAIController;
class ARestockerManager;

UCLASS()
class SUPERMARKET_API ARestockerAI : public ACharacter
{
    GENERATED_BODY()

public:
    ARestockerAI();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintCallable, Category = "Restocking")
    void AssignTask(AShelf* Shelf, AProductBox* ProductBox);

    UFUNCTION(BlueprintCallable, Category = "Restocking")
    bool IsAvailable() const { return CurrentState == ERestockerState::Idle; }

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
        Restocking
    };

    ERestockerState CurrentState;

    void SetState(ERestockerState NewState);
    FString GetStateName(ERestockerState State);

    FVector CurrentAccessPoint;
    FRotator TargetRotation;
    bool bIsRotating;

    void MoveToAccessPoint();
    FVector FindNearestAccessPoint(AShelf* Shelf);
    FTimerHandle RestockTimerHandle;
    FTimerHandle RotationTimerHandle;
    int32 RemainingProducts;
    TSubclassOf<AProduct> CurrentProductClass;

    void CheckRestockProgress();
    float RotationSpeed;
    float CurrentRotationAlpha;
    void UpdateRotation(float DeltaTime);
    void DropCurrentBox();

    UFUNCTION()
    void TurnToFaceTarget();

    UFUNCTION()
    void OnRotationComplete();

    void AbortCurrentTask();

    UPROPERTY()
    ARestockerManager* Manager;

    void NotifyTaskComplete();
    AShelf* FindNextShelfToRestock();
};