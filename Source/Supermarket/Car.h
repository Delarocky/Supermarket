// Car.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Components/SplineComponent.h"
#include "Car.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCarParked, ACar*, ParkedCar);

UCLASS()
class SUPERMARKET_API ACar : public APawn
{
    GENERATED_BODY()

public:
    ACar();

    virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintCallable, Category = "Car Movement")
    void FollowSpline(USplineComponent* Spline);

    UPROPERTY(BlueprintAssignable, Category = "Car Events")
    FOnCarParked OnCarParked;

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Car", meta = (AllowPrivateAccess = "true"))
    UStaticMeshComponent* CarMesh;

    UPROPERTY()
    USplineComponent* CurrentSpline;

    float DistanceTraveled;
    float MovementSpeed;

    void MoveAlongSpline(float DeltaTime);
    void RotateAlongSpline();
};