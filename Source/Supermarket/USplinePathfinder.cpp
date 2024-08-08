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
}

void ASplinePathfinder::BeginPlay()
{
    Super::BeginPlay();

    //GeneratePath();
    //DrawDebugSpline(100, FColor::Green, 3.0f);
    SpawnAICar();
    // Set up the timer to call UpdatePathAndDebug every UpdateInterval seconds
    GetWorldTimerManager().SetTimer(UpdateTimerHandle, this, &ASplinePathfinder::UpdatePathAndDebug, UpdateInterval, true);
}


void ASplinePathfinder::UpdatePathAndDebug()
{
    GeneratePath();
    DrawDebugSpline(UpdateInterval, FColor::Green, 3.0f);
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

    TArray<FVector> PathPoints = FindPath(WorldStartLocation, WorldEndLocation, ObstacleActors);

    // Check if a valid path was found
    if (PathPoints.Num() < 2)
    {
        UE_LOG(LogTemp, Warning, TEXT("No valid path found between start and end points. Path generation aborted."));
        SplineComponent->ClearSplinePoints();
        return;
    }

    SplineComponent->ClearSplinePoints();
    for (int32 i = 0; i < PathPoints.Num(); ++i)
    {
        SplineComponent->AddSplinePoint(PathPoints[i], ESplineCoordinateSpace::World);

        // Set tangents to create sharp corners
        if (i > 0 && i < PathPoints.Num() - 1)
        {
            FVector PrevTangent = (PathPoints[i] - PathPoints[i - 1]).GetSafeNormal() * 0.0001f;
            FVector NextTangent = (PathPoints[i + 1] - PathPoints[i]).GetSafeNormal() * 0.0001f;

            SplineComponent->SetTangentsAtSplinePoint(i, PrevTangent, NextTangent, ESplineCoordinateSpace::World);
        }
    }

    // Set start and end tangents
    if (PathPoints.Num() > 1)
    {
        FVector StartTangent = (PathPoints[1] - PathPoints[0]).GetSafeNormal() * 0.0001f;
        FVector EndTangent = (PathPoints.Last() - PathPoints[PathPoints.Num() - 2]).GetSafeNormal() * 0.0001f;

        SplineComponent->SetTangentAtSplinePoint(0, StartTangent, ESplineCoordinateSpace::World);
        SplineComponent->SetTangentAtSplinePoint(PathPoints.Num() - 1, EndTangent, ESplineCoordinateSpace::World);
    }

    SplineComponent->UpdateSpline();
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

    while (OpenSet.Num() > 0)
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
    }

    return TArray<FVector>();  // No path found
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

void ASplinePathfinder::SpawnAICar()
{
    if (AICarClass && SplineComponent)
    {
        FVector SpawnLocation = SplineComponent->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World);
        FRotator SpawnRotation = SplineComponent->GetRotationAtSplinePoint(0, ESplineCoordinateSpace::World);

        AAISplineCar* SpawnedCar = GetWorld()->SpawnActor<AAISplineCar>(AICarClass, SpawnLocation, SpawnRotation);
        if (SpawnedCar)
        {
            SpawnedCar->SetSplineToFollow(SplineComponent);
        }
    }
}