// RestockerAI.cpp
#include "RestockerAI.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "StorageRack.h"
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
    StorageRackSearchRadius = 10000.0f;
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
        return;
    }

    // First, check if we're holding a box
    if (bIsHoldingProductBox && TargetProductBox)
    {
        // Try to find a shelf that needs this product
        FindShelfToRestock();
        if (TargetShelf)
        {
            SetState(ERestockerState::MovingToShelf);
            MoveToTarget(TargetShelf);
            return;
        }

        // If no shelf needs this product, try to replenish storage
        if (ShouldReplenishStorageRack())
        {
            FindStorageRack();
            return;
        }

        // If we can't restock or replenish, drop the box
        DropCurrentBox();
    }

    // If we're not holding a box, look for shelves that need restocking
    FindShelfToRestock();
    if (TargetShelf)
    {
        // Try to find a suitable product box
        FindProductBox();
        if (TargetProductBox)
        {
            SetState(ERestockerState::MovingToProductBox);
            MoveToTarget(TargetProductBox);
            return;
        }
        else
        {
            // If no suitable box found, get one from storage
            AStorageRack* AppropriateRack = FindAppropriateStorageRack(TargetShelf->GetCurrentProductClass());
            if (AppropriateRack)
            {
                TargetStorageRack = AppropriateRack;
                SetState(ERestockerState::MovingToStorageRack);
                MoveToStorageRack();
                return;
            }
        }
    }

    // If we reach here, there's nothing to do
    UE_LOG(LogTemp, Display, TEXT("RestockerAI: No tasks to perform, idling"));
    SetState(ERestockerState::Idle);
}

void ARestockerAI::FindShelfToRestock()
{
    TArray<AActor*> FoundShelves;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AShelf::StaticClass(), FoundShelves);

    for (AActor* Actor : FoundShelves)
    {
        AShelf* Shelf = Cast<AShelf>(Actor);
        if (Shelf && !IsShelfSufficientlyStocked(Shelf) && !IsShelfReserved(Shelf))
        {
            if (bIsHoldingProductBox && TargetProductBox)
            {
                // If holding a box, only consider shelves with matching product type
                if (Shelf->GetCurrentProductClass() == TargetProductBox->GetProductClass())
                {
                    if (ReserveShelf(Shelf))
                    {
                        TargetShelf = Shelf;
                        SetState(ERestockerState::MovingToShelf);
                        MoveToTarget(TargetShelf);
                        return;
                    }
                }
            }
            else
            {
                // If not holding a box, consider any shelf that needs restocking
                if (ReserveShelf(Shelf))
                {
                    TargetShelf = Shelf;
                    return;
                }
            }
        }
    }

    TargetShelf = nullptr;
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
    if (TargetProductBox && !bIsHoldingProductBox)
    {
        bIsHoldingProductBox = true;

        // Disable physics simulation
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

        // Make sure the products in the box are visible
        TargetProductBox->MakeProductsVisible();

        UE_LOG(LogTemp, Display, TEXT("RestockerAI: Picked up product box: %s with %d products"), *TargetProductBox->GetName(), TargetProductBox->GetProductCount());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("RestockerAI: Failed to pick up product box: TargetProductBox is null or already holding a box"));
    }

    // After attempting to pick up the box, decide what to do next
    SetState(ERestockerState::Idle);
    StartRestocking();
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
    if (Result == EPathFollowingResult::Success)
    {
        switch (CurrentState)
        {
        case ERestockerState::MovingToProductBox:
            if (TargetProductBox && !bIsHoldingProductBox)
            {
                PickUpProductBox();
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Target product box is no longer valid or AI is already holding a box"));
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
                UE_LOG(LogTemp, Warning, TEXT("No longer holding product box or target shelf is invalid"));
                AbortCurrentTask();
            }
            break;

        case ERestockerState::MovingToStorageRack:
            if (TargetStorageRack)
            {
                if (bIsHoldingProductBox)
                {
                    ReplenishStorageRack();
                }
                else
                {
                    CreateProductBoxFromStorageRack();
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Target storage rack is no longer valid"));
                AbortCurrentTask();
            }
            break;

        default:
            UE_LOG(LogTemp, Warning, TEXT("Unexpected state %s after successful move"), *GetStateName(CurrentState));
            AbortCurrentTask();
            break;
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Move failed in state %s, retrying..."), *GetStateName(CurrentState));
        RetryMove();
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

        //UE_LOGLogTemp, Display, TEXT("RestockerAI: Shelf fully stocked, starting over"));
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
            //UE_LOGLogTemp, Display, TEXT("RestockerAI: Found new box with same product: %s"), *ProductBox->GetName());
            SetState(ERestockerState::MovingToProductBox);
            MoveToTarget(TargetProductBox);
            return;
        }
    }

    //UE_LOGLogTemp, Display, TEXT("RestockerAI: No more boxes with the same product, moving on to next task"));
    TargetShelf = nullptr;
    SetState(ERestockerState::Idle);
    StartRestocking();
}

void ARestockerAI::HandleEmptyBox()
{
    //UE_LOGLogTemp, Display, TEXT("RestockerAI: Product box is empty, disposing of it"));

    if (TargetProductBox)
    {
        ReleaseProductBox(TargetProductBox);
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
        //UE_LOGLogTemp, Display, TEXT("RestockerAI: Moving on to next task"));
        ReleaseShelf(TargetShelf);
        TargetShelf = nullptr;
        SetState(ERestockerState::Idle);
        StartRestocking();
    }
}

void ARestockerAI::CheckRestockProgress()
{
    if (TargetShelf && TargetProductBox)
    {
        if (IsShelfSufficientlyStocked(TargetShelf) || TargetProductBox->GetProductCount() == 0)
        {
            GetWorld()->GetTimerManager().ClearTimer(RestockTimerHandle);

            if (TargetProductBox->GetProductCount() == 0)
            {
                // Box is empty, delete it and continue restocking
                DeleteEmptyBox();
                SetState(ERestockerState::Idle);
                StartRestocking();
            }
            else
            {
                // Shelf is fully stocked but box still has products, find next shelf
                ReleaseShelf(TargetShelf);
                TargetShelf = nullptr;
                SetState(ERestockerState::Idle);
                StartRestocking();
            }
        }
        else
        {
            // Continue restocking the current shelf
            TargetShelf->StartStockingShelf(TargetProductBox->GetProductClass());
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
        UE_LOG(LogTemp, Display, TEXT("RestockerAI: Dropping current box"));
        UnlockProductBox();
        ReleaseProductBox(TargetProductBox);
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
        if (Shelf && !IsShelfSufficientlyStocked(Shelf) && Shelf->GetCurrentProductClass() == CurrentProductClass && !IsShelfReserved(Shelf))
        {
            if (ReserveShelf(Shelf))
            {
                ReleaseShelf(TargetShelf);
                TargetShelf = Shelf;
                //UE_LOGLogTemp, Display, TEXT("RestockerAI: Found another shelf for current product: %s"), *Shelf->GetName());
                SetState(ERestockerState::MovingToShelf);
                MoveToTarget(TargetShelf);
                return;
            }
        }
    }
    // If no shelf needs the current product, drop the box and start over
    //UE_LOGLogTemp, Display, TEXT("RestockerAI: No more shelves need current product, dropping box and starting over"));
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



void ARestockerAI::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    StopPeriodicShelfCheck();
    ReleaseAllReservations();
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

void ARestockerAI::ReleaseTargetProductBox()
{
    FScopeLock Lock(&TargetedProductBoxesLock);
    TargetedProductBoxes.Remove(TargetProductBox);
    TargetProductBox = nullptr;
}

bool ARestockerAI::IsTargetingProductBox(AProductBox* ProductBox) const
{
    return TargetProductBox == ProductBox;
}

bool ARestockerAI::ReserveTargetProductBox(AProductBox* ProductBox)
{
    FScopeLock Lock(&TargetedProductBoxesLock);
    if (!TargetedProductBoxes.Contains(ProductBox))
    {
        TargetedProductBoxes.Add(ProductBox, this);
        return true;
    }
    return false;
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
    UE_LOG(LogTemp, Warning, TEXT("RestockerAI: Aborting current task"));

    // Clear all targets
    TargetProductBox = nullptr;
    TargetShelf = nullptr;
    TargetStorageRack = nullptr;

    // If holding a box, drop it
    if (bIsHoldingProductBox)
    {
        DropCurrentBox();
    }

    // Reset state and start over
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

AStorageRack* ARestockerAI::FindNearestStorageRack(TSubclassOf<AProduct> ProductType)
{
    TArray<AActor*> FoundStorageRacks;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AStorageRack::StaticClass(), FoundStorageRacks);

    AStorageRack* NearestRack = nullptr;
    float NearestDistance = MAX_FLT;

    for (AActor* Actor : FoundStorageRacks)
    {
        AStorageRack* StorageRack = Cast<AStorageRack>(Actor);
        if (StorageRack && StorageRack->ProductType == ProductType)
        {
            float Distance = FVector::Dist(GetActorLocation(), StorageRack->GetActorLocation());
            if (Distance < NearestDistance)
            {
                NearestRack = StorageRack;
                NearestDistance = Distance;
            }
        }
    }

    return NearestRack;
}

void ARestockerAI::FindStorageRack()
{
    TArray<AActor*> FoundStorageRacks;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AStorageRack::StaticClass(), FoundStorageRacks);

    for (AActor* Actor : FoundStorageRacks)
    {
        AStorageRack* StorageRack = Cast<AStorageRack>(Actor);
        if (StorageRack && StorageRack->GetAvailableSpace() > 0)
        {
            TargetStorageRack = StorageRack;
            SetState(ERestockerState::MovingToStorageRack);
            MoveToStorageRack();
            return;
        }
    }

    // If no storage rack needs replenishing, go back to idle state
    SetState(ERestockerState::Idle);
}

void ARestockerAI::MoveToStorageRack()
{
    if (TargetStorageRack)
    {
        // Calculate a position near the storage rack
        FVector Direction = FMath::VRand();
        Direction.Z = 0;
        Direction.Normalize();
        FVector TargetLocation = TargetStorageRack->GetActorLocation() + Direction * 150.0f;

        // Move to the calculated position
        AIController->MoveToLocation(TargetLocation, 50.0f, true, true, false, false, nullptr, true);
    }
    else
    {
        SetState(ERestockerState::Idle);
        StartRestocking();
    }
}

void ARestockerAI::CreateProductBoxFromStorageRack()
{
    if (TargetStorageRack && TargetStorageRack->CanCreateProductBox())
    {
        AProduct* ProductDefault = TargetStorageRack->ProductType.GetDefaultObject();
        if (!ProductDefault)
        {
            UE_LOG(LogTemp, Error, TEXT("RestockerAI: Failed to get default object for ProductType"));
            AbortCurrentTask();
            return;
        }

        FProductData ProductData = ProductDefault->GetProductData();
        int32 AvailableQuantity = TargetStorageRack->GetCurrentStock();
        int32 DesiredQuantity = FMath::Min(ProductData.MaxProductsInBox, AvailableQuantity);

        UE_LOG(LogTemp, Log, TEXT("RestockerAI: Attempting to create product box with %d products (Available: %d)"), DesiredQuantity, AvailableQuantity);

        TargetProductBox = TargetStorageRack->CreateProductBox(DesiredQuantity);
        if (TargetProductBox)
        {
            UE_LOG(LogTemp, Log, TEXT("RestockerAI: Successfully created product box with %d products"), TargetProductBox->GetProductCount());
            PickUpProductBox();
            SetState(ERestockerState::Idle);
            StartRestocking();  // This will find a shelf to restock
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("RestockerAI: Failed to create product box from storage rack"));
            AbortCurrentTask();
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("RestockerAI: Storage rack cannot create product box"));
        AbortCurrentTask();
    }
}

void ARestockerAI::ReplenishStorageRack()
{
    if (TargetStorageRack && TargetProductBox)
    {
        int32 RemainingProductss = TargetProductBox->GetProductCount();
        int32 AvailableSpace = TargetStorageRack->GetAvailableSpace();
        int32 ProductsToReplenish = FMath::Min(RemainingProductss, AvailableSpace);

        TargetStorageRack->ReplenishStock(ProductsToReplenish);

        UE_LOG(LogTemp, Display, TEXT("RestockerAI: Replenished storage rack with %d products"), ProductsToReplenish);

        // Remove the product box
        TargetProductBox->Destroy();
        TargetProductBox = nullptr;
        bIsHoldingProductBox = false;

        SetState(ERestockerState::Idle);
        StartRestocking();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("RestockerAI: Cannot replenish storage rack, missing target rack or product box"));
        AbortCurrentTask();
    }
}

void ARestockerAI::ReturnToStorageRack()
{
    if (TargetStorageRack)
    {
        SetState(ERestockerState::ReturningToStorageRack);
        MoveToTarget(TargetStorageRack);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("RestockerAI: No target storage rack to return to"));
        AbortCurrentTask();
    }
}

AStorageRack* ARestockerAI::FindAppropriateStorageRack(TSubclassOf<AProduct> ProductType)
{
    TArray<AActor*> FoundStorageRacks;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AStorageRack::StaticClass(), FoundStorageRacks);

    for (AActor* Actor : FoundStorageRacks)
    {
        AStorageRack* StorageRack = Cast<AStorageRack>(Actor);
        if (StorageRack && StorageRack->ProductType == ProductType && StorageRack->CurrentStock > 1)
        {
            return StorageRack;
        }
    }

    return nullptr;
}

void ARestockerAI::DeleteEmptyBox()
{
    if (TargetProductBox)
    {
        UE_LOG(LogTemp, Display, TEXT("RestockerAI: Deleting empty product box"));

        // Destroy all products associated with the box
        TArray<AProduct*> ProductsToDestroy = TargetProductBox->GetProducts();
        for (AProduct* Product : ProductsToDestroy)
        {
            if (Product)
            {
                Product->Destroy();
            }
        }

        UnlockProductBox();
        ReleaseProductBox(TargetProductBox);
        TargetProductBox->Destroy();
        TargetProductBox = nullptr;
        bIsHoldingProductBox = false;
    }
}

AProductBox* ARestockerAI::FindNearestProductBox()
{
    TArray<AActor*> FoundProductBoxes;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AProductBox::StaticClass(), FoundProductBoxes);

    AProductBox* NearestBox = nullptr;
    float NearestDistance = MAX_FLT;

    for (AActor* Actor : FoundProductBoxes)
    {
        AProductBox* ProductBox = Cast<AProductBox>(Actor);
        if (ProductBox && !ProductBox->GetAttachParentActor() && !IsBoxHeldByPlayer(ProductBox) && !IsProductBoxReserved(ProductBox))
        {
            float Distance = FVector::Dist(GetActorLocation(), ProductBox->GetActorLocation());
            if (Distance < NearestDistance)
            {
                NearestBox = ProductBox;
                NearestDistance = Distance;
            }
        }
    }

    return NearestBox;
}

void ARestockerAI::DropBox()
{
    if (bIsHoldingProductBox && TargetProductBox)
    {
        UE_LOG(LogTemp, Display, TEXT("RestockerAI: Dropping box"));
        UnlockProductBox();
        ReleaseProductBox(TargetProductBox);
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

    SetState(ERestockerState::Idle);
    StartRestocking();
}

bool ARestockerAI::ShouldReplenishStorageRack()
{
    if (!TargetProductBox)
    {
        return false;
    }

    AStorageRack* MatchingRack = FindMatchingStorageRack(TargetProductBox->GetProductClass());
    return MatchingRack && !IsStorageRackFull(MatchingRack);
}

bool ARestockerAI::IsStorageRackFull(AStorageRack* StorageRack)
{
    return StorageRack->GetAvailableSpace() == 0;
}


AStorageRack* ARestockerAI::FindMatchingStorageRack(TSubclassOf<AProduct> ProductType)
{
    TArray<AActor*> FoundStorageRacks;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AStorageRack::StaticClass(), FoundStorageRacks);

    for (AActor* Actor : FoundStorageRacks)
    {
        AStorageRack* StorageRack = Cast<AStorageRack>(Actor);
        if (StorageRack && StorageRack->ProductType == ProductType)
        {
            return StorageRack;
        }
    }

    return nullptr;
}

void ARestockerAI::RetryMoveImpl()
{
    switch (CurrentState)
    {
    case ERestockerState::MovingToProductBox:
        if (TargetProductBox)
        {
            MoveToTarget(TargetProductBox);
        }
        else
        {
            AbortCurrentTask();
        }
        break;

    case ERestockerState::MovingToShelf:
        if (TargetShelf)
        {
            MoveToTarget(TargetShelf);
        }
        else
        {
            AbortCurrentTask();
        }
        break;

    case ERestockerState::MovingToStorageRack:
        if (TargetStorageRack)
        {
            MoveToStorageRack();
        }
        else
        {
            AbortCurrentTask();
        }
        break;

    default:
        AbortCurrentTask();
        break;
    }
}

void ARestockerAI::RetryMove()
{
    // Wait a short time before retrying the move
    FTimerHandle RetryTimerHandles;
    GetWorldTimerManager().SetTimer(RetryTimerHandles, this, &ARestockerAI::RetryMoveImpl, 1.0f, false);
}