// ParkingSpot.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ParkingSpot.generated.h"

UCLASS()
class SUPERMARKET_API AParkingSpot : public AActor
{
    GENERATED_BODY()

public:
    AParkingSpot();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parking")
    bool bIsOccupied;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parking")
    FVector ParkingLocation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parking")
    FRotator ParkingRotation;

    UFUNCTION(BlueprintCallable, Category = "Parking")
    void OccupySpot();

    UFUNCTION(BlueprintCallable, Category = "Parking")
    void VacateSpot();
};