// RestockerAI.cpp
#include "RestockerAI.h"
#include "RestockerManager.h"
#include "Kismet/GameplayStatics.h"

ARestockerAI::ARestockerAI()
{
    PrimaryActorTick.bCanEverTick = true;
    CurrentState = ERestockerState::Idle;
    bIsHoldingProductBox = false;
    RotationSpeed = 360.0f;
    CurrentRotationAlpha = 0.0f;
    bIsAbortingTask = false;
}

void ARestockerAI::BeginPlay()
{
    Super::BeginPlay();
    AIController = Cast<AAIController>(GetController());
    if (AIController)
    {
        AIController->ReceiveMoveCompleted.AddDynamic(this, &ARestockerAI::OnMoveCompleted);
    }

    // Find the RestockerManager
    TArray<AActor*> FoundManagers;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARestockerManager::StaticClass(), FoundManagers);
    if (FoundManagers.Num() > 0)
    {
        Manager = Cast<ARestockerManager>(FoundManagers[0]);
    }
}

void ARestockerAI::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (bIsRotating)
    {
        UpdateRotation(DeltaTime);
    }
}

void ARestockerAI::AssignTask(AShelf* Shelf, AProductBox* ProductBox)
{
    TargetShelf = Shelf;
    TargetProductBox = ProductBox;
    SetState(ERestockerState::MovingToProductBox);
    MoveToTarget(TargetProductBox);
}

void ARestockerAI::MoveToTarget(AActor* Target)
{
    if (!AIController || !Target)
    {
        SetState(ERestockerState::Idle);
        return;
    }

    if (AShelf* Shelf = Cast<AShelf>(Target))
    {
        CurrentAccessPoint = FindNearestAccessPoint(Shelf);
        MoveToAccessPoint();
    }
    else
    {
        AIController->MoveToActor(Target, 50.0f);
    }
}

void ARestockerAI::PickUpProductBox()
{
    if (TargetProductBox && !TargetProductBox->GetAttachParentActor())
    {
        bIsHoldingProductBox = true;
        UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(TargetProductBox->GetRootComponent());
        if (PrimitiveComponent)
        {
            PrimitiveComponent->SetSimulatePhysics(false);
            PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }
        FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, false);
        TargetProductBox->AttachToActor(this, AttachmentRules);
        FVector AttachmentOffset(50.0f, 0.0f, 0.0f);
        TargetProductBox->SetActorRelativeLocation(AttachmentOffset);
        RemainingProducts = TargetProductBox->GetProductCountt();
        CurrentProductClass = TargetProductBox->GetProductClass();
        SetState(ERestockerState::MovingToShelf);
        MoveToTarget(TargetShelf);
    }
    else
    {
        AbortCurrentTask();
    }
}

void ARestockerAI::RestockShelf()
{
    if (TargetShelf && bIsHoldingProductBox && TargetProductBox)
    {
        TargetShelf->SetProductBox(TargetProductBox);
        TargetShelf->StartStockingShelf(TargetProductBox->GetProductClass());
        GetWorld()->GetTimerManager().SetTimer(RestockTimerHandle, this, &ARestockerAI::CheckRestockProgress, 0.5f, true);
    }
    else
    {
        AbortCurrentTask();
    }
}

void ARestockerAI::OnMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result)
{
    if (Result == EPathFollowingResult::Success)
    {
        switch (CurrentState)
        {
        case ERestockerState::MovingToProductBox:
            PickUpProductBox();
            break;
        case ERestockerState::MovingToShelf:
            TurnToFaceTarget();
            break;
        case ERestockerState::Restocking:
            RestockShelf();
            break;
        default:
            AbortCurrentTask();
            break;
        }
    }
    else
    {
        AbortCurrentTask();
    }
}

void ARestockerAI::SetState(ERestockerState NewState)
{
    CurrentState = NewState;
}

FString ARestockerAI::GetStateName(ERestockerState State)
{
    switch (State)
    {
    case ERestockerState::Idle: return TEXT("Idle");
    case ERestockerState::MovingToProductBox: return TEXT("MovingToProductBox");
    case ERestockerState::MovingToShelf: return TEXT("MovingToShelf");
    case ERestockerState::Restocking: return TEXT("Restocking");
    default: return TEXT("Unknown");
    }
}

void ARestockerAI::MoveToAccessPoint()
{
    if (AIController)
    {
        AIController->MoveToLocation(CurrentAccessPoint, 10.0f);
    }
}

FVector ARestockerAI::FindNearestAccessPoint(AShelf* Shelf)
{
    TArray<FVector> AccessPoints = Shelf->GetAllAccessPointLocations();
    FVector AILocation = GetActorLocation();
    FVector NearestPoint = AccessPoints[0];
    float NearestDistance = FVector::DistSquared(AILocation, NearestPoint);

    for (const FVector& Point : AccessPoints)
    {
        float Distance = FVector::DistSquared(AILocation, Point);
        if (Distance < NearestDistance)
        {
            NearestPoint = Point;
            NearestDistance = Distance;
        }
    }

    return NearestPoint;
}

void ARestockerAI::TurnToFaceTarget()
{
    if (TargetShelf)
    {
        FVector Direction = TargetShelf->GetActorLocation() - GetActorLocation();
        Direction.Z = 0;
        TargetRotation = Direction.Rotation();
        CurrentRotationAlpha = 0.0f;
        bIsRotating = true;
        SetActorTickEnabled(true);
    }
    else
    {
        OnRotationComplete();
    }
}

void ARestockerAI::OnRotationComplete()
{
    bIsRotating = false;
    RestockShelf();
}

void ARestockerAI::CheckRestockProgress()
{
    if (TargetShelf && TargetProductBox)
    {
        RemainingProducts = TargetProductBox->GetProductCountt();

        if (TargetShelf->IsFullyStocked() || RemainingProducts == 0)
        {
            GetWorld()->GetTimerManager().ClearTimer(RestockTimerHandle);

            if (RemainingProducts == 0)
            {
                TargetProductBox->Destroy();
                bIsHoldingProductBox = false;
                TargetProductBox = nullptr;
                NotifyTaskComplete();
            }
            else
            {
                // Check if there's another shelf that needs this product
                AShelf* NextShelf = FindNextShelfToRestock();
                if (NextShelf)
                {
                    // Move to the next shelf
                    TargetShelf = NextShelf;
                    SetState(ERestockerState::MovingToShelf);
                    MoveToTarget(TargetShelf);
                }
                else
                {
                    // If no more shelves need this product, drop the box
                    DropCurrentBox();
                    NotifyTaskComplete();
                }
            }
        }
    }
    else
    {
        GetWorld()->GetTimerManager().ClearTimer(RestockTimerHandle);
        AbortCurrentTask();
    }
}

void ARestockerAI::UpdateRotation(float DeltaTime)
{
    if (bIsRotating)
    {
        FRotator CurrentRotation = GetActorRotation();
        FRotator NewRotation = FMath::RInterpConstantTo(CurrentRotation, TargetRotation, DeltaTime, RotationSpeed);
        SetActorRotation(NewRotation);

        // Check if we've essentially reached the target rotation
        if (NewRotation.Equals(TargetRotation, 0.1f))
        {
            bIsRotating = false;
            OnRotationComplete();
        }
    }
}

void ARestockerAI::DropCurrentBox()
{
    if (bIsHoldingProductBox && TargetProductBox)
    {
        TargetProductBox->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(TargetProductBox->GetRootComponent());
        if (PrimitiveComponent)
        {
            PrimitiveComponent->SetSimulatePhysics(true);
            PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        }

        bIsHoldingProductBox = false;
        TargetProductBox = nullptr;
    }
}

void ARestockerAI::AbortCurrentTask()
{
    if (bIsAbortingTask)
    {
        return; // Prevent recursive calls
    }

    bIsAbortingTask = true;

    if (bIsHoldingProductBox && TargetProductBox)
    {
        if (TargetProductBox->GetProductCount() == 0)
        {
            TargetProductBox->Destroy();
        }
        else
        {
            DropCurrentBox();
        }
    }

    if (Manager && TargetShelf)
    {
        Manager->ReleaseShelf(TargetShelf);
    }

    TargetShelf = nullptr;
    TargetProductBox = nullptr;
    SetState(ERestockerState::Idle);

    // Instead of calling NotifyTaskComplete, set a flag for the manager
    if (Manager)
    {
        Manager->MarkRestockerAsAvailable(this);
    }

    bIsAbortingTask = false;
}

void ARestockerAI::NotifyTaskComplete()
{
    if (Manager)
    {
        if (TargetShelf)
        {
            Manager->ReleaseShelf(TargetShelf);
        }
        Manager->OnRestockerTaskComplete(this);
    }
    SetState(ERestockerState::Idle);
}

AShelf* ARestockerAI::FindNextShelfToRestock()
{
    if (!Manager || !TargetProductBox)
    {
        return nullptr;
    }

    TSubclassOf<AProduct> ProductClassToRestock = TargetProductBox->GetProductClass();
    return Manager->FindShelfNeedingProduct(ProductClassToRestock, this);
}