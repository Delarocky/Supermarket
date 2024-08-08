// CarSplinePathfinder.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SplineComponent.h"
#include "CarSplinePathfinder.generated.h"

UCLASS()
class SUPERMARKET_API ACarSplinePathfinder : public AActor
{
    GENERATED_BODY()

public:
    ACarSplinePathfinder();

    UFUNCTION(BlueprintCallable, Category = "Car Pathfinding")
    USplineComponent* GeneratePathForCar(const FVector& StartLocation, const FVector& EndLocation, const FRotator& EndRotation);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car Pathfinding")
    TArray<TSubclassOf<AActor>> ObstacleClasses;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car Pathfinding")
    float MinDistanceFromObstacles = 200.0f;
    UFUNCTION(BlueprintCallable, Category = "Car Pathfinding")
    USplineComponent* GetSplineComponent() const { return SplineComponent; }
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car Pathfinding|Smoothing", Meta = (ClampMin = "0.0", ClampMax = "360.0"))
    float MaxSmoothingAngle = 66.239998f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car Pathfinding|Smoothing", Meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MinSmoothingFactor = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car Pathfinding|Smoothing", Meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MaxSmoothingFactor = 0.508798f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car Pathfinding|Optimization", Meta = (ClampMin = "0.0", ClampMax = "180.0"))
    float MinAngleForTurn = 40.0f;
private:
    UPROPERTY()
    USplineComponent* SplineComponent;

    TArray<FVector> FindPath(const FVector& Start, const FVector& End, const TArray<AActor*>& ObstacleActors);
    float Heuristic(const FVector& A, const FVector& B);
    TArray<FVector> GetNeighbors(const FVector& Current, float StepSize);
    bool IsValidLocation(const FVector& Location, const TArray<AActor*>& ObstacleActors);
    void FindAllObstacles(TArray<AActor*>& OutObstacles);
    TArray<FVector> ReconstructPath(const TMap<FVector, FVector>& CameFrom, FVector Current, const FVector& Start);
    TArray<FVector> OptimizePath(const TArray<FVector>& OriginalPath, const TArray<AActor*>& ObstacleActors);
    bool IsLineClear(const FVector& Start, const FVector& End, const TArray<AActor*>& ObstacleActors);
    void ModifyPathForParking(TArray<FVector>& PathPoints, const FVector& EndLocation, const FRotator& EndRotation);
};