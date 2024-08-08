// CarSplinePathfinder.cpp
#include "CarSplinePathfinder.h"
#include "Kismet/GameplayStatics.h"

ACarSplinePathfinder::ACarSplinePathfinder()
{
    PrimaryActorTick.bCanEverTick = false;

    SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
    RootComponent = SplineComponent;
}

USplineComponent* ACarSplinePathfinder::GeneratePathForCar(const FVector& StartLocation, const FVector& EndLocation, const FRotator& EndRotation)
{
    TArray<AActor*> ObstacleActors;
    FindAllObstacles(ObstacleActors);

    UE_LOG(LogTemp, Log, TEXT("Generating path from (%s) to (%s) with end rotation (%s)"),
        *StartLocation.ToString(), *EndLocation.ToString(), *EndRotation.ToString());

    if (!IsValidLocation(StartLocation, ObstacleActors) || !IsValidLocation(EndLocation, ObstacleActors))
    {
        UE_LOG(LogTemp, Warning, TEXT("Start or end position is invalid (overlapping with an obstacle). Path generation aborted."));
        SplineComponent->ClearSplinePoints();
        return nullptr;
    }

    TArray<FVector> NewPathPoints = FindPath(StartLocation, EndLocation, ObstacleActors);

    if (NewPathPoints.Num() < 2)
    {
        UE_LOG(LogTemp, Warning, TEXT("No valid path found between start and end points. Path generation aborted."));
        SplineComponent->ClearSplinePoints();
        return nullptr;
    }

    SplineComponent->ClearSplinePoints();
    for (int32 i = 0; i < NewPathPoints.Num(); ++i)
    {
        SplineComponent->AddSplinePoint(NewPathPoints[i], ESplineCoordinateSpace::World);

        if (i > 0 && i < NewPathPoints.Num() - 1)
        {
            FVector PrevVector = (NewPathPoints[i] - NewPathPoints[i - 1]).GetSafeNormal();
            FVector NextVector = (NewPathPoints[i + 1] - NewPathPoints[i]).GetSafeNormal();

            float Angle = FMath::Acos(FVector::DotProduct(PrevVector, NextVector));
            float AngleDegrees = FMath::RadiansToDegrees(Angle);

            float SmoothingFactor = FMath::GetMappedRangeValueClamped(
                FVector2D(0, MaxSmoothingAngle),
                FVector2D(MinSmoothingFactor, MaxSmoothingFactor),
                AngleDegrees
            );

            FVector AverageTangent = (PrevVector + NextVector).GetSafeNormal();
            float SegmentLength = FMath::Min((NewPathPoints[i] - NewPathPoints[i - 1]).Size(),
                (NewPathPoints[i + 1] - NewPathPoints[i]).Size());

            FVector SmoothTangent = AverageTangent * (SegmentLength * SmoothingFactor);

            SplineComponent->SetTangentsAtSplinePoint(i, SmoothTangent, SmoothTangent, ESplineCoordinateSpace::World);
        }
    }

    if (NewPathPoints.Num() > 1)
    {
        float StartLength = (NewPathPoints[1] - NewPathPoints[0]).Size();
        float EndLength = (NewPathPoints.Last() - NewPathPoints[NewPathPoints.Num() - 2]).Size();

        FVector StartTangent = (NewPathPoints[1] - NewPathPoints[0]).GetSafeNormal() * (StartLength * MinSmoothingFactor);
        FVector EndTangent = (NewPathPoints.Last() - NewPathPoints[NewPathPoints.Num() - 2]).GetSafeNormal() * (EndLength * MinSmoothingFactor);

        SplineComponent->SetTangentAtSplinePoint(0, StartTangent, ESplineCoordinateSpace::World);
        SplineComponent->SetTangentAtSplinePoint(NewPathPoints.Num() - 1, EndTangent, ESplineCoordinateSpace::World);
    }

    // Add parking approach
    AddParkingApproach(EndLocation, EndRotation);

    SplineComponent->UpdateSpline();
    UE_LOG(LogTemp, Log, TEXT("Path generated with %d points."), NewPathPoints.Num());

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
            //UE_LOG(LogTemp, Warning, TEXT("Location (%s) is invalid due to obstacle at (%s)"),*Location.ToString(), *ObstacleLocation.ToString());
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

void ACarSplinePathfinder::AddParkingApproach(const FVector& EndLocation, const FRotator& EndRotation)
{
    int32 LastPointIndex = SplineComponent->GetNumberOfSplinePoints() - 1;
    FVector LastPoint = SplineComponent->GetLocationAtSplinePoint(LastPointIndex, ESplineCoordinateSpace::World);

    // Calculate approach vector
    FVector ApproachVector = EndRotation.Vector() * -300.0f; // 3 meters approach distance

    // Add approach point
    FVector ApproachPoint = EndLocation + ApproachVector;
    SplineComponent->AddSplinePoint(ApproachPoint, ESplineCoordinateSpace::World);

    // Add final parking point
    SplineComponent->AddSplinePoint(EndLocation, ESplineCoordinateSpace::World);

    // Set tangents for smooth approach
    FVector ApproachTangent = (EndLocation - ApproachPoint).GetSafeNormal() * 200.0f;
    SplineComponent->SetTangentAtSplinePoint(LastPointIndex + 1, ApproachTangent, ESplineCoordinateSpace::World, true);
    SplineComponent->SetTangentAtSplinePoint(LastPointIndex + 2, ApproachTangent, ESplineCoordinateSpace::World, true);
}