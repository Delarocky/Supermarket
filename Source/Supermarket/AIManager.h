// AIManager.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AICustomerPawn.h"
#include "RestockerAI.h"
#include "AIManager.generated.h"

UCLASS()
class SUPERMARKET_API AAIManager : public AActor
{
    GENERATED_BODY()

public:
    AAIManager();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintCallable, Category = "AI Management")
    void SpawnCustomer(TSubclassOf<AAICustomerPawn> CustomerClass, const FVector& SpawnLocation);

    UFUNCTION(BlueprintCallable, Category = "AI Management")
    void SpawnRestocker(TSubclassOf<ARestockerAI> RestockerClass, const FVector& SpawnLocation);

    UFUNCTION(BlueprintCallable, Category = "AI Management")
    void RemoveCustomer(AAICustomerPawn* Customer);

    UFUNCTION(BlueprintCallable, Category = "AI Management")
    void RemoveRestocker(ARestockerAI* Restocker);

private:
    UPROPERTY()
    TArray<AAICustomerPawn*> Customers;

    UPROPERTY()
    TArray<ARestockerAI*> Restockers;

    UPROPERTY(EditDefaultsOnly, Category = "AI Management")
    int32 MaxCustomers;

    UPROPERTY(EditDefaultsOnly, Category = "AI Management")
    int32 MaxRestockers;

    UPROPERTY(EditDefaultsOnly, Category = "AI Management")
    float UpdateInterval;

    FTimerHandle UpdateTimerHandle;

    void UpdateAIAgents();
    void OptimizeAIBehavior();
};