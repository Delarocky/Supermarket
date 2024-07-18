#include "AICustomerPawn.h"
#include "Product.h"
#include "Checkout.h"
#include "ShoppingBag.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "AI/NavigationSystemBase.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Shelf.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "TimerManager.h"

AAICustomerPawn::AAICustomerPawn()
{
    PrimaryActorTick.bCanEverTick = true;
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    ShoppingBag = CreateDefaultSubobject<UShoppingBag>(TEXT("ShoppingBag"));
    MaxItems = FMath::RandRange(2, 12);  // Random number between 2 and 12
    ShoppingTime = 300.0f; // 5 minutes
    CurrentItems = 0;
}

void AAICustomerPawn::BeginPlay()
{
    Super::BeginPlay();
    //UE_LOGLogTemp, Display, TEXT("AI BeginPlay called"));
    InitializeAIController();
}

FVector AAICustomerPawn::FindMostAccessiblePoint(const TArray<FVector>& Points)
{
    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!NavSys)
    {
        //UE_LOGLogTemp, Error, TEXT("Navigation system not found in FindMostAccessiblePoint"));
        return Points[0]; // Return the first point if nav system is not available
    }

    FVector AILocation = GetActorLocation();
    FVector BestPoint = Points[0];
    float BestDistance = TNumericLimits<float>::Max();

    for (const FVector& Point : Points)
    {
        FNavLocation NavLocation;
        if (NavSys->ProjectPointToNavigation(Point, NavLocation, FVector(100, 100, 100)))
        {
            float Distance = FVector::Dist(AILocation, NavLocation.Location);
            if (Distance < BestDistance)
            {
                BestDistance = Distance;
                BestPoint = NavLocation.Location;
            }
        }
    }

    return BestPoint;
}

void AAICustomerPawn::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (!AIController)
    {
        InitializeAIController();
    }

    if (bIsRotating)
    {
        ElapsedTime += DeltaTime;
        float Alpha = ElapsedTime / RotationTime;

        FRotator CurrentRotation = GetActorRotation();
        CurrentRotation.Pitch = 0.0f;
        CurrentRotation.Roll = 0.0f;

        FRotator NewRotation = FMath::Lerp(CurrentRotation, TargetRotation, Alpha);

        SetActorRotation(NewRotation);

        if (Alpha >= 1.0f)
        {
            bIsRotating = false;
            SetActorRotation(TargetRotation);
            TryPickUpProduct();
        }
    }
}


void AAICustomerPawn::SetCurrentShelf(AShelf* Shelf)
{
    CurrentShelf = Shelf;
}


void AAICustomerPawn::InitializeAIController()
{
    if (!AIController)
    {
        AIController = Cast<AAIController>(GetController());
        if (AIController)
        {
            //UE_LOGLogTemp, Display, TEXT("AIController set successfully"));
        }
        else
        {
            //UE_LOGLogTemp, Error, TEXT("Failed to set AIController"));
        }
    }
}

void AAICustomerPawn::StartShopping()
{
    CurrentItems = 0;
    ChooseProduct();
    GetWorldTimerManager().SetTimer(ShoppingTimerHandle, this, &AAICustomerPawn::FinishShopping, ShoppingTime, false);
}

void AAICustomerPawn::FinishShopping()
{
    GetWorldTimerManager().ClearTimer(ShoppingTimerHandle);
    GoToCheckoutWhenDone();
}

void AAICustomerPawn::ChooseProduct()
{
   //UE_LOGLogTemp, Display, TEXT("ChooseProduct called. Current Items: %d, Max Items: %d"), CurrentItems, MaxItems);
    ResetAccessPointAttempts();
    if (CurrentItems >= MaxItems)
    {
        //UE_LOGLogTemp, Display, TEXT("Max items reached, going to checkout"));
        GoToCheckoutWhenDone();
        return;
    }

    if (!AIController)
    {
        InitializeAIController();
    }

    if (!AIController)
    {
        //UE_LOGLogTemp, Error, TEXT("AIController is null in ChooseProduct!"));
        return;
    }

    // Find a random stocked shelf
    AShelf* TargetShelf = FindRandomStockedShelf();
    if (TargetShelf)
    {
        CurrentShelf = TargetShelf;
        TArray<FVector> AccessPoints = TargetShelf->GetAllAccessPointLocations();
        FVector TargetLocation = FindMostAccessiblePoint(AccessPoints);

        // Move to the chosen access point
        UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
        if (NavSys)
        {
            FNavLocation NavLocation;
            if (NavSys->ProjectPointToNavigation(TargetLocation, NavLocation, FVector(100, 100, 100)))
            {
                AIController->MoveToLocation(NavLocation.Location, 1.0f, true, true, false, false, nullptr, true);

                // Set up a timer to check if we've reached the shelf's access point
                GetWorldTimerManager().SetTimer(CheckReachedShelfTimerHandle, this, &AAICustomerPawn::CheckReachedShelf, 0.1f, true);
            }
            else
            {
                //UE_LOGLogTemp, Error, TEXT("Failed to find valid navigation point for shelf access point. Choosing new product."));
                CurrentShelf = nullptr;
                GetWorldTimerManager().SetTimer(RetryTimerHandle, this, &AAICustomerPawn::ChooseProduct, 1.0f, false);
            }
        }
        else
        {
            //UE_LOGLogTemp, Error, TEXT("Navigation system not found. Choosing new product."));
            CurrentShelf = nullptr;
            GetWorldTimerManager().SetTimer(RetryTimerHandle, this, &AAICustomerPawn::ChooseProduct, 1.0f, false);
        }
    }
    else
    {
        //UE_LOGLogTemp, Warning, TEXT("No accessible stocked shelves found. Waiting..."));
        GetWorldTimerManager().SetTimer(RetryTimerHandle, this, &AAICustomerPawn::ChooseProduct, 2.0f, false);
    }
}


void AAICustomerPawn::PutCurrentProductInBag()
{
    if (CurrentTargetProduct && ShoppingBag)
    {
        //UE_LOGLogTemp, Display, TEXT("Putting product in bag: %s"), *CurrentTargetProduct->GetProductName());
 

        // Add the product to the shopping bag
        ShoppingBag->AddProduct(CurrentTargetProduct);

        // Hide the product and disable its collision
        CurrentTargetProduct->SetActorHiddenInGame(true);
        CurrentTargetProduct->SetActorEnableCollision(false);

        CurrentItems++;
        //UE_LOGLogTemp, Display, TEXT("Product added to bag. Current Items: %d"), CurrentItems);

        // Debug print the contents of the shopping bag
        ShoppingBag->DebugPrintContents();

        // Reset CurrentTargetProduct
        CurrentTargetProduct = nullptr;

        // Check if we've reached MaxItems
        if (CurrentItems >= MaxItems)
        {
            //UE_LOGLogTemp, Display, TEXT("Max items reached, going to checkout"));
            GoToCheckoutWhenDone();
        }
        else
        {
            // If we haven't reached max items, choose the next product after a short delay
            GetWorldTimerManager().SetTimer(ChooseProductTimerHandle, this, &AAICustomerPawn::ChooseProduct, 0.1f, false);
        }
    }
    else
    {
        //UE_LOGLogTemp, Error, TEXT("Failed to put product in bag. Product: %s, ShoppingBag: %s"), CurrentTargetProduct ? TEXT("Valid") : TEXT("Invalid"), ShoppingBag ? TEXT("Valid") : TEXT("Invalid"));
        // If we failed to put the product in the bag, try to choose another product
        ChooseProduct();
    }
}

void AAICustomerPawn::DetachAllItems()
{
    if (ShoppingBag)
    {
        TArray<AProduct*> Products = ShoppingBag->GetProducts();
        for (AProduct* Product : Products)
        {
            if (Product)
            {
                // Detach the product from any component it might be attached to
                Product->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

                // Hide the product
                Product->SetActorHiddenInGame(true);
                Product->SetActorEnableCollision(false);

                //UE_LOGLogTemp, Display, TEXT("Detached and hidden product: %s"), *Product->GetProductName());
            }
        }
    }
    else
    {
        //UE_LOGLogTemp, Warning, TEXT("Shopping bag is null when trying to detach items"));
    }

    // Clear the current target product
    if (CurrentTargetProduct)
    {
        CurrentTargetProduct->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        CurrentTargetProduct->SetActorHiddenInGame(true);
        CurrentTargetProduct->SetActorEnableCollision(false);
        //UE_LOGLogTemp, Display, TEXT("Detached and hidden current target product: %s"), *CurrentTargetProduct->GetProductName());
        CurrentTargetProduct = nullptr;
    }

    // Lower the arm if it's raised
   // bRaiseArm = false;
}

void AAICustomerPawn::PickUpProduct()
{
    // Clear the timeout timer as we've started picking up the product
    GetWorldTimerManager().ClearTimer(PickUpTimeoutHandle);

    AProduct* PickedProduct = CurrentShelf->RemoveNextProduct();
    if (PickedProduct)
    {
        //UE_LOGLogTemp, Display, TEXT("Picked up product %s from shelf %s. Total products: %d/%d"),*PickedProduct->GetProductName(), *CurrentShelf->GetName(), CurrentItems + 1, MaxItems);

        // Store the product's current transform
        FTransform OriginalTransform = PickedProduct->GetActorTransform();

        // Detach the product from any parent
        PickedProduct->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

        // Set the product's transform to maintain its position, rotation, and scale
        PickedProduct->SetActorTransform(OriginalTransform);

        // Ensure the product is visible and has collision enabled
        PickedProduct->SetActorHiddenInGame(false);
        PickedProduct->SetActorEnableCollision(true);

        // If the product has a mesh component, make sure it's visible
        UStaticMeshComponent* ProductMesh = PickedProduct->FindComponentByClass<UStaticMeshComponent>();
        if (ProductMesh)
        {
            ProductMesh->SetVisibility(true);
        }

        // Set the CurrentTargetProduct
        CurrentTargetProduct = PickedProduct;

        //UE_LOGLogTemp, Display, TEXT("Starting product interpolation for %s"), *PickedProduct->GetProductName());
        
       
        StartProductInterpolation();
    }
    else
    {
        //UE_LOGLogTemp, Warning, TEXT("Failed to pick up product from shelf %s"), *CurrentShelf->GetName());
        LowerArm();
        ChooseProduct();
    }
}

void AAICustomerPawn::DetermineShelfPosition()
{
    if (!CurrentShelf)
    {
        //UE_LOGLogTemp, Warning, TEXT("No current shelf set"));
        return;
    }

    // Reset all animation flags
    ResetGrabAnimationFlags();

    FVector AILocation = GetActorLocation();
    FVector AIEyeLocation = AILocation + FVector(0, 0, GetDefaultHalfHeight()); // Assume eye level is at the top of the capsule

    // Get the product's location instead of the shelf's location
    FVector ProductLocation;
    ProductLocation = CurrentShelf->GetNextProductLocation();
    if (ProductLocation != FVector::ZeroVector)
    {
        // Calculate the height difference between the AI's eye level and the product
        float HeightDifference = ProductLocation.Z - AIEyeLocation.Z;

        // Get the AI's height
        float AIHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 2.0f;

        // Define thresholds for different animations
        float LowThreshold = -AIHeight * 0.4f;  // 30% of AI's height below eye level
        float HighThreshold = AIHeight * 0.01f;  // 30% of AI's height above eye level

        if (HeightDifference < LowThreshold)
        {
            bKneelDown = true;
            //UE_LOGLogTemp, Display, TEXT("Product is low, kneeling down. Height difference: %f"), HeightDifference);
        }
        else if (HeightDifference > HighThreshold)
        {
            bReachUp = true;
            //UE_LOGLogTemp, Display, TEXT("Product is high, reaching up. Height difference: %f"), HeightDifference);
        }
        else
        {
            bRaiseArm = true;
            //UE_LOGLogTemp, Display, TEXT("Product is at waist level, normal grab. Height difference: %f"), HeightDifference);
        }
    }
    else
    {
        //UE_LOGLogTemp, Warning, TEXT("Unable to get next product location from shelf"));
        bRaiseArm = true; // Default to normal grab if we can't determine product location
    }
}

void AAICustomerPawn::ResetGrabAnimationFlags()
{
    bKneelDown = false;
    bReachUp = false;
    bRaiseArm = false;
}


void AAICustomerPawn::StartProductInterpolation()
{
    if (CurrentTargetProduct)
    {
        // Get the location of the hand socket
        FName RightHandSocketName = FName("middle_03_rSocket");
        FVector TargetLocation = GetMesh()->GetSocketLocation(RightHandSocketName);

        // Start the interpolation timer
        GetWorldTimerManager().SetTimer(ProductInterpolationTimerHandle, this, &AAICustomerPawn::InterpolateProduct, 0.01f, true);
    }
    else
    {
        //UE_LOGLogTemp, Warning, TEXT("No product to interpolate"));
        
    }
}

void AAICustomerPawn::InterpolateProduct()
{
    if (CurrentTargetProduct)
    {
        FName RightHandSocketName = FName("middle_03_r");
        FVector TargetLocation = GetMesh()->GetSocketLocation(RightHandSocketName);
        FRotator SocketRotation = GetMesh()->GetSocketRotation(RightHandSocketName);

        FVector NewLocation = FMath::VInterpTo(CurrentTargetProduct->GetActorLocation(), TargetLocation, GetWorld()->GetDeltaSeconds(), 16.0f);
        FRotator NewRotation = FMath::RInterpTo(CurrentTargetProduct->GetActorRotation(), SocketRotation, GetWorld()->GetDeltaSeconds(), 16.0f);

        CurrentTargetProduct->SetActorLocationAndRotation(NewLocation, NewRotation);

        // Check if the product is close enough to the target location
        if (FVector::Dist(NewLocation, TargetLocation) < 1.0f)
        {
            GetWorldTimerManager().ClearTimer(ProductInterpolationTimerHandle);

            // Attach the product to the hand socket
            CurrentTargetProduct->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, RightHandSocketName);

            // Lower the arm after 1 second
            GetWorldTimerManager().SetTimer(LowerArmTimerHandle, this, &AAICustomerPawn::LowerArm, 0.01f, false);

        }
    }
    else
    {
        GetWorldTimerManager().ClearTimer(ProductInterpolationTimerHandle);
        ResetGrabAnimationFlags();
        LowerArm();
    }
}

void AAICustomerPawn::LowerArm()
{
    ResetGrabAnimationFlags();
    // Wait for 1 second, then put the product in the bag
    GetWorldTimerManager().SetTimer(PutInBagTimerHandle, this, &AAICustomerPawn::PutCurrentProductInBag, 0.55f, false);
}

void AAICustomerPawn::PutProductInBag(AProduct* Product)
{
    if (Product && ShoppingBag)
    {
        //UE_LOGLogTemp, Display, TEXT("Putting product in bag: %s"), *Product->GetProductName());


        // Remove the product from the shelf
        if (CurrentShelf)
        {
            AProduct* RemovedProduct = CurrentShelf->RemoveNextProduct();
            if (RemovedProduct != Product)
            {
                //UE_LOGLogTemp, Warning, TEXT("Removed product does not match the expected product"));
            }
        }
        else
        {
            //UE_LOGLogTemp, Warning, TEXT("CurrentShelf is null when trying to remove product"));
        }

        Product->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        ShoppingBag->AddProduct(Product);

        // Do not destroy the product, just hide it
        Product->SetActorHiddenInGame(true);
        Product->SetActorEnableCollision(false);

        CurrentItems++;
        //UE_LOGLogTemp, Display, TEXT("Product added to bag. Current Items: %d"), CurrentItems);

        // Debug print the contents of the shopping bag
        ShoppingBag->DebugPrintContents();

        // Check if we've reached MaxItems
        if (CurrentItems >= MaxItems)
        {
            //UE_LOGLogTemp, Display, TEXT("Max items reached, going to checkout"));
            GoToCheckoutWhenDone();
        }
        else
        {
            // If we haven't reached max items, choose the next product after a short delay
            GetWorldTimerManager().SetTimer(ChooseProductTimerHandle, this, &AAICustomerPawn::ChooseProduct, 0.1f, false);
        }
    }
    else
    {
        //UE_LOGLogTemp, Error, TEXT("Failed to put product in bag. Product: %s, ShoppingBag: %s"), Product ? TEXT("Valid") : TEXT("Invalid"), ShoppingBag ? TEXT("Valid") : TEXT("Invalid"));
        // If we failed to put the product in the bag, try to choose another product
        ChooseProduct();
    }
}

void AAICustomerPawn::GoToCheckoutWhenDone()
{
    //UE_LOGLogTemp, Display, TEXT("AI is heading to checkout. Detaching all items."));

    // Detach all items from the character
    DetachAllItems();

    RetryCount = 0;
    RetryEnterCheckoutQueue();
}

void AAICustomerPawn::RetryEnterCheckoutQueue()
{
    TArray<AActor*> FoundCheckouts;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACheckout::StaticClass(), FoundCheckouts);

    if (FoundCheckouts.Num() > 0)
    {
        // Choose a random checkout
        int32 RandomIndex = FMath::RandRange(0, FoundCheckouts.Num() - 1);
        ACheckout* ChosenCheckout = Cast<ACheckout>(FoundCheckouts[RandomIndex]);

        if (ChosenCheckout && ChosenCheckout->TryEnterQueue(this))
        {
            CurrentCheckout = ChosenCheckout;
            //UE_LOGLogTemp, Display, TEXT("AI entered checkout queue after %d attempts"), RetryCount + 1);
        }
        else
        {
            RetryCount++;
            //UE_LOGLogTemp, Warning, TEXT("AI couldn't enter checkout queue, retrying in %.1f seconds. Attempt %d"),RetryDelay, RetryCount + 1);
            GetWorldTimerManager().SetTimer(RetryTimerHandle, this, &AAICustomerPawn::RetryEnterCheckoutQueue, RetryDelay, false);
        }
    }
    else
    {
        //UE_LOGLogTemp, Warning, TEXT("No checkouts found in the level. Retrying in %.1f seconds."), RetryDelay);
        GetWorldTimerManager().SetTimer(RetryTimerHandle, this, &AAICustomerPawn::RetryEnterCheckoutQueue, RetryDelay, false);
    }
}

void AAICustomerPawn::DebugShoppingState()
{
    //UE_LOGLogTemp, Display, TEXT("AI Shopping State - Current Items: %d, Max Items: %d"), CurrentItems, MaxItems);
}

void AAICustomerPawn::MoveTo(const FVector& Location)
{
    if (AIController)
    {
        // Use MoveToLocation with a small acceptance radius for precise movement
        AIController->MoveToLocation(Location, 1.0f, false, true, false, false, nullptr, true);
    }
    else
    {
        //UE_LOGLogTemp, Warning, TEXT("AIController is null in AAICustomerPawn::MoveTo"));
        InitializeAIController();
        if (AIController)
        {
            AIController->MoveToLocation(Location, 1.0f, false, true, false, false, nullptr, true);
        }
    }
}

void AAICustomerPawn::LeaveCheckout()
{
    if (CurrentCheckout)
    {
        CurrentCheckout->CustomerLeft(this);
        CurrentCheckout = nullptr;
        //UE_LOGLogTemp, Display, TEXT("AI left checkout"));
    }

    // Reset shopping state
    CurrentItems = 0;
    ShoppingBag->EmptyBag();
    UE_LOG(LogTemp, Display, TEXT("calling leave store"));
    // Leave the store
    LeaveStore();
}


void AAICustomerPawn::GoToCheckout()
{
    InitializeAIController();
    if (!AIController)
    {
        //UE_LOGLogTemp, Error, TEXT("AIController is null in GoToCheckout!"));
        return;
    }

    TArray<AActor*> FoundCheckouts;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACheckout::StaticClass(), FoundCheckouts);
    if (FoundCheckouts.Num() > 0)
    {
        // Choose a random checkout
        int32 RandomIndex = FMath::RandRange(0, FoundCheckouts.Num() - 1);
        ACheckout* AvailableCheckout = Cast<ACheckout>(FoundCheckouts[RandomIndex]);
        if (AvailableCheckout)
        {
            UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
            if (NavSys)
            {
                FNavLocation AINavLocation;
                FNavLocation CheckoutNavLocation;
                bool bAIOnNavMesh = NavSys->ProjectPointToNavigation(GetActorLocation(), AINavLocation);
                bool bCheckoutOnNavMesh = NavSys->ProjectPointToNavigation(AvailableCheckout->GetActorLocation(), CheckoutNavLocation);

                if (bAIOnNavMesh && bCheckoutOnNavMesh)
                {
                    //UE_LOGLogTemp, Display, TEXT("Attempting to move to checkout"));
                    UAIBlueprintHelperLibrary::SimpleMoveToLocation(AIController, CheckoutNavLocation.Location);
                }
                else
                {
                    //UE_LOGLogTemp, Error, TEXT("AI or Checkout not on NavMesh"));
                }
            }
            else
            {
                //UE_LOGLogTemp, Error, TEXT("Navigation system is not available"));
            }
        }
    }
    else
    {
        //UE_LOGLogTemp, Warning, TEXT("No checkouts found in the level"));
    }
}

void AAICustomerPawn::CheckShoppingComplete()
{
    if (CurrentItems >= MaxItems)
    {
        GoToCheckoutWhenDone();
        return;
    }

    TArray<AActor*> FoundProducts;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AProduct::StaticClass(), FoundProducts);

    bool bAnyValidProductsLeft = false;
    for (AActor* Product : FoundProducts)
    {
        AProduct* CastedProduct = Cast<AProduct>(Product);
        if (CastedProduct && !CastedProduct->IsHidden() && CastedProduct->GetActorEnableCollision())
        {
            bAnyValidProductsLeft = true;
            break;
        }
    }

    if (!bAnyValidProductsLeft)
    {
        //UE_LOGLogTemp, Display, TEXT("No more products to pick up. Going to checkout."));
        GoToCheckoutWhenDone();
    }
    else
    {
        // If there are still products and we haven't reached MaxItems, choose another product
        ChooseProduct();
    }
}

void AAICustomerPawn::CheckReachedShelf()
{
    AController* ControllerInstance = GetController();
    AAIController* AIControllerInstance = Cast<AAIController>(ControllerInstance);
    if (!AIControllerInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("AIController is null in CheckReachedShelf"));
        GetWorldTimerManager().ClearTimer(CheckReachedShelfTimerHandle);
        ChooseProduct();
        return;
    }

    if (AIControllerInstance->GetMoveStatus() == EPathFollowingStatus::Idle)
    {
        GetWorldTimerManager().ClearTimer(CheckReachedShelfTimerHandle);
        UE_LOG(LogTemp, Display, TEXT("AI reached shelf. Turning to face shelf."));
        TurnToFaceShelf();
    }
    else if (GetWorldTimerManager().GetTimerElapsed(CheckReachedShelfTimerHandle) > 15.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to reach shelf after 15 seconds, choosing a new one"));
        GetWorldTimerManager().ClearTimer(CheckReachedShelfTimerHandle);
        CurrentShelf = nullptr;
        ChooseProduct();
    }
}

void AAICustomerPawn::TurnToFaceShelf()
{
    if (!CurrentShelf)
    {
        UE_LOG(LogTemp, Warning, TEXT("AI %s: Cannot turn to face shelf, CurrentShelf is null"), *GetName());
        ChooseProduct();
        return;
    }

    FVector AILocation = GetActorLocation();
    FVector ShelfLocation = CurrentShelf->GetActorLocation();
    TargetRotation = UKismetMathLibrary::FindLookAtRotation(AILocation, ShelfLocation);

    // Only consider yaw rotation to avoid twitching on X and Y axes
    TargetRotation.Pitch = 0.0f;
    TargetRotation.Roll = 0.0f;

    // Calculate the shortest rotation
    FRotator CurrentRotation = GetActorRotation();
    CurrentRotation.Pitch = 0.0f;
    CurrentRotation.Roll = 0.0f;

    DeltaRotation = UKismetMathLibrary::NormalizedDeltaRotator(TargetRotation, CurrentRotation);

    // Check if the AI is already facing the shelf within 10 degrees
    if (FMath::Abs(DeltaRotation.Yaw) <= 10.0f)
    {
        UE_LOG(LogTemp, Display, TEXT("AI %s: Already facing the shelf within 10 degrees"), *GetName());
        TryPickUpProduct();
        return;
    }

    // Calculate the time needed for rotation (assuming 180 degrees per second)
    RotationTime = FMath::Abs(DeltaRotation.Yaw) / 180.0f;
    ElapsedTime = 0.0f;

    bIsRotating = true;
}


void AAICustomerPawn::TryPickUpProduct()
{
    if (!CurrentShelf || CurrentShelf->GetProductCount() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("No current shelf or shelf is empty"));
        CurrentShelf = nullptr;
        ResetAccessPointAttempts();
        ChooseProduct();
        return;
    }

    TArray<FVector> AccessPoints = CurrentShelf->GetAllAccessPointLocations();
    FVector AILocation = GetActorLocation();
    float MinDistance = 100.0f;

    bool bIsCloseEnough = false;
    for (const FVector& AccessPoint : AccessPoints)
    {
        if (FVector::Dist(AILocation, AccessPoint) <= MinDistance)
        {
            bIsCloseEnough = true;
            break;
        }
    }

    if (bIsCloseEnough)
    {
        DetermineShelfPosition();
        GetWorldTimerManager().SetTimer(RetryPickUpTimerHandle, this, &AAICustomerPawn::PickUpProduct, 0.5f, false);
    }
    else
    {
        AccessPointAttempts++;
        if (AccessPointAttempts >= MaxAccessPointAttempts)
        {
            UE_LOG(LogTemp, Warning, TEXT("Max attempts to reach access point reached. Choosing new product."));
            ResetAccessPointAttempts();
            CurrentShelf = nullptr;
            ChooseProduct();
            return;
        }

        UE_LOG(LogTemp, Warning, TEXT("AI is not close enough to an access point. Attempting to navigate. Attempt %d of %d"), AccessPointAttempts, MaxAccessPointAttempts);
        FVector NearestAccessPoint = FindMostAccessiblePoint(AccessPoints);

        // Use AIController to move to the access point
        if (AIController)
        {
            AIController->MoveToLocation(NearestAccessPoint, 1.0f, true, true, true, false, nullptr, true);
            GetWorldTimerManager().SetTimer(RetryPickUpTimerHandle, this, &AAICustomerPawn::CheckReachedAccessPoint, 0.5f, true);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("AIController is null. Cannot navigate to access point."));
            ResetAccessPointAttempts();
            ChooseProduct();
        }
    }
}


AShelf* AAICustomerPawn::FindRandomStockedShelf()
{
    TArray<AActor*> FoundShelves;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AShelf::StaticClass(), FoundShelves);
    TArray<AShelf*> AccessibleStockedShelves;

    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!NavSys)
    {
        //UE_LOGLogTemp, Error, TEXT("Navigation system not found"));
        return nullptr;
    }

    for (AActor* Actor : FoundShelves)
    {
        AShelf* Shelf = Cast<AShelf>(Actor);
        if (IsValid(Shelf) && Shelf->GetProductCount() > 0 && Shelf->GetCurrentProductClass() != nullptr)
        {
            TArray<FVector> AccessPoints = Shelf->GetAllAccessPointLocations();
            bool bIsAccessible = false;

            for (const FVector& AccessPoint : AccessPoints)
            {
                FNavLocation NavLocation;
                if (NavSys->ProjectPointToNavigation(AccessPoint, NavLocation, FVector(100, 100, 100)))
                {
                    bIsAccessible = true;
                    break;
                }
            }

            if (bIsAccessible)
            {
                AccessibleStockedShelves.Add(Shelf);
            }
        }
    }

    if (AccessibleStockedShelves.Num() > 0)
    {
        int32 RandomIndex = FMath::RandRange(0, AccessibleStockedShelves.Num() - 1);
        return AccessibleStockedShelves[RandomIndex];
    }

    return nullptr;
}

FVector AAICustomerPawn::GetRandomLocationInStore()
{
    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (NavSys)
    {
        FNavLocation RandomNavLocation;
        if (NavSys->GetRandomReachablePointInRadius(GetActorLocation(), 5000.0f, RandomNavLocation))
        {
            return RandomNavLocation.Location;
        }
    }

    // Fallback: return a random point 5000 units away in a random direction
    FVector RandomDirection = FMath::VRand();
    return GetActorLocation() + RandomDirection * 5000.0f;
}

void AAICustomerPawn::LeaveStore()
{
    UE_LOG(LogTemp, Display, TEXT("leave store called"));

    if (AIController)
    {
        UAIBlueprintHelperLibrary::SimpleMoveToLocation(AIController, InitialSpawnLocation);

        // Set a timer to check if the AI has reached the parking space
        GetWorldTimerManager().SetTimer(LeaveStoreTimerHandle, this, &AAICustomerPawn::CheckReachedParkingSpace, 0.5f, true);
    }
    else
    {
        //UE_LOGLogTemp, Error, TEXT("AIController is null in LeaveStore!"));
        NotifyParkingSpaceAndDestroy();
    }
}

void AAICustomerPawn::DestroyAI()
{
    //UE_LOGLogTemp, Display, TEXT("AI has left the store and is being destroyed"));
    Destroy();
}

void AAICustomerPawn::CheckReachedAccessPoint()
{
    if (!CurrentShelf)
    {
        GetWorldTimerManager().ClearTimer(RetryPickUpTimerHandle);
        ResetAccessPointAttempts();
        ChooseProduct();
        return;
    }

    AController* ControllerInstance = GetController();
    AAIController* AIControllerInstance = Cast<AAIController>(ControllerInstance);
    if (!AIControllerInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("AIController is null in CheckReachedAccessPoint"));
        GetWorldTimerManager().ClearTimer(RetryPickUpTimerHandle);
        ResetAccessPointAttempts();
        ChooseProduct();
        return;
    }

    if (AIControllerInstance->GetMoveStatus() == EPathFollowingStatus::Idle)
    {
        // We've either reached the destination or failed to move
        TArray<FVector> AccessPoints = CurrentShelf->GetAllAccessPointLocations();
        FVector AILocation = GetActorLocation();
        float MinDistance = 260.0f;

        bool bReachedAccessPoint = false;
        for (const FVector& AccessPoint : AccessPoints)
        {
            if (FVector::Dist(AILocation, AccessPoint) <= MinDistance)
            {
                bReachedAccessPoint = true;
                break;
            }
        }

        if (bReachedAccessPoint)
        {
            GetWorldTimerManager().ClearTimer(RetryPickUpTimerHandle);
            ResetAccessPointAttempts();
            TurnToFaceShelf();
        }
        else
        {
            // If we're here, we didn't reach an access point
            TryPickUpProduct(); // This will increment AccessPointAttempts and potentially choose a new product
        }
    }
    // If still moving, continue waiting
}


void AAICustomerPawn::CheckReachedParkingSpace()
{
    if (FVector::Dist(GetActorLocation(), InitialSpawnLocation) < 200.0f) // Adjust this distance as needed
    {
        GetWorldTimerManager().ClearTimer(LeaveStoreTimerHandle);
        NotifyParkingSpaceAndDestroy();
    }
}

void AAICustomerPawn::NotifyParkingSpaceAndDestroy()
{
    if (AssignedParkingSpace)
    {
        AssignedParkingSpace->CustomerReturned(this);
    }
    Destroy();
}

