// RestockerAI.cpp
#include "RestockerAI.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "SupermarketCharacter.h"

ARestockerAI::ARestockerAI()
{
    PrimaryActorTick.bCanEverTick = true;
    CurrentState = ERestockerState::Idle;
    bIsHoldingProductBox = false;
   
}

void ARestockerAI::BeginPlay()
{
    Super::BeginPlay();
    AIController = Cast<AAIController>(GetController());
    if (AIController)
    {
        AIController->ReceiveMoveCompleted.AddDynamic(this, &ARestockerAI::OnMoveCompleted);
    }

}

void ARestockerAI::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (CurrentState == ERestockerState::Idle)
    {
        StartRestocking();
    }
}

void ARestockerAI::StartRestocking()
{
    if (CurrentState != ERestockerState::Idle)
    {
        UE_LOG(LogTemp, Warning, TEXT("RestockerAI: Attempted to start restocking while not idle. Current state: %s"), *GetStateName(CurrentState));
        return;
    }

    //UE_LOG(LogTemp, Display, TEXT("RestockerAI: Starting restocking process"));
    FindShelfToRestock();
}

void ARestockerAI::FindShelfToRestock()
{
    TArray<AActor*> FoundShelves;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AShelf::StaticClass(), FoundShelves);

    //UE_LOG(LogTemp, Display, TEXT("RestockerAI: Found %d shelves"), FoundShelves.Num());

    for (AActor* Actor : FoundShelves)
    {
        AShelf* Shelf = Cast<AShelf>(Actor);
        if (Shelf && !Shelf->IsFullyStocked() && Shelf->GetCurrentProductClass() != nullptr)
        {
            TargetShelf = Shelf;
            UE_LOG(LogTemp, Display, TEXT("RestockerAI: Found shelf to restock: %s"), *Shelf->GetName());
            FindProductBox();
            return;
        }
    }

    //UE_LOG(LogTemp, Display, TEXT("RestockerAI: No shelf needs restocking, staying idle"));
    SetState(ERestockerState::Idle);
}

void ARestockerAI::FindProductBox()
{
    if (!TargetShelf)
    {
        UE_LOG(LogTemp, Error, TEXT("RestockerAI: TargetShelf is null in FindProductBox"));
        SetState(ERestockerState::Idle);
        return;
    }

    TArray<AActor*> FoundProductBoxes;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AProductBox::StaticClass(), FoundProductBoxes);

    UE_LOG(LogTemp, Display, TEXT("RestockerAI: Found %d product boxes"), FoundProductBoxes.Num());

    for (AActor* Actor : FoundProductBoxes)
    {
        AProductBox* ProductBox = Cast<AProductBox>(Actor);
        if (ProductBox && ProductBox->GetProductClass() == TargetShelf->GetCurrentProductClass() && !IsBoxHeldByPlayer(ProductBox))
        {
            TargetProductBox = ProductBox;
            UE_LOG(LogTemp, Display, TEXT("RestockerAI: Found matching product box: %s"), *ProductBox->GetName());
            SetState(ERestockerState::MovingToProductBox);
            MoveToTarget(TargetProductBox);
            return;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("RestockerAI: No matching product box found, staying idle"));
    SetState(ERestockerState::Idle);
}

void ARestockerAI::MoveToTarget(AActor* Target)
{
    if (AIController && Target)
    {
        if (AShelf* Shelf = Cast<AShelf>(Target))
        {
            CurrentAccessPoint = FindNearestAccessPoint(Shelf);
            MoveToAccessPoint();
        }
        else
        {
            UE_LOG(LogTemp, Display, TEXT("RestockerAI: Moving to target: %s"), *Target->GetName());
            AIController->MoveToActor(Target, 50.0f); // 50.0f is the acceptance radius
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("RestockerAI: Failed to move to target. AIController: %s, Target: %s"),
            AIController ? TEXT("Valid") : TEXT("Invalid"),
            Target ? TEXT("Valid") : TEXT("Invalid"));
        SetState(ERestockerState::Idle);
    }
}

void ARestockerAI::PickUpProductBox()
{
    if (TargetProductBox)
    {
        bIsHoldingProductBox = true;

        // Disable physics on the product box
        UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(TargetProductBox->GetRootComponent());
        if (PrimitiveComponent)
        {
            PrimitiveComponent->SetSimulatePhysics(false);
        }

        // Attach the product box to the AI
        FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, false);
        TargetProductBox->AttachToActor(this, AttachmentRules);

        // Set the relative location of the product box
        FVector AttachmentOffset(50.0f, 0.0f, 0.0f); // Adjust these values as needed
        TargetProductBox->SetActorRelativeLocation(AttachmentOffset);

        // Get the number of products in the box
        RemainingProducts = TargetProductBox->GetProductCountt();
        CurrentProductClass = TargetProductBox->GetProductClass();

        UE_LOG(LogTemp, Display, TEXT("RestockerAI: Picked up product box: %s with %d products"), *TargetProductBox->GetName(), RemainingProducts);
        SetState(ERestockerState::MovingToShelf);
        MoveToTarget(TargetShelf);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("RestockerAI: Failed to pick up product box, TargetProductBox is null"));
        SetState(ERestockerState::Idle);
    }
}

void ARestockerAI::RestockShelf()
{
    if (TargetShelf && bIsHoldingProductBox && TargetProductBox)
    {
        UE_LOG(LogTemp, Display, TEXT("RestockerAI: Restocking shelf: %s"), *TargetShelf->GetName());
        TargetShelf->SetProductBox(TargetProductBox);
        TargetShelf->StartStockingShelf(TargetProductBox->GetProductClass());

        // Wait for the shelf to be fully stocked or the box to be empty
        GetWorld()->GetTimerManager().SetTimer(RestockTimerHandle, this, &ARestockerAI::CheckRestockProgress, 0.5f, true);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("RestockerAI: Failed to restock shelf. TargetShelf: %s, Holding ProductBox: %s, TargetProductBox: %s"),
            TargetShelf ? TEXT("Valid") : TEXT("Invalid"),
            bIsHoldingProductBox ? TEXT("Yes") : TEXT("No"),
            TargetProductBox ? TEXT("Valid") : TEXT("Invalid"));
        SetState(ERestockerState::Idle);
        StartRestocking();
    }
}

void ARestockerAI::OnMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result)
{
    UE_LOG(LogTemp, Display, TEXT("RestockerAI: Move completed with result: %d"), static_cast<int32>(Result));

    if (Result == EPathFollowingResult::Success)
    {
        switch (CurrentState)
        {
        case ERestockerState::MovingToProductBox:
            UE_LOG(LogTemp, Display, TEXT("RestockerAI: Reached product box, picking up"));
            PickUpProductBox();
            break;
        case ERestockerState::MovingToShelf:
            if (bIsHoldingProductBox && TargetProductBox && TargetProductBox->GetAttachParentActor() == this)
            {
                UE_LOG(LogTemp, Display, TEXT("RestockerAI: Reached shelf, turning to face it"));
                TurnToFaceTarget();
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("RestockerAI: Reached shelf but not holding product box, returning to idle"));
                SetState(ERestockerState::Idle);
            }
            break;
        default:
            UE_LOG(LogTemp, Warning, TEXT("RestockerAI: Move completed in unexpected state: %s"), *GetStateName(CurrentState));
            break;
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("RestockerAI: Move failed, retrying"));
        if (CurrentState == ERestockerState::MovingToProductBox || CurrentState == ERestockerState::MovingToShelf)
        {
            // Retry the move
            if (TargetProductBox && CurrentState == ERestockerState::MovingToProductBox)
            {
                MoveToTarget(TargetProductBox);
            }
            else if (TargetShelf && CurrentState == ERestockerState::MovingToShelf)
            {
                MoveToTarget(TargetShelf);
            }
        }
        else
        {
            SetState(ERestockerState::Idle);
        }
    }
}

void ARestockerAI::SetState(ERestockerState NewState)
{
    //UE_LOG(LogTemp, Display, TEXT("RestockerAI: State changing from %s to %s"), *GetStateName(CurrentState), *GetStateName(NewState));
    CurrentState = NewState;
}



FString ARestockerAI::GetStateName(ERestockerState State)
{
    switch (State)
    {
    case ERestockerState::Idle:
        return TEXT("Idle");
    case ERestockerState::MovingToProductBox:
        return TEXT("MovingToProductBox");
    case ERestockerState::MovingToShelf:
        return TEXT("MovingToShelf");
    case ERestockerState::Restocking:
        return TEXT("Restocking");
    default:
        return TEXT("Unknown");
    }
}

void ARestockerAI::MoveToAccessPoint()
{
    if (AIController)
    {
        UE_LOG(LogTemp, Display, TEXT("RestockerAI: Moving to access point: %s"), *CurrentAccessPoint.ToString());
        AIController->MoveToLocation(CurrentAccessPoint, 30.0f); // 50.0f is the acceptance radius
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
        Direction.Z = 0;  // We don't want to pitch up or down
        TargetRotation = Direction.Rotation();

        bIsRotating = true;

        // Start a timer to check rotation progress
        GetWorld()->GetTimerManager().SetTimer(RotationTimerHandle, this, &ARestockerAI::OnRotationComplete, 0.01f, true);
    }
    else
    {
        OnRotationComplete();
    }
}

void ARestockerAI::OnRotationComplete()
{
    FRotator CurrentRotation = GetActorRotation();
    if (FMath::IsNearlyEqual(CurrentRotation.Yaw, TargetRotation.Yaw, 1.0f))
    {
        GetWorld()->GetTimerManager().ClearTimer(RotationTimerHandle);
        bIsRotating = false;

        // Now that we're facing the shelf, start restocking
        RestockShelf();
    }
    else
    {
        // Continue rotating
        FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, GetWorld()->GetDeltaSeconds(), 5.0f);
        SetActorRotation(NewRotation);
    }
}

void ARestockerAI::CheckShelfStocked()
{
    if (TargetShelf && TargetShelf->IsFullyStocked())
    {
        GetWorld()->GetTimerManager().ClearTimer(RestockTimerHandle);

        // Drop the product box
        if (TargetProductBox)
        {
            TargetProductBox->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
            UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(TargetProductBox->GetRootComponent());
            if (PrimitiveComponent)
            {
                PrimitiveComponent->SetSimulatePhysics(true);
            }
        }

        bIsHoldingProductBox = false;
        TargetProductBox = nullptr;
        TargetShelf = nullptr;

        UE_LOG(LogTemp, Display, TEXT("RestockerAI: Shelf fully stocked, starting over"));
        SetState(ERestockerState::Idle);
        StartRestocking();
    }
}

bool ARestockerAI::IsBoxHeldByPlayer(AProductBox* Box)
{
    if (!Box)
    {
        return false;
    }

    ASupermarketCharacter* PlayerCharacter = Cast<ASupermarketCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
    if (PlayerCharacter)
    {
        return PlayerCharacter->IsHoldingProductBox(Box);
    }

    return false;
}

void ARestockerAI::FindNewBoxWithSameProduct()
{
    TArray<AActor*> FoundProductBoxes;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AProductBox::StaticClass(), FoundProductBoxes);

    for (AActor* Actor : FoundProductBoxes)
    {
        AProductBox* ProductBox = Cast<AProductBox>(Actor);
        if (ProductBox && ProductBox->GetProductClass() == CurrentProductClass && !IsBoxHeldByPlayer(ProductBox))
        {
            TargetProductBox = ProductBox;
            UE_LOG(LogTemp, Display, TEXT("RestockerAI: Found new box with same product: %s"), *ProductBox->GetName());
            SetState(ERestockerState::MovingToProductBox);
            MoveToTarget(TargetProductBox);
            return;
        }
    }

    UE_LOG(LogTemp, Display, TEXT("RestockerAI: No more boxes with the same product, starting over"));
    SetState(ERestockerState::Idle);
    StartRestocking();
}

void ARestockerAI::HandleEmptyBox()
{
    UE_LOG(LogTemp, Display, TEXT("RestockerAI: Product box is empty, disposing of it"));

    if (TargetProductBox)
    {
        TargetProductBox->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        TargetProductBox->Destroy();
    }

    bIsHoldingProductBox = false;
    TargetProductBox = nullptr;

    FindNewBoxWithSameProduct();
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
                HandleEmptyBox();
            }
            else
            {
                UE_LOG(LogTemp, Display, TEXT("RestockerAI: Shelf fully stocked, looking for new shelf"));
                SetState(ERestockerState::Idle);
                StartRestocking();
            }
        }
    }
    else
    {
        GetWorld()->GetTimerManager().ClearTimer(RestockTimerHandle);
        SetState(ERestockerState::Idle);
        StartRestocking();
    }
}