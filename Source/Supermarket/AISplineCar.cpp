// AISplineCar.cpp
#include "AISplineCar.h"

AAISplineCar::AAISplineCar()
{
    PrimaryActorTick.bCanEverTick = true;

    CarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CarMesh"));
    RootComponent = CarMesh;

    // Load a default car mesh if you have one
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CarMeshAsset(TEXT("/Game/Path/To/Your/CarMesh"));
    if (CarMeshAsset.Succeeded())
    {
        CarMesh->SetStaticMesh(CarMeshAsset.Object);
    }

    DistanceAlongSpline = 0.0f;
}

void AAISplineCar::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (SplineToFollow)
    {
        // Move along the spline
        DistanceAlongSpline += MovementSpeed * DeltaTime;

        // Wrap around if we reach the end of the spline
        if (DistanceAlongSpline > SplineToFollow->GetSplineLength())
        {
            DistanceAlongSpline -= SplineToFollow->GetSplineLength();
        }

        // Get new location and rotation from the spline
        FVector NewLocation = SplineToFollow->GetLocationAtDistanceAlongSpline(DistanceAlongSpline, ESplineCoordinateSpace::World);
        FRotator NewRotation = SplineToFollow->GetRotationAtDistanceAlongSpline(DistanceAlongSpline, ESplineCoordinateSpace::World);

        // Set the new location and rotation
        SetActorLocationAndRotation(NewLocation, NewRotation);
    }
}

void AAISplineCar::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AAISplineCar::SetSplineToFollow(USplineComponent* Spline)
{
    SplineToFollow = Spline;
}