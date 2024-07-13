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
    RotationSpeed = 10.0f; // Adjust this value to control rotation speed (higher = faster)
    CurrentRotationAlpha = 0.0f;
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

    if (bIsRotating)
    {
        UpdateRotation(DeltaTime);
    }
}

void ARestockerAI::StartRestocking()
{
    if (CurrentState != ERestockerState::Idle)
    {
        ////UE_LOG(LogTemp, Warning, TEXT("RestockerAI: Attempted to start restocking while not idle. Current state: %s"), *GetStateName(CurrentState));
        return;
    }

    ////UE_LOG(LogTemp, Display, TEXT("RestockerAI: Starting restocking process"));
    CheckedShelves.Empty();
    FindShelfToRestock();
}

void ARestockerAI::FindShelfToRestock()
{
    TArray<AActor*> FoundShelves;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AShelf::StaticClass(), FoundShelves);

    for (AActor* Actor : FoundShelves)
    {
        AShelf* Shelf = Cast<AShelf>(Actor);
        if (Shelf && !IsShelfSufficientlyStocked(Shelf) && Shelf->GetCurrentProductClass() != nullptr && !CheckedShelves.Contains(Shelf))
        {
            TargetShelf = Shelf;
            ////UE_LOG(LogTemp, Display, TEXT("RestockerAI: Found shelf to restock: %s"), *Shelf->GetName());
            FindProductBox();
            return;
        }
    }

    ////UE_LOG(LogTemp, Display, TEXT("RestockerAI: No shelf needs restocking, staying idle"));
    SetState(ERestockerState::Idle);
}

void ARestockerAI::FindProductBox()
{
    if (!TargetShelf)
    {
        //UE_LOG(LogTemp, Error, TEXT("RestockerAI: TargetShelf is null in FindProductBox"));
        SetState(ERestockerState::Idle);
        return;
    }

    TArray<AActor*> FoundProductBoxes;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AProductBox::StaticClass(), FoundProductBoxes);

    ////UE_LOG(LogTemp, Display, TEXT("RestockerAI: Found %d product boxes"), FoundProductBoxes.Num());

    for (AActor* Actor : FoundProductBoxes)
    {
        AProductBox* ProductBox = Cast<AProductBox>(Actor);
        if (ProductBox && ProductBox->GetProductClass() == TargetShelf->GetCurrentProductClass() && !IsBoxHeldByPlayer(ProductBox))
        {
            TargetProductBox = ProductBox;
            ////UE_LOG(LogTemp, Display, TEXT("RestockerAI: Found matching product box: %s"), *ProductBox->GetName());
            SetState(ERestockerState::MovingToProductBox);
            MoveToTarget(TargetProductBox);
            return;
        }
    }

    ////UE_LOG(LogTemp, Warning, TEXT("RestockerAI: No matching product box found for shelf %s"), *TargetShelf->GetName());
    CheckedShelves.Add(TargetShelf);
    TargetShelf = nullptr;
    FindShelfToRestock(); // Try to find another shelf to restock
}

void ARestockerAI::MoveToTarget(AActor* Target)
{
    if (!AIController || !Target)
    {
        UE_LOG(LogTemp, Error, TEXT("RestockerAI: Failed to move to target. AIController: %s, Target: %s"),
            AIController ? TEXT("Valid") : TEXT("Invalid"),
            Target ? TEXT("Valid") : TEXT("Invalid"));
        SetState(ERestockerState::Idle);
        return;
    }

    // Add a recursion guard
    static int32 RecursionCount = 0;
    if (RecursionCount > 5)
    {
        UE_LOG(LogTemp, Warning, TEXT("RestockerAI: Excessive recursion in MoveToTarget. Aborting."));
        SetState(ERestockerState::Idle);
        RecursionCount = 0;
        return;
    }
    RecursionCount++;

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

    RecursionCount--;
}

void ARestockerAI::PickUpProductBox()
{
    if (TargetProductBox)
    {
        bIsHoldingProductBox = true;

        // Disable physics and collision on the product box
        UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(TargetProductBox->GetRootComponent());
        if (PrimitiveComponent)
        {
            PrimitiveComponent->SetSimulatePhysics(false);
            PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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

        //UE_LOG(LogTemp, Display, TEXT("RestockerAI: Picked up product box: %s with %d products"), *TargetProductBox->GetName(), RemainingProducts);
        SetState(ERestockerState::MovingToShelf);
        MoveToTarget(TargetShelf);
    }
    else
    {
        //UE_LOG(LogTemp, Error, TEXT("RestockerAI: Failed to pick up product box, TargetProductBox is null"));
        SetState(ERestockerState::Idle);
    }
}

void ARestockerAI::RestockShelf()
{
    if (TargetShelf && bIsHoldingProductBox && TargetProductBox)
    {
        //UE_LOG(LogTemp, Display, TEXT("RestockerAI: Restocking shelf: %s"), *TargetShelf->GetName());
        TargetShelf->SetProductBox(TargetProductBox);
        TargetShelf->StartStockingShelf(TargetProductBox->GetProductClass());

        // Wait for the shelf to be fully stocked or the box to be empty
        GetWorld()->GetTimerManager().SetTimer(RestockTimerHandle, this, &ARestockerAI::CheckRestockProgress, 0.5f, true);
    }
    else
    {
        //UE_LOG(LogTemp, Error, TEXT("RestockerAI: Failed to restock shelf. TargetShelf: %s, Holding ProductBox: %s, TargetProductBox: %s"),
        //    TargetShelf ? TEXT("Valid") : TEXT("Invalid"),
        //   bIsHoldingProductBox ? TEXT("Yes") : TEXT("No"),
        //    TargetProductBox ? TEXT("Valid") : TEXT("Invalid"));
        SetState(ERestockerState::Idle);
        StartRestocking();
    }
}

void ARestockerAI::OnMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result)
{
    UE_LOG(LogTemp, Display, TEXT("RestockerAI: Move completed with result: %d"), static_cast<int32>(Result));

    if (Result == EPathFollowingResult::Success)
    {
        // Handle successful move
        switch (CurrentState)
        {
        case ERestockerState::MovingToProductBox:
            PickUpProductBox();
            break;
        case ERestockerState::MovingToShelf:
            if (bIsHoldingProductBox)
            {
                TurnToFaceTarget();
            }
            else
            {
                SetState(ERestockerState::Idle);
            }
            break;
        default:
            break;
        }
    }
    else
    {
        // Handle failed move
        UE_LOG(LogTemp, Warning, TEXT("RestockerAI: Move failed, retrying"));
        SetState(ERestockerState::Idle);
        // Instead of calling MoveToTarget directly, schedule a retry
        GetWorld()->GetTimerManager().SetTimer(RetryTimerHandle, this, &ARestockerAI::RetryMove, 1.0f, false);
    }
}

void ARestockerAI::SetState(ERestockerState NewState)
{
    ////UE_LOG(LogTemp, Display, TEXT("RestockerAI: State changing from %s to %s"), *GetStateName(CurrentState), *GetStateName(NewState));
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
        //UE_LOG(LogTemp, Display, TEXT("RestockerAI: Moving to access point: %s"), *CurrentAccessPoint.ToString());
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

        // Reset the rotation alpha
        CurrentRotationAlpha = 0.0f;

        bIsRotating = true;

        // Instead of using a timer, we'll update the rotation in the Tick function
        SetActorTickEnabled(true);
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

        //UE_LOG(LogTemp, Display, TEXT("RestockerAI: Shelf fully stocked, starting over"));
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
            //UE_LOG(LogTemp, Display, TEXT("RestockerAI: Found new box with same product: %s"), *ProductBox->GetName());
            SetState(ERestockerState::MovingToProductBox);
            MoveToTarget(TargetProductBox);
            return;
        }
    }

    //UE_LOG(LogTemp, Display, TEXT("RestockerAI: No more boxes with the same product, moving on to next task"));
    TargetShelf = nullptr;
    SetState(ERestockerState::Idle);
    StartRestocking();
}

void ARestockerAI::HandleEmptyBox()
{
    //UE_LOG(LogTemp, Display, TEXT("RestockerAI: Product box is empty, disposing of it"));

    if (TargetProductBox)
    {
        TargetProductBox->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        TargetProductBox->Destroy();
    }

    bIsHoldingProductBox = false;
    TargetProductBox = nullptr;

    if (TargetShelf && !TargetShelf->IsFullyStocked())
    {
        FindNewBoxWithSameProduct();
    }
    else
    {
        //UE_LOG(LogTemp, Display, TEXT("RestockerAI: Moving on to next task"));
        TargetShelf = nullptr;
        SetState(ERestockerState::Idle);
        StartRestocking();
    }
}

void ARestockerAI::CheckRestockProgress()
{
    if (TargetShelf && TargetProductBox)
    {
        RemainingProducts = TargetProductBox->GetProductCountt();

        if (IsShelfSufficientlyStocked(TargetShelf) || RemainingProducts == 0)
        {
            GetWorld()->GetTimerManager().ClearTimer(RestockTimerHandle);

            if (RemainingProducts == 0)
            {
                HandleEmptyBox();
            }
            else if (IsShelfSufficientlyStocked(TargetShelf))
            {
                // Look for another shelf that needs the same product
                FindNextShelfForCurrentProduct();
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

void ARestockerAI::UpdateRotation(float DeltaTime)
{
    if (bIsRotating)
    {
        CurrentRotationAlpha += DeltaTime * RotationSpeed;

        if (CurrentRotationAlpha >= 1.0f)
        {
            // Rotation complete
            SetActorRotation(TargetRotation);
            bIsRotating = false;
            OnRotationComplete();
        }
        else
        {
            // Interpolate rotation
            FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, RotationSpeed);
            SetActorRotation(NewRotation);
        }
    }
}

void ARestockerAI::DropCurrentBox()
{
    if (bIsHoldingProductBox && TargetProductBox)
    {
        //UE_LOG(LogTemp, Display, TEXT("RestockerAI: Dropping current box"));
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

void ARestockerAI::FindNextShelfForCurrentProduct()
{
    TArray<AActor*> FoundShelves;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AShelf::StaticClass(), FoundShelves);

    for (AActor* Actor : FoundShelves)
    {
        AShelf* Shelf = Cast<AShelf>(Actor);
        if (Shelf && !IsShelfSufficientlyStocked(Shelf) && Shelf->GetCurrentProductClass() == CurrentProductClass)
        {
            TargetShelf = Shelf;
            //UE_LOG(LogTemp, Display, TEXT("RestockerAI: Found another shelf for current product: %s"), *Shelf->GetName());
            SetState(ERestockerState::MovingToShelf);
            MoveToTarget(TargetShelf);
            return;
        }
    }

    // If no shelf needs the current product, drop the box and start over
    //UE_LOG(LogTemp, Display, TEXT("RestockerAI: No more shelves need current product, dropping box and starting over"));
    DropCurrentBox();
    SetState(ERestockerState::Idle);
    StartRestocking();
}

bool ARestockerAI::IsShelfSufficientlyStocked(AShelf* Shelf)
{
    if (!Shelf)
    {
        return true;
    }

    int32 CurrentStock = Shelf->GetCurrentStock();
    int32 MaxStock = Shelf->GetMaxStock();

    // Consider the shelf sufficiently stocked if it has at most 2 items less than the maximum
    return (MaxStock - CurrentStock) <= 2;
}

void ARestockerAI::RetryMove()
{
    if (CurrentState == ERestockerState::MovingToProductBox && TargetProductBox)
    {
        MoveToTarget(TargetProductBox);
    }
    else if (CurrentState == ERestockerState::MovingToShelf && TargetShelf)
    {
        MoveToTarget(TargetShelf);
    }
    else
    {
        SetState(ERestockerState::Idle);
    }
}