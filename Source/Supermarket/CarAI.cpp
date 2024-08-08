// CarAI.cpp
#include "CarAI.h"
#include "Kismet/GameplayStatics.h"
#include "USplinePathfinder.h"

ACarAI::ACarAI()
{
    PrimaryActorTick.bCanEverTick = true;

    CarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CarMesh"));
    RootComponent = CarMesh;

    MovementSpeed = 500.0f;
    TurningSpeed = 2.0f;
    AcceptanceRadius = 100.0f;
    DistanceAlongSpline = 0.0f;
}

void ACarAI::BeginPlay()
{
    Super::BeginPlay();
}

void ACarAI::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (PathSpline && !HasReachedDestination())
    {
        MoveAlongSpline(DeltaTime);
    }
    else if (HasReachedDestination())
    {
        ParkCar();
    }
}

void ACarAI::SetDestination(AParkingSpot* Destination)
{
    TargetParkingSpot = Destination;

    if (TargetParkingSpot)
    {
        // Find the USplinePathfinder in the level
        TArray<AActor*> FoundActors;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASplinePathfinder::StaticClass(), FoundActors);

        if (FoundActors.Num() > 0)
        {
            ASplinePathfinder* Pathfinder = Cast<ASplinePathfinder>(FoundActors[0]);
            if (Pathfinder)
            {
                // Generate path from current location to parking spot
                Pathfinder->StartLocation = GetActorLocation();
                Pathfinder->EndLocation = TargetParkingSpot->ParkingLocation;
                Pathfinder->GeneratePath();

                // Use the generated spline for movement
                PathSpline = Pathfinder->SplineComponent;
            }
        }
    }
}

void ACarAI::MoveAlongSpline(float DeltaTime)
{
    if (!PathSpline) return;

    float SplineLength = PathSpline->GetSplineLength();
    DistanceAlongSpline += MovementSpeed * DeltaTime;

    if (DistanceAlongSpline <= SplineLength)
    {
        FVector NewLocation = PathSpline->GetLocationAtDistanceAlongSpline(DistanceAlongSpline, ESplineCoordinateSpace::World);
        FVector NextLocation = PathSpline->GetLocationAtDistanceAlongSpline(FMath::Min(DistanceAlongSpline + 100.0f, SplineLength), ESplineCoordinateSpace::World);

        SetActorLocation(NewLocation);
        UpdateRotation(NextLocation - NewLocation);
    }
}

void ACarAI::UpdateRotation(const FVector& TargetDirection)
{
    FRotator NewRotation = TargetDirection.Rotation();
    NewRotation.Pitch = 0.0f; // Keep the car level
    SetActorRotation(FMath::RInterpTo(GetActorRotation(), NewRotation, GetWorld()->GetDeltaSeconds(), TurningSpeed));
}

bool ACarAI::HasReachedDestination() const
{
    if (TargetParkingSpot)
    {
        return FVector::Dist(GetActorLocation(), TargetParkingSpot->ParkingLocation) < AcceptanceRadius;
    }
    return false;
}

void ACarAI::ParkCar()
{
    if (TargetParkingSpot)
    {
        SetActorLocation(TargetParkingSpot->ParkingLocation);
        SetActorRotation(TargetParkingSpot->ParkingRotation);
    }
}