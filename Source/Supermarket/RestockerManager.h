// RestockerManager.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Shelf.h"
#include "ProductBox.h"
#include "RestockerAI.h"
#include "RestockerManager.generated.h"

UCLASS()
class SUPERMARKET_API ARestockerManager : public AActor
{
    GENERATED_BODY()

public:
    ARestockerManager();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintCallable, Category = "Restocking")
    void AssignTaskToRestocker(ARestockerAI* Restocker);
    UFUNCTION(BlueprintCallable, Category = "Restocking")
    AShelf* FindShelfNeedingProduct(TSubclassOf<AProduct> ProductClass, ARestockerAI* Restocker);

    void ReleaseShelf(AShelf* Shelf);
    UFUNCTION()
    void OnRestockerTaskComplete(ARestockerAI* Restocker);
private:
    UPROPERTY()
    TArray<ARestockerAI*> AvailableRestockers;

    UPROPERTY()
    TArray<AShelf*> ShelvesNeedingRestock;

    UPROPERTY()
    TArray<AProductBox*> AvailableProductBoxes;

    void UpdateShelvesNeedingRestock();
    void UpdateAvailableProductBoxes();
    AShelf* FindShelfToRestock();
    AProductBox* FindMatchingProductBox(TSubclassOf<AProduct> ProductClass);
    bool IsShelfSufficientlyStocked(AShelf* Shelf);

    FTimerHandle PeriodicUpdateTimerHandle;
    void PeriodicUpdate();

    static TMap<AShelf*, ARestockerAI*> ReservedShelves;
    static TMap<AProductBox*, ARestockerAI*> ReservedProductBoxes;
    static FCriticalSection ReservationLock;

    bool ReserveShelf(AShelf* Shelf, ARestockerAI* Restocker);
    
    bool ReserveProductBox(AProductBox* ProductBox, ARestockerAI* Restocker);
    void ReleaseProductBox(AProductBox* ProductBox);

    FTimerHandle UpdateTimerHandle;
    void ScheduledUpdate();
    void AssignTasks();

    UPROPERTY()
    TMap<AShelf*, ARestockerAI*> ShelvesBeingRestocked;

    FCriticalSection ShelfAssignmentLock;
};
