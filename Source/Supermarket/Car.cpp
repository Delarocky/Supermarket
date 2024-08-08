// Car.cpp
#include "Car.h"

ACar::ACar()
{
    PrimaryActorTick.bCanEverTick = true;

    CarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CarMesh"));
    RootComponent = CarMesh;

    // You can set a default car mesh here
    // static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("/Game/Path/To/Your/CarMesh"));
    // if (MeshAsset.Succeeded())
    //     CarMesh->SetStaticMesh(MeshAsset.Object);

    DistanceTraveled = 0.0f;
    MovementSpeed = 500.0f; // Adjust as needed
}

void ACar::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (CurrentSpline)
    {
        MoveAlongSpline(DeltaTime);
        RotateAlongSpline();
    }
}

void ACar::FollowSpline(USplineComponent* Spline)
{
    CurrentSpline = Spline;
    DistanceTraveled = 0.0f;
}

void ACar::MoveAlongSpline(float DeltaTime)
{
    if (!CurrentSpline) return;

    DistanceTraveled += MovementSpeed * DeltaTime;

    if (DistanceTraveled >= CurrentSpline->GetSplineLength())
    {
        // Reached the end of the spline
        SetActorLocation(CurrentSpline->GetLocationAtSplinePoint(CurrentSpline->GetNumberOfSplinePoints() - 1, ESplineCoordinateSpace::World));
        OnCarParked.Broadcast(this);
        CleanupSpline();
    }
    else
    {
        FVector NewLocation = CurrentSpline->GetLocationAtDistanceAlongSpline(DistanceTraveled, ESplineCoordinateSpace::World);
        SetActorLocation(NewLocation);
    }
}

void ACar::RotateAlongSpline()
{
    if (!CurrentSpline) return;

    FVector ForwardVector = CurrentSpline->GetDirectionAtDistanceAlongSpline(DistanceTraveled, ESplineCoordinateSpace::World);
    SetActorRotation(ForwardVector.Rotation());
}

void ACar::CleanupSpline()
{
    if (CurrentSpline)
    {
        CurrentSpline->DestroyComponent();
        CurrentSpline = nullptr;
    }

    if (SplineOwner)
    {
        SplineOwner->Destroy();
        SplineOwner = nullptr;
    }
}