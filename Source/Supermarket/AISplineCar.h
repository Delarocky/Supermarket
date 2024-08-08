// AISplineCar.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Components/SplineComponent.h"
#include "AISplineCar.generated.h"

UCLASS()
class SUPERMARKET_API AAISplineCar : public APawn
{
    GENERATED_BODY()

public:
    AAISplineCar();

    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    UFUNCTION(BlueprintCallable, Category = "AI")
    void SetSplineToFollow(USplineComponent* Spline);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* CarMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
    float MovementSpeed = 500.0f;

private:
    UPROPERTY()
    USplineComponent* SplineToFollow;

    float DistanceAlongSpline;
};