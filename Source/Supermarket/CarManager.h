// CarManager.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CarSplinePathfinder.h"
#include "ParkingSpot.h"
#include "Car.h"
#include "CarManager.generated.h"

UCLASS()
class SUPERMARKET_API ACarManager : public AActor
{
    GENERATED_BODY()

public:
    ACarManager();

    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category = "Car Management")
    void SpawnCarAndNavigateToParkingSpot(TSubclassOf<ACar> CarClass);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car Pathfinding")
    TArray<TSubclassOf<AActor>> ObstacleClasses;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car Spawning")
    float CarSpawnInterval = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car Spawning")
    TSubclassOf<ACar> CarClassToSpawn;

private:
    UPROPERTY()
    TArray<AParkingSpot*> ParkingSpots;

    UFUNCTION()
    void OnCarParked(ACar* ParkedCar);

    AParkingSpot* FindAvailableParkingSpot();
    void FindAllParkingSpots();

    FTimerHandle SpawnTimerHandle;
    void SpawnCarTimerCallback();

    int32 OccupiedParkingSpots;

    const FVector CarSpawnLocation = FVector(0, 0, 70);
};