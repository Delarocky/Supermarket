// CarAI.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Components/SplineComponent.h"
#include "ParkingSpot.h"
#include "CarAI.generated.h"

UCLASS()
class SUPERMARKET_API ACarAI : public APawn
{
    GENERATED_BODY()

public:
    ACarAI();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintCallable, Category = "Car AI")
    void SetDestination(AParkingSpot* Destination);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Car AI")
    UStaticMeshComponent* CarMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car AI")
    float MovementSpeed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car AI")
    float TurningSpeed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car AI")
    float AcceptanceRadius;

private:
    UPROPERTY()
    AParkingSpot* TargetParkingSpot;

    UPROPERTY()
    USplineComponent* PathSpline;

    float DistanceAlongSpline;

    void MoveAlongSpline(float DeltaTime);
    void UpdateRotation(const FVector& TargetDirection);
    bool HasReachedDestination() const;
    void ParkCar();
};