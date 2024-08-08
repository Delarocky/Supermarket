// CarSplinePathfinder.cpp
#include "CarSplinePathfinder.h"
#include "Kismet/GameplayStatics.h"

ACarSplinePathfinder::ACarSplinePathfinder()
{
    PrimaryActorTick.bCanEverTick = false;

    SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
    RootComponent = SplineComponent;
}

USplineComponent* ACarSplinePathfinder::GeneratePathForCar(const FVector& StartLocation, const FVector& EndLocation)
{
    TArray<AActor*> ObstacleActors;
    FindAllObstacles(ObstacleActors);

    UE_LOG(LogTemp, Log, TEXT("Attempting to generate path from (%s) to (%s)"),
        *StartLocation.ToString(), *EndLocation.ToString());

    if (!IsValidLocation(StartLocation, ObstacleActors))
    {
        UE_LOG(LogTemp, Warning, TEXT("Start position (%s) is invalid (overlapping with an obstacle). Path generation aborted."), *StartLocation.ToString());
        return nullptr;
    }

    if (!IsValidLocation(EndLocation, ObstacleActors))
    {
        UE_LOG(LogTemp, Warning, TEXT("End position (%s) is invalid (overlapping with an obstacle). Path generation aborted."), *EndLocation.ToString());
        return nullptr;
    }

    TArray<FVector> PathPoints = FindPath(StartLocation, EndLocation, ObstacleActors);

    if (PathPoints.Num() < 2)
    {
        UE_LOG(LogTemp, Warning, TEXT("No valid path found between start (%s) and end (%s) points. Path generation aborted."),
            *StartLocation.ToString(), *EndLocation.ToString());
        return nullptr;
    }

    SplineComponent->ClearSplinePoints();
    for (const FVector& Point : PathPoints)
    {
        SplineComponent->AddSplinePoint(Point, ESplineCoordinateSpace::World);
    }

    SplineComponent->UpdateSpline();
    UE_LOG(LogTemp, Log, TEXT("Path generated successfully with %d points."), PathPoints.Num());
    return SplineComponent;
}

TArray<FVector> ACarSplinePathfinder::FindPath(const FVector& Start, const FVector& End, const TArray<AActor*>& ObstacleActors)
{
    TArray<FVector> OpenSet;
    OpenSet.Add(Start);

    TMap<FVector, FVector> CameFrom;
    TMap<FVector, float> GScore;
    TMap<FVector, float> FScore;

    GScore.Add(Start, 0);
    FScore.Add(Start, Heuristic(Start, End));

    int32 IterationLimit = 10000; // Prevent infinite loops
    int32 Iterations = 0;

    while (OpenSet.Num() > 0 && Iterations < IterationLimit)
    {
        FVector Current = OpenSet[0];
        float LowestFScore = FScore[Current];
        for (const FVector& Point : OpenSet)
        {
            if (FScore[Point] < LowestFScore)
            {
                Current = Point;
                LowestFScore = FScore[Point];
            }
        }

        if (FVector::Dist2D(Current, End) < 100.0f)
        {
            return OptimizePath(ReconstructPath(CameFrom, Current, Start), ObstacleActors);
        }

        OpenSet.Remove(Current);

        for (const FVector& Neighbor : GetNeighbors(Current, 100.0f))
        {
            if (!IsValidLocation(Neighbor, ObstacleActors))
                continue;

            float TentativeGScore = GScore[Current] + FVector::Dist2D(Current, Neighbor);

            if (!GScore.Contains(Neighbor) || TentativeGScore < GScore[Neighbor])
            {
                CameFrom.Add(Neighbor, Current);
                GScore.Add(Neighbor, TentativeGScore);
                FScore.Add(Neighbor, GScore[Neighbor] + Heuristic(Neighbor, End));
                if (!OpenSet.Contains(Neighbor))
                {
                    OpenSet.Add(Neighbor);
                }
            }
        }

        Iterations++;
    }

    UE_LOG(LogTemp, Warning, TEXT("Path finding exceeded iteration limit or no path found. Returning empty path."));
    return TArray<FVector>();
}

float ACarSplinePathfinder::Heuristic(const FVector& A, const FVector& B)
{
    return FVector::Dist2D(A, B);
}

TArray<FVector> ACarSplinePathfinder::GetNeighbors(const FVector& Current, float StepSize)
{
    TArray<FVector> Neighbors;
    float Z = Current.Z; // Maintain the current Z level

    // Cardinal directions
    Neighbors.Add(FVector(Current.X + StepSize, Current.Y, Z));
    Neighbors.Add(FVector(Current.X - StepSize, Current.Y, Z));
    Neighbors.Add(FVector(Current.X, Current.Y + StepSize, Z));
    Neighbors.Add(FVector(Current.X, Current.Y - StepSize, Z));

    // Diagonal directions
    Neighbors.Add(FVector(Current.X + StepSize, Current.Y + StepSize, Z));
    Neighbors.Add(FVector(Current.X + StepSize, Current.Y - StepSize, Z));
    Neighbors.Add(FVector(Current.X - StepSize, Current.Y + StepSize, Z));
    Neighbors.Add(FVector(Current.X - StepSize, Current.Y - StepSize, Z));

    return Neighbors;
}

bool ACarSplinePathfinder::IsValidLocation(const FVector& Location, const TArray<AActor*>& ObstacleActors)
{
    for (AActor* Obstacle : ObstacleActors)
    {
        FVector ObstacleLocation = Obstacle->GetActorLocation();
        FVector ObstacleExtent = Obstacle->GetComponentsBoundingBox().GetExtent();

        float Buffer = MinDistanceFromObstacles;

        if (FMath::Abs(Location.X - ObstacleLocation.X) < (ObstacleExtent.X + Buffer) &&
            FMath::Abs(Location.Y - ObstacleLocation.Y) < (ObstacleExtent.Y + Buffer))
        {
            UE_LOG(LogTemp, Warning, TEXT("Location (%s) is invalid due to obstacle at (%s)"),
                *Location.ToString(), *ObstacleLocation.ToString());
            return false;
        }
    }
    return true;
}

void ACarSplinePathfinder::FindAllObstacles(TArray<AActor*>& OutObstacles)
{
    for (TSubclassOf<AActor> ObstacleClass : ObstacleClasses)
    {
        if (ObstacleClass)
        {
            TArray<AActor*> ObstaclesOfClass;
            UGameplayStatics::GetAllActorsOfClass(GetWorld(), ObstacleClass, ObstaclesOfClass);
            OutObstacles.Append(ObstaclesOfClass);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Found %d obstacles from %d obstacle classes."), OutObstacles.Num(), ObstacleClasses.Num());
}

TArray<FVector> ACarSplinePathfinder::ReconstructPath(const TMap<FVector, FVector>& CameFrom, FVector Current, const FVector& Start)
{
    TArray<FVector> Path;
    while (CameFrom.Contains(Current))
    {
        Path.Insert(Current, 0);
        Current = CameFrom[Current];
    }
    Path.Insert(Start, 0);
    return Path;
}

TArray<FVector> ACarSplinePathfinder::OptimizePath(const TArray<FVector>& OriginalPath, const TArray<AActor*>& ObstacleActors)
{
    TArray<FVector> OptimizedPath;
    if (OriginalPath.Num() < 2) return OriginalPath;

    OptimizedPath.Add(OriginalPath[0]);

    int32 currentIndex = 0;
    while (currentIndex < OriginalPath.Num() - 1)
    {
        int32 furthestValidIndex = currentIndex + 1;

        for (int32 i = currentIndex + 2; i < OriginalPath.Num(); ++i)
        {
            if (IsLineClear(OriginalPath[currentIndex], OriginalPath[i], ObstacleActors))
            {
                furthestValidIndex = i;
            }
            else
            {
                break;
            }
        }

        OptimizedPath.Add(OriginalPath[furthestValidIndex]);
        currentIndex = furthestValidIndex;
    }

    return OptimizedPath;
}

bool ACarSplinePathfinder::IsLineClear(const FVector& Start, const FVector& End, const TArray<AActor*>& ObstacleActors)
{
    FVector Direction = (End - Start).GetSafeNormal2D();
    float Distance = FVector::Dist2D(Start, End);

    float StepSize = FMath::Min(50.0f, MinDistanceFromObstacles / 2.0f);

    for (float Step = 0; Step < Distance; Step += StepSize)
    {
        FVector CheckPoint = Start + Direction * Step;
        CheckPoint.Z = Start.Z; // Maintain the same Z level
        if (!IsValidLocation(CheckPoint, ObstacleActors))
        {
            return false;
        }
    }

    return true;
}