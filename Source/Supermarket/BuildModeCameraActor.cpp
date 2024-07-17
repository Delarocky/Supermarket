#include "BuildModeCameraActor.h"
#include "GameFramework/PlayerController.h"

ABuildModeCameraActor::ABuildModeCameraActor()
{
    PrimaryActorTick.bCanEverTick = true;

    PivotPoint = CreateDefaultSubobject<USceneComponent>(TEXT("PivotPoint"));
    RootComponent = PivotPoint;

    CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
    CameraComponent->SetupAttachment(PivotPoint);

    // Set initial position and rotation
    PivotPoint->SetWorldLocation(FVector((2300.000000, 2190.000000, 0.000000)));
    UpdateCameraPosition();
    CameraComponent->SetFieldOfView(108.0f);
}

void ABuildModeCameraActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}


void ABuildModeCameraActor::MoveCamera(const FVector2D& InputVector)
{
    FVector ForwardVector = PivotPoint->GetForwardVector();
    FVector RightVector = PivotPoint->GetRightVector();
    ForwardVector.Z = 0.0f;
    RightVector.Z = 0.0f;
    ForwardVector.Normalize();
    RightVector.Normalize();

    FVector Movement = (ForwardVector * InputVector.Y + RightVector * InputVector.X) * MoveSpeed * GetWorld()->GetDeltaSeconds();
    FVector NewLocation = PivotPoint->GetComponentLocation() + Movement;
    NewLocation.Z = PivotPoint->GetComponentLocation().Z; // Maintain the same Z level
    PivotPoint->SetWorldLocation(NewLocation);
    UpdateCameraPosition();
}

void ABuildModeCameraActor::RotateCamera(const FVector2D& InputVector)
{
    FRotator NewRotation = PivotPoint->GetRelativeRotation();
    NewRotation.Yaw += InputVector.X * RotateSpeed * GetWorld()->GetDeltaSeconds();
    PivotPoint->SetRelativeRotation(NewRotation);

    UpdateCameraPosition();
}

void ABuildModeCameraActor::UpdateCameraPosition()
{
    FVector CameraLocation = PivotPoint->GetComponentLocation() -
        PivotPoint->GetForwardVector() * CameraDistance;
    CameraLocation.Z = PivotPoint->GetComponentLocation().Z + CameraHeight;

    CameraComponent->SetWorldLocation(CameraLocation);
    CameraComponent->SetWorldRotation(FRotator(CameraPitch, PivotPoint->GetComponentRotation().Yaw, 0.0f));
}

void ABuildModeCameraActor::SetPivotLocation(const FVector& NewPivotLocation)
{
    if (PivotPoint)
    {
        PivotPoint->SetWorldLocation(NewPivotLocation);
        UpdateCameraPosition();
    }
}