// MovementBoundary.cpp
#include "MovementBoundary.h"

AMovementBoundary::AMovementBoundary()
{
    PrimaryActorTick.bCanEverTick = false;

    BoundaryBox = CreateDefaultSubobject<UBoxComponent>(TEXT("BoundaryBox"));
    RootComponent = BoundaryBox;
    BoundaryBox->SetCollisionProfileName(TEXT("OverlapAll"));
    BoundaryBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void AMovementBoundary::BeginPlay()
{
    Super::BeginPlay();
}

bool AMovementBoundary::IsPointWithinBoundary(const FVector& Point) const
{
    if (BoundaryBox)
    {
        FVector LocalPoint = BoundaryBox->GetComponentTransform().InverseTransformPosition(Point);
        FVector Extent = BoundaryBox->GetScaledBoxExtent();
        return FMath::Abs(LocalPoint.X) <= Extent.X &&
            FMath::Abs(LocalPoint.Y) <= Extent.Y &&
            FMath::Abs(LocalPoint.Z) <= Extent.Z;
    }
    return false;
}

FVector AMovementBoundary::ClampPointToBoundary(const FVector& Point) const
{
    if (BoundaryBox)
    {
        FVector LocalPoint = BoundaryBox->GetComponentTransform().InverseTransformPosition(Point);
        FVector Extent = BoundaryBox->GetScaledBoxExtent();
        FVector ClampedLocal = FVector(
            FMath::Clamp(LocalPoint.X, -Extent.X, Extent.X),
            FMath::Clamp(LocalPoint.Y, -Extent.Y, Extent.Y),
            FMath::Clamp(LocalPoint.Z, -Extent.Z, Extent.Z)
        );
        return BoundaryBox->GetComponentTransform().TransformPosition(ClampedLocal);
    }
    return Point;
}