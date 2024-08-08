// CarManager.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ParkingSpot.h"
#include "CarAI.h"
#include "StoreManager.h"
#include "CarManager.generated.h"

UCLASS()
class SUPERMARKET_API ACarManager : public AActor
{
    GENERATED_BODY()

public:
    ACarManager();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car Spawning")
    TSubclassOf<ACarAI> CarClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car Spawning")
    float SpawnInterval;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car Spawning")
    FVector SpawnLocation;

private:
    UPROPERTY()
    TArray<AParkingSpot*> ParkingSpots;

    UPROPERTY()
    TArray<ACarAI*> SpawnedCars;

    UPROPERTY()
    AStoreManager* StoreManager;

    FTimerHandle SpawnTimerHandle;

    void SpawnCar();
    void FindAllParkingSpots();
    AParkingSpot* FindAvailableParkingSpot();
    void OnStoreStatusChanged(bool bIsOpen);
};