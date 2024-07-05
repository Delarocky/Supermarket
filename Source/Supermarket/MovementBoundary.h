// MovementBoundary.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "MovementBoundary.generated.h"

UCLASS()
class SUPERMARKET_API AMovementBoundary : public AActor
{
    GENERATED_BODY()

public:
    AMovementBoundary();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boundary")
    UBoxComponent* BoundaryBox;

    UFUNCTION(BlueprintCallable, Category = "Boundary")
    bool IsPointWithinBoundary(const FVector& Point) const;

    UFUNCTION(BlueprintCallable, Category = "Boundary")
    FVector ClampPointToBoundary(const FVector& Point) const;

protected:
    virtual void BeginPlay() override;
};