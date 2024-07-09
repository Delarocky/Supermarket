// ParkingSpace.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ParkingSpace.generated.h"

class AStoreManager;

UCLASS()
class SUPERMARKET_API AParkingSpace : public AActor
{
    GENERATED_BODY()

public:
    AParkingSpace();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintCallable, Category = "Parking")
    void TrySpawnCar();

    UFUNCTION(BlueprintCallable, Category = "Parking")
    void SpawnCustomers();

    UFUNCTION(BlueprintCallable, Category = "Parking")
    void CustomerReturned(AActor* Customer);


private:
    UPROPERTY(VisibleAnywhere, Category = "Components")
    UStaticMeshComponent* ParkingSpaceMesh;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UStaticMeshComponent* CarMesh;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    USceneComponent* SpawnPoint1;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    USceneComponent* SpawnPoint2;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    USceneComponent* SpawnPoint3;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    USceneComponent* SpawnPoint4;

    UPROPERTY(EditDefaultsOnly, Category = "Spawning")
    TSubclassOf<AActor> CustomerClass;

    UPROPERTY(EditDefaultsOnly, Category = "Spawning")
    float SpawnChance;

    UPROPERTY(EditDefaultsOnly, Category = "Spawning")
    float SpawnInterval;

    FTimerHandle SpawnTimerHandle;

    bool bIsOccupied;
    int32 SpawnedCustomersCount;
    int32 ReturnedCustomersCount;
    TArray<AActor*> SpawnedCustomers;

    UPROPERTY()
    AStoreManager* StoreManager;

    void RemoveCar();

    UFUNCTION()
    void OnStoreStatusChanged(bool bIsOpen);
    FTimerHandle RemoveCarTimerHandle;
};