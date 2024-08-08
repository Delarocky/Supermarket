// USplinePathfinder.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SplineComponent.h"
#include "AISplineCar.h"
#include "USplinePathfinder.generated.h"

UCLASS()
class SUPERMARKET_API ASplinePathfinder : public AActor
{
    GENERATED_BODY()

public:
    ASplinePathfinder();

    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category = "Pathfinding")
    void GeneratePath();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pathfinding")
    USplineComponent* SplineComponent;

    UFUNCTION(BlueprintCallable, Category = "Debug")
    void DrawDebugSpline(float Duration = 5.0f, FColor Color = FColor::Red, float Thickness = 2.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pathfinding", Meta = (MakeEditWidget = true))
    FVector StartLocation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pathfinding", Meta = (MakeEditWidget = true))
    FVector EndLocation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pathfinding")
    TArray<TSubclassOf<AActor>> ObstacleClasses;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pathfinding")
    float MinDistanceFromObstacles = 1.0f; // New variable for minimum distance from obstacles


    UFUNCTION()
    void UpdatePathAndDebug();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pathfinding")
    float UpdateInterval = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
    TSubclassOf<AAISplineCar> AICarClass;

    UFUNCTION(BlueprintCallable, Category = "AI")
    void SpawnAICar();
private:
    TArray<FVector> FindPath(const FVector& Start, const FVector& End, const TArray<AActor*>& ObstacleActors);
    float Heuristic(const FVector& A, const FVector& B);
    TArray<FVector> GetNeighbors(const FVector& Current, float StepSize);
    bool IsValidLocation(const FVector& Location, const TArray<AActor*>& ObstacleActors);
    void FindAllObstacles(TArray<AActor*>& OutObstacles);
    TArray<FVector> ReconstructPath(const TMap<FVector, FVector>& CameFrom, FVector Current, const FVector& Start);
    TArray<FVector> OptimizePath(const TArray<FVector>& OriginalPath, const TArray<AActor*>& ObstacleActors);
    bool IsLineClear(const FVector& Start, const FVector& End, const TArray<AActor*>& ObstacleActors);
    bool ArePointsCollinear(const FVector& A, const FVector& B, const FVector& C);
    FTimerHandle UpdateTimerHandle;
};