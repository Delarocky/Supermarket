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

    // Generate and draw the path
    GeneratePath();
    DrawDebugSpline(10.0f, FColor::Green, 3.0f);
}

void ASplinePathfinder::GeneratePath()
{
    // Convert local space to world space
    FVector WorldStartLocation = GetActorTransform().TransformPosition(StartLocation);
    FVector WorldEndLocation = GetActorTransform().TransformPosition(EndLocation);

    // Set the Z component of both start and end to the same value (using the start Z)
    WorldEndLocation.Z = WorldStartLocation.Z;

    TArray<AActor*> ObstacleActors;
    FindAllObstacles(ObstacleActors);

    TArray<FVector> PathPoints = FindPath(WorldStartLocation, WorldEndLocation, ObstacleActors);

    SplineComponent->ClearSplinePoints();
    for (const FVector& Point : PathPoints)
    {
        SplineComponent->AddSplinePoint(Point, ESplineCoordinateSpace::World);
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
            TArray<FVector> Path;
            while (CameFrom.Contains(Current))
            {
                Path.Insert(Current, 0);
                Current = CameFrom[Current];
            }
            Path.Insert(Start, 0);
            return Path;
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
        
        // Add a small buffer around the obstacle
        float Buffer = 50.0f; // Adjust this value as needed
        
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
    if (ObstacleClass)
    {
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), ObstacleClass, OutObstacles);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ObstacleClass is not set in ASplinePathfinder"));
    }
}