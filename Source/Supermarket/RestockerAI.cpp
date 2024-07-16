// RestockerAI.cpp
#include "RestockerAI.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "TimerManager.h"
#include "BigShelf.h"
#include "SupermarketCharacter.h"

TMap<AShelf*, ARestockerAI*> ARestockerAI::ReservedShelves;
TMap<AProductBox*, ARestockerAI*> ARestockerAI::ReservedProductBoxes;
FCriticalSection ARestockerAI::ReservationLock;

TMap<AProductBox*, ARestockerAI*> ARestockerAI::TargetedProductBoxes;
FCriticalSection ARestockerAI::TargetedProductBoxesLock;

TMap<AProductBox*, ARestockerAI*> ARestockerAI::LockedProductBoxes;
FCriticalSection ARestockerAI::LockedProductBoxesLock;
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
    StartPeriodicShelfCheck();

    GetWorld()->GetTimerManager().SetTimer(ClearReservationsTimerHandle,FTimerDelegate::CreateUObject(this, &ARestockerAI::ClearUnattachedProductBoxReservations),15.0f, true);
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
        //UE_LOGLogTemp, Warning, TEXT("RestockerAI: Attempted to start restocking while not idle. Current state: %s"), *GetStateName(CurrentState));
        return;
    }

    //UE_LOGLogTemp, Display, TEXT("RestockerAI: Starting restocking process"));
    CheckedShelves.Empty();
    FindShelfToRestock();
}

void ARestockerAI::FindShelfToRestock()
{
    TArray<AActor*> FoundBigShelves;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABigShelf::StaticClass(), FoundBigShelves);

    // Randomize the order of BigShelves
    for (int32 i = FoundBigShelves.Num() - 1; i > 0; --i)
    {
        int32 j = FMath::RandRange(0, i);
        FoundBigShelves.Swap(i, j);
    }

    for (AActor* Actor : FoundBigShelves)
    {
        ABigShelf* BigShelf = Cast<ABigShelf>(Actor);
        if (BigShelf)
        {
            TArray<AShelf*> EligibleShelves;

            // Collect eligible shelves from this BigShelf
            for (AShelf* Shelf : BigShelf->Shelves)
            {
                if (Shelf && !IsShelfSufficientlyStocked(Shelf) &&
                    Shelf->GetCurrentProductClass() != nullptr &&
                    !CheckedShelves.Contains(Shelf) &&
                    !IsShelfReserved(Shelf))
                {
                    EligibleShelves.Add(Shelf);
                }
            }

            // Randomize the order of eligible shelves within this BigShelf
            for (int32 i = EligibleShelves.Num() - 1; i > 0; --i)
            {
                int32 j = FMath::RandRange(0, i);
                EligibleShelves.Swap(i, j);
            }

            // Try to reserve a random eligible shelf from this BigShelf
            for (AShelf* Shelf : EligibleShelves)
            {
                if (ReserveShelf(Shelf))
                {
                    TargetShelf = Shelf;
                    //UE_LOG(LogTemp, Display, TEXT("RestockerAI: Found shelf to restock: %s in BigShelf: %s"),*Shelf->GetName(), *BigShelf->GetName());
                    FindProductBox();
                    return;
                }
            }
        }
    }

    //UE_LOG(LogTemp, Display, TEXT("RestockerAI: No shelf needs restocking, staying idle"));
    SetState(ERestockerState::Idle);
}

void ARestockerAI::FindProductBox()
{
    if (!TargetShelf)
    {
        //UE_LOGLogTemp, Error, TEXT("RestockerAI: TargetShelf is null in FindProductBox"));
        SetState(ERestockerState::Idle);
        return;
    }

    TArray<AActor*> FoundProductBoxes;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AProductBox::StaticClass(), FoundProductBoxes);

    //UE_LOGLogTemp, Display, TEXT("RestockerAI: Found %d product boxes"), FoundProductBoxes.Num());

    for (AActor* Actor : FoundProductBoxes)
    {
        AProductBox* ProductBox = Cast<AProductBox>(Actor);
        if (ProductBox && ProductBox->GetProductClass() == TargetShelf->GetCurrentProductClass())
        {
            // Check if the ProductBox is attached to any actor
            if (ProductBox->GetAttachParentActor())
            {
                //UE_LOGLogTemp, Verbose, TEXT("RestockerAI: ProductBox %s is attached to another actor, skipping"), *ProductBox->GetName());
                continue; // Skip this ProductBox as it's already attached to something
            }

            if (!IsBoxHeldByPlayer(ProductBox) && !IsProductBoxReserved(ProductBox) && !IsProductBoxLocked(ProductBox))
            {
                if (LockProductBox(ProductBox) && ReserveProductBox(ProductBox))
                {
                    TargetProductBox = ProductBox;
                    //UE_LOGLogTemp, Display, TEXT("RestockerAI: Found and locked matching product box: %s"), *ProductBox->GetName());
                    SetState(ERestockerState::MovingToProductBox);
                    MoveToTarget(TargetProductBox);
                    return;
                }
            }
        }
    }

    //UE_LOGLogTemp, Warning, TEXT("RestockerAI: No matching product box found for shelf %s"), *TargetShelf->GetName());
    CheckedShelves.Add(TargetShelf);
    ReleaseShelf(TargetShelf);
    TargetShelf = nullptr;
    FindShelfToRestock(); // Try to find another shelf to restock
}

void ARestockerAI::MoveToTarget(AActor* Target)
{
    if (!AIController || !Target)
    {
        //UE_LOGLogTemp, Error, TEXT("RestockerAI: Failed to move to target. AIController: %s, Target: %s"),AIController ? TEXT("Valid") : TEXT("Invalid"),Target ? TEXT("Valid") : TEXT("Invalid"));
        SetState(ERestockerState::Idle);
        return;
    }

    // Add a recursion guard
    static int32 RecursionCount = 0;
    if (RecursionCount > 5)
    {
        //UE_LOGLogTemp, Warning, TEXT("RestockerAI: Excessive recursion in MoveToTarget. Aborting."));
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
        //UE_LOGLogTemp, Display, TEXT("RestockerAI: Moving to target: %s"), *Target->GetName());
        AIController->MoveToActor(Target, 50.0f); // 50.0f is the acceptance radius
    }

    RecursionCount--;
}

void ARestockerAI::PickUpProductBox()
{
    if (TargetProductBox && IsProductBoxLocked(TargetProductBox) && !TargetProductBox->GetAttachParentActor())
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

        //UE_LOGLogTemp, Display, TEXT("RestockerAI: Picked up product box: %s with %d products"), *TargetProductBox->GetName(), RemainingProducts);
        SetState(ERestockerState::MovingToShelf);
        MoveToTarget(TargetShelf);
    }
    else
    {
        //UE_LOGLogTemp, Error, TEXT("RestockerAI: Failed to pick up product box, TargetProductBox is null, not locked, or already attached"));
        AbortCurrentTask();
    }
}

void ARestockerAI::RestockShelf()
{
    if (TargetShelf && bIsHoldingProductBox && TargetProductBox)
    {
        //UE_LOGLogTemp, Display, TEXT("RestockerAI: Restocking shelf: %s"), *TargetShelf->GetName());
        TargetShelf->SetProductBox(TargetProductBox);
        TargetShelf->StartStockingShelf(TargetProductBox->GetProductClass());

        // Wait for the shelf to be fully stocked or the box to be empty
        GetWorld()->GetTimerManager().SetTimer(RestockTimerHandle, this, &ARestockerAI::CheckRestockProgress, 0.5f, true);
    }
    else
    {
        //UE_LOGLogTemp, Error, TEXT("RestockerAI: Failed to restock shelf. TargetShelf: %s, Holding ProductBox: %s, TargetProductBox: %s"),TargetShelf ? TEXT("Valid") : TEXT("Invalid"),bIsHoldingProductBox ? TEXT("Yes") : TEXT("No"),TargetProductBox ? TEXT("Valid") : TEXT("Invalid"));
        SetState(ERestockerState::Idle);
        ReleaseAllReservations();
        StartRestocking();
    }
}

void ARestockerAI::OnMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result)
{
    //UE_LOGLogTemp, Display, TEXT("RestockerAI: Move completed with result: %d"), static_cast<int32>(Result));

    if (Result == EPathFollowingResult::Success)
    {
        switch (CurrentState)
        {
        case ERestockerState::MovingToProductBox:
            if (TargetProductBox && IsProductBoxLocked(TargetProductBox) && !TargetProductBox->GetAttachParentActor())
            {
                PickUpProductBox();
            }
            else
            {
                //UE_LOGLogTemp, Warning, TEXT("RestockerAI: Target product box is no longer valid, locked, or is attached to something"));
                AbortCurrentTask();
            }
            break;

        case ERestockerState::MovingToShelf:
            if (bIsHoldingProductBox && TargetShelf)
            {
                TurnToFaceTarget();
            }
            else
            {
                //UE_LOGLogTemp, Warning, TEXT("RestockerAI: No longer holding product box or target shelf is invalid"));
                AbortCurrentTask();
            }
            break;

        case ERestockerState::Restocking:
            // This case might occur if we've just finished turning to face the shelf
            if (bIsHoldingProductBox && TargetShelf)
            {
                RestockShelf();
            }
            else
            {
                //UE_LOGLogTemp, Warning, TEXT("RestockerAI: Cannot restock, no product box or invalid shelf"));
                AbortCurrentTask();
            }
            break;

        default:
            //UE_LOGLogTemp, Warning, TEXT("RestockerAI: Unexpected state %s after successful move"), *GetStateName(CurrentState));
            AbortCurrentTask();
            break;
        }
    }
    else
    {
        //UE_LOGLogTemp, Warning, TEXT("RestockerAI: Move failed, aborting current task"));
        AbortCurrentTask();
    }

    // If we're not actively doing something, go back to idle
    if (CurrentState != ERestockerState::MovingToProductBox &&
        CurrentState != ERestockerState::MovingToShelf &&
        CurrentState != ERestockerState::Restocking)
    {
        SetState(ERestockerState::Idle);
        StartRestocking(); // Try to find a new task immediately
    }
}

void ARestockerAI::SetState(ERestockerState NewState)
{
    //UE_LOGLogTemp, Display, TEXT("RestockerAI: State changing from %s to %s"), *GetStateName(CurrentState), *GetStateName(NewState));
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
        //UE_LOGLogTemp, Display, TEXT("RestockerAI: Moving to access point: %s"), *CurrentAccessPoint.ToString());
        AIController->MoveToLocation(CurrentAccessPoint, 10.0f); // 50.0f is the acceptance radius
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
                UE_LOG(LogTemp, Display, TEXT("RestockerAI: Product box is empty, deleting it"));
                // Delete the empty ProductBox
                TargetProductBox->Destroy();
                bIsHoldingProductBox = false;
                TargetProductBox = nullptr;
                ReleaseShelf(TargetShelf);
                TargetShelf = nullptr;
                SetState(ERestockerState::Idle);
                StartRestocking();
            }
            else
            {
                if (IsShelfSufficientlyStocked(TargetShelf))
                {
                    UE_LOG(LogTemp, Display, TEXT("RestockerAI: Shelf fully stocked, checking for other shelves needing the same product"));
                    CurrentProductClass = TargetProductBox->GetProductClass();
                    ReleaseShelf(TargetShelf);
                    TargetShelf = nullptr;
                    FindNextShelfForCurrentProduct();
                }
                else
                {
                    UE_LOG(LogTemp, Display, TEXT("RestockerAI: Box empty but shelf not fully stocked, looking for more boxes"));
                    DropCurrentBox();
                    ReleaseShelf(TargetShelf);
                    TargetShelf = nullptr;
                    SetState(ERestockerState::Idle);
                    StartRestocking();
                }
            }
        }
    }
    else
    {
        GetWorld()->GetTimerManager().ClearTimer(RestockTimerHandle);
        UE_LOG(LogTemp, Warning, TEXT("RestockerAI: Lost target shelf or product box during restocking"));
        AbortCurrentTask();
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
        UE_LOG(LogTemp, Display, TEXT("RestockerAI: Preparing to drop current box"));

        // Clear all reservations for this product box
        {
            FScopeLock Lock(&ReservationLock);
            FScopeLock TargetLock(&TargetedProductBoxesLock);
            FScopeLock LockLock(&LockedProductBoxesLock);

            ReservedProductBoxes.Remove(TargetProductBox);
            TargetedProductBoxes.Remove(TargetProductBox);
            LockedProductBoxes.Remove(TargetProductBox);
        }

        // Release the product box
        ReleaseProductBox(TargetProductBox);

        UE_LOG(LogTemp, Display, TEXT("RestockerAI: All reservations cleared, now dropping box physically"));

        // Now physically drop the box
        TargetProductBox->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(TargetProductBox->GetRootComponent());
        if (PrimitiveComponent)
        {
            PrimitiveComponent->SetSimulatePhysics(true);
            PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        }

        bIsHoldingProductBox = false;
        TargetProductBox = nullptr;

        UE_LOG(LogTemp, Display, TEXT("RestockerAI: Box fully dropped and available for others"));
    }
}

void ARestockerAI::FindNextShelfForCurrentProduct()
{
    TArray<AActor*> FoundShelves;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AShelf::StaticClass(), FoundShelves);

    for (AActor* Actor : FoundShelves)
    {
        AShelf* Shelf = Cast<AShelf>(Actor);
        if (Shelf && !IsShelfSufficientlyStocked(Shelf) && Shelf->GetCurrentProductClass() == CurrentProductClass && !IsShelfReserved(Shelf))
        {
            if (ReserveShelf(Shelf))
            {
                ReleaseShelf(TargetShelf);
                TargetShelf = Shelf;
                UE_LOG(LogTemp, Display, TEXT("RestockerAI: Found another shelf for current product: %s"), *Shelf->GetName());
                SetState(ERestockerState::MovingToShelf);
                MoveToTarget(TargetShelf);
                return;
            }
        }
    }

    // If no shelf needs the current product, drop the box and start over
    UE_LOG(LogTemp, Display, TEXT("RestockerAI: No more shelves need current product, dropping box and starting over"));
    DropCurrentBox();
    SetState(ERestockerState::Idle);
    ReleaseAllReservations();
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
    StartRestocking();
}

void ARestockerAI::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    StopPeriodicShelfCheck();
    ReleaseAllReservations();

    // Clear the timer when the actor is destroyed
    GetWorld()->GetTimerManager().ClearTimer(ClearReservationsTimerHandle);
}


bool ARestockerAI::ReserveShelf(AShelf* Shelf)
{
    FScopeLock Lock(&ReservationLock);
    if (!ReservedShelves.Contains(Shelf))
    {
        ReservedShelves.Add(Shelf, this);
        return true;
    }
    return false;
}

void ARestockerAI::ReleaseShelf(AShelf* Shelf)
{
    FScopeLock Lock(&ReservationLock);
    ReservedShelves.Remove(Shelf);
}

bool ARestockerAI::ReserveProductBox(AProductBox* ProductBox)
{
    FScopeLock Lock(&ReservationLock);
    if (!ReservedProductBoxes.Contains(ProductBox))
    {
        ReservedProductBoxes.Add(ProductBox, this);
        return true;
    }
    return false;
}

void ARestockerAI::ReleaseProductBox(AProductBox* ProductBox)
{
    FScopeLock Lock(&ReservationLock);
    ReservedProductBoxes.Remove(ProductBox);
}

void ARestockerAI::ReleaseProductBox()
{
    if (TargetProductBox)
    {
        UnlockProductBox();
        ReleaseProductBox(TargetProductBox);
        TargetProductBox = nullptr;
    }
    bIsHoldingProductBox = false;
}

bool ARestockerAI::IsShelfReserved(AShelf* Shelf) const
{
    FScopeLock Lock(&ReservationLock);
    return ReservedShelves.Contains(Shelf) && ReservedShelves[Shelf] != this;
}

bool ARestockerAI::IsProductBoxReserved(AProductBox* ProductBox) const
{
    FScopeLock Lock(&ReservationLock);
    return ReservedProductBoxes.Contains(ProductBox) && ReservedProductBoxes[ProductBox] != this;
}

void ARestockerAI::ReleaseAllReservations()
{
    FScopeLock Lock(&ReservationLock);
    for (auto It = ReservedShelves.CreateIterator(); It; ++It)
    {
        if (It.Value() == this)
        {
            It.RemoveCurrent();
        }
    }
    for (auto It = ReservedProductBoxes.CreateIterator(); It; ++It)
    {
        if (It.Value() == this)
        {
            It.RemoveCurrent();
        }
    }
    UnlockProductBox();
}




bool ARestockerAI::LockProductBox(AProductBox* ProductBox)
{
    FScopeLock Lock(&LockedProductBoxesLock);
    if (!LockedProductBoxes.Contains(ProductBox))
    {
        LockedProductBoxes.Add(ProductBox, this);
        return true;
    }
    return false;
}

void ARestockerAI::UnlockProductBox()
{
    FScopeLock Lock(&LockedProductBoxesLock);
    LockedProductBoxes.Remove(TargetProductBox);
}

bool ARestockerAI::IsProductBoxLocked(AProductBox* ProductBox) const
{
    FScopeLock Lock(&LockedProductBoxesLock);
    return LockedProductBoxes.Contains(ProductBox);
}

void ARestockerAI::AbortCurrentTask()
{
    if (bIsHoldingProductBox && TargetProductBox)
    {
        if (TargetProductBox->GetProductCountt() == 0)
        {
            // Delete the empty ProductBox
            TargetProductBox->Destroy();
        }
        else
        {
            // Drop the product box only if it's not empty
            TargetProductBox->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
            UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(TargetProductBox->GetRootComponent());
            if (PrimitiveComponent)
            {
                PrimitiveComponent->SetSimulatePhysics(true);
                PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            }
        }

        // Release the ProductBox
        UnlockProductBox();
        ReleaseProductBox(TargetProductBox);

        bIsHoldingProductBox = false;
        TargetProductBox = nullptr;
    }

    if (TargetShelf)
    {
        ReleaseShelf(TargetShelf);
        TargetShelf = nullptr;
    }

    SetState(ERestockerState::Idle);
    StartRestocking();
}

void ARestockerAI::StopPeriodicShelfCheck()
{
    GetWorldTimerManager().ClearTimer(ShelfCheckTimerHandle);
}

void ARestockerAI::StartPeriodicShelfCheck()
{
    // Check shelves every 5 seconds
    GetWorldTimerManager().SetTimer(ShelfCheckTimerHandle, this, &ARestockerAI::PeriodicShelfCheck, 5.0f, true);
}

void ARestockerAI::PeriodicShelfCheck()
{
    if (CurrentState == ERestockerState::Idle)
    {
        StartRestocking();
    }
}

void ARestockerAI::ClearUnattachedProductBoxReservations()
{
    TArray<AActor*> FoundProductBoxes;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AProductBox::StaticClass(), FoundProductBoxes);

    FScopeLock Lock(&ReservationLock);
    FScopeLock TargetLock(&TargetedProductBoxesLock);
    FScopeLock LockLock(&LockedProductBoxesLock);

    for (AActor* Actor : FoundProductBoxes)
    {
        AProductBox* ProductBox = Cast<AProductBox>(Actor);
        if (ProductBox && !ProductBox->GetAttachParentActor())
        {
            ReservedProductBoxes.Remove(ProductBox);
            TargetedProductBoxes.Remove(ProductBox);
            LockedProductBoxes.Remove(ProductBox);
        }
    }

    //UE_LOGLogTemp, Display, TEXT("Cleared reservations for unattached ProductBoxes"));
}