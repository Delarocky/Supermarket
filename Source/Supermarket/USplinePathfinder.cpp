// USplinePathfinder.cpp
#include "USplinePathfinder.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

ASplinePathfinder::ASplinePathfinder()
{
    PrimaryActorTick.bCanEverTick = false;

    SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
    RootComponent = SplineComponent;

    // Initialize default values
    StartLocation = FVector::ZeroVector;
    EndLocation = FVector(1000, 0, 0);  // Default end location 1000 units away on X-axis

    MaxSmoothingAngle = 66.239998f;
    MinSmoothingFactor = 1.0f;
    MaxSmoothingFactor = 0.508798f;
    MinAngleForTurn = 40.0f;

}

void ASplinePathfinder::BeginPlay()
{
    Super::BeginPlay();

    //GeneratePath();
    //DrawDebugSpline(100, FColor::Green, 3.0f);
    // Set up the timer to call UpdatePathAndDebug every UpdateInterval seconds
    GetWorldTimerManager().SetTimer(UpdateTimerHandle, this, &ASplinePathfinder::UpdatePathAndDebug, UpdateInterval, true);
}


void ASplinePathfinder::UpdatePathAndDebug()
{
    TArray<AActor*> ObstacleActors;
    FindAllObstacles(ObstacleActors);

    // Check if obstacles have moved
    if (!HaveObstaclesMoved(ObstacleActors))
    {
        // If obstacles haven't moved, no need to update the path
        UE_LOG(LogTemp, Log, TEXT("Obstacles haven't moved. Skipping path update."));
        return;
    }

    GeneratePath();
    DrawDebugSpline(UpdateInterval, FColor::Green, 3.0f);

    // Update the last known obstacle positions
    UpdateLastObstaclePositions(ObstacleActors);
}

void ASplinePathfinder::GeneratePath()
{
    if (!SplineComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("SplineComponent is null in ASplinePathfinder::GeneratePath"));
        return;
    }

    // Convert local space to world space
    FVector WorldStartLocation = GetActorTransform().TransformPosition(StartLocation);
    FVector WorldEndLocation = GetActorTransform().TransformPosition(EndLocation);

    // Set the Z component of both start and end to the same value (using the start Z)
    WorldEndLocation.Z = WorldStartLocation.Z;

    TArray<AActor*> ObstacleActors;
    FindAllObstacles(ObstacleActors);

    // Check if start or end positions are invalid
    if (!IsValidLocation(WorldStartLocation, ObstacleActors) || !IsValidLocation(WorldEndLocation, ObstacleActors))
    {
        UE_LOG(LogTemp, Warning, TEXT("Start or end position is invalid (overlapping with an obstacle). Path generation aborted."));
        SplineComponent->ClearSplinePoints();
        return;
    }

    TArray<FVector> NewPathPoints = FindPath(WorldStartLocation, WorldEndLocation, ObstacleActors);

    // Check if a valid path was found
    if (NewPathPoints.Num() < 2)
    {
        UE_LOG(LogTemp, Warning, TEXT("No valid path found between start and end points. Path generation aborted."));
        SplineComponent->ClearSplinePoints();
        return;
    }

    // Update the spline with the new path
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

            // Calculate smoothing factor based on angle
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

    // Set start and end tangents
    if (NewPathPoints.Num() > 1)
    {
        float StartLength = (NewPathPoints[1] - NewPathPoints[0]).Size();
        float EndLength = (NewPathPoints.Last() - NewPathPoints[NewPathPoints.Num() - 2]).Size();

        FVector StartTangent = (NewPathPoints[1] - NewPathPoints[0]).GetSafeNormal() * (StartLength * MinSmoothingFactor);
        FVector EndTangent = (NewPathPoints.Last() - NewPathPoints[NewPathPoints.Num() - 2]).GetSafeNormal() * (EndLength * MinSmoothingFactor);

        SplineComponent->SetTangentAtSplinePoint(0, StartTangent, ESplineCoordinateSpace::World);
        SplineComponent->SetTangentAtSplinePoint(NewPathPoints.Num() - 1, EndTangent, ESplineCoordinateSpace::World);
    }

    SplineComponent->UpdateSpline();
    UE_LOG(LogTemp, Log, TEXT("Path updated."));
}

TArray<FVector> ASplinePathfinder::FindPath(const FVector& Start, const FVector& End, const TArray<AActor*>& ObstacleActors)
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

        if (FVector2D::Distance(FVector2D(Current), FVector2D(End)) < 100.0f)
        {
            return OptimizePath(ReconstructPath(CameFrom, Current, Start), ObstacleActors);
        }

        OpenSet.Remove(Current);

        for (const FVector& Neighbor : GetNeighbors(Current, 100.0f))
        {
            if (!IsValidLocation(Neighbor, ObstacleActors))
                continue;

            float TentativeGScore = GScore[Current] + FVector2D::Distance(FVector2D(Current), FVector2D(Neighbor));

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
    return TArray<FVector>();  // No path found or iteration limit exceeded
}

float ASplinePathfinder::Heuristic(const FVector& A, const FVector& B)
{
    // Use only X and Y for distance calculation
    return FVector2D::Distance(FVector2D(A), FVector2D(B));
}

TArray<FVector> ASplinePathfinder::GetNeighbors(const FVector& Current, float StepSize)
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

bool ASplinePathfinder::IsValidLocation(const FVector& Location, const TArray<AActor*>& ObstacleActors)
{
    for (AActor* Obstacle : ObstacleActors)
    {
        FVector ObstacleLocation = Obstacle->GetActorLocation();
        FVector ObstacleExtent = Obstacle->GetComponentsBoundingBox().GetExtent();

        // Use MinDistanceFromObstacles instead of a fixed buffer
        float Buffer = MinDistanceFromObstacles;

        // Check if the location is inside the obstacle's 2D bounding box with buffer
        if (FMath::Abs(Location.X - ObstacleLocation.X) < (ObstacleExtent.X + Buffer) &&
            FMath::Abs(Location.Y - ObstacleLocation.Y) < (ObstacleExtent.Y + Buffer))
        {
            return false;
        }
    }
    return true;
}

void ASplinePathfinder::DrawDebugSpline(float Duration, FColor Color, float Thickness)
{
    if (!SplineComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("SplineComponent is null in ASplinePathfinder::DrawDebugSpline"));
        return;
    }

    const int32 NumPoints = SplineComponent->GetNumberOfSplinePoints();
    if (NumPoints < 2)
    {
        UE_LOG(LogTemp, Warning, TEXT("Not enough spline points to draw debug line"));
        return;
    }

    for (int32 i = 0; i < NumPoints - 1; ++i)
    {
        FVector StartPoint = SplineComponent->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
        FVector EndPoint = SplineComponent->GetLocationAtSplinePoint(i + 1, ESplineCoordinateSpace::World);

        DrawDebugLine(
            GetWorld(),
            StartPoint,
            EndPoint,
            Color,
            false,
            Duration,
            0,
            Thickness
        );
    }
}

void ASplinePathfinder::FindAllObstacles(TArray<AActor*>& OutObstacles)
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

    if (ObstacleClasses.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("No ObstacleClasses set in ASplinePathfinder"));
    }
}

TArray<FVector> ASplinePathfinder::ReconstructPath(const TMap<FVector, FVector>& CameFrom, FVector Current, const FVector& Start)
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

TArray<FVector> ASplinePathfinder::OptimizePath(const TArray<FVector>& OriginalPath, const TArray<AActor*>& ObstacleActors)
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

bool ASplinePathfinder::IsLineClear(const FVector& Start, const FVector& End, const TArray<AActor*>& ObstacleActors)
{
    FVector Direction = (End - Start).GetSafeNormal();
    float Distance = FVector::Dist(Start, End);

    // Increase the check frequency for more precise obstacle avoidance
    float StepSize = FMath::Min(50.0f, MinDistanceFromObstacles / 2.0f);

    for (float Step = 0; Step < Distance; Step += StepSize)
    {
        FVector CheckPoint = Start + Direction * Step;
        if (!IsValidLocation(CheckPoint, ObstacleActors))
        {
            return false;
        }
    }

    return true;
}

bool ASplinePathfinder::ArePointsCollinear(const FVector& A, const FVector& B, const FVector& C)
{
    FVector AB = B - A;
    FVector BC = C - B;
    FVector Cross = FVector::CrossProduct(AB, BC);

    // If the cross product is close to zero, the points are collinear
    return Cross.SizeSquared() < 1.0f;
}

bool ASplinePathfinder::HaveObstaclesMoved(const TArray<AActor*>& ObstacleActors)
{
    if (ObstacleActors.Num() != LastObstaclePositions.Num() || ObstacleActors.Num() != LastObstacleRotations.Num())
    {
        return true;  // Number of obstacles has changed
    }

    for (int32 i = 0; i < ObstacleActors.Num(); ++i)
    {
        if (!ObstacleActors[i]->GetActorLocation().Equals(LastObstaclePositions[i], 1.0f))
        {
            return true;  // An obstacle has moved
        }

        if (IsSignificantRotation(LastObstacleRotations[i], ObstacleActors[i]->GetActorRotation()))
        {
            return true;  // An obstacle has rotated significantly
        }
    }

    return false;  // No obstacles have moved or rotated significantly
}

void ASplinePathfinder::UpdateLastObstaclePositions(const TArray<AActor*>& ObstacleActors)
{
    LastObstaclePositions.Empty();
    LastObstacleRotations.Empty();
    for (const AActor* Obstacle : ObstacleActors)
    {
        LastObstaclePositions.Add(Obstacle->GetActorLocation());
        LastObstacleRotations.Add(Obstacle->GetActorRotation());
    }
}

bool ASplinePathfinder::IsSignificantRotation(const FRotator& OldRotation, const FRotator& NewRotation, float Threshold)
{
    // Check if any component (Pitch, Yaw, Roll) has changed more than the threshold
    return FMath::Abs(FMath::FindDeltaAngleDegrees(OldRotation.Pitch, NewRotation.Pitch)) > Threshold ||
        FMath::Abs(FMath::FindDeltaAngleDegrees(OldRotation.Yaw, NewRotation.Yaw)) > Threshold ||
        FMath::Abs(FMath::FindDeltaAngleDegrees(OldRotation.Roll, NewRotation.Roll)) > Threshold;
}