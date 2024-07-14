// Copyright Epic Games, Inc. All Rights Reserved.

#include "SupermarketCharacter.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "ProductBox.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Shelf.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "BuildModeWidget.h"
#include "Components/BoxComponent.h"
#include "Checkout.h"
#include "Components/PrimitiveComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Camera/CameraComponent.h"
#include "StoreStatusWidget.h"
#include "BuildModeCameraActor.h"
#include "Engine/StaticMeshActor.h"
#include "DrawDebugHelpers.h"
#include "HAL/CriticalSection.h"
#include "Kismet/GameplayStatics.h"
#include "SceneBoxComponent.h"
#include "Materials/MaterialInterface.h"
#include "GameFramework/CharacterMovementComponent.h"

FVector IntersectRayWithPlane(const FVector& RayOrigin, const FVector& RayDirection, const FPlane& Plane)
{
    float t = (Plane.W - FVector::DotProduct(Plane.GetNormal(), RayOrigin)) / FVector::DotProduct(Plane.GetNormal(), RayDirection);
    return RayOrigin + RayDirection * t;
}

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// ASupermarketCharacter

ASupermarketCharacter::ASupermarketCharacter()
{
    // Set size for collision capsule
    GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

    // Create a CameraComponent    
    FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
    FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the camera
    FirstPersonCameraComponent->bUsePawnControlRotation = true;

    TabletCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TabletCamera"));
    TabletCameraComponent->SetupAttachment(GetCapsuleComponent());
    TabletCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the camera
    TabletCameraComponent->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f)); // Look down at 60 degree angle
    TabletCameraComponent->bUsePawnControlRotation = false;
    TabletCameraComponent->SetActive(false); // Start with this camera inactive

    ProductBoxOffset = FVector(90.0f, 0.0f, -60.0f);
    ProductBoxRotation = FRotator(0.0f, 90.0f, 0.0f);

    HeldProductBox = nullptr;
    CurrentTargetShelf = nullptr;
    bIsInteracting = false;


    // Create tablet mesh
    TabletMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TabletMesh"));
    //TabletMesh->SetupAttachment(GetMesh(), "hand_r"); // Attach to right hand
    TabletMesh->SetVisibility(false);

    // Set default values for tablet transform
    TabletOffset = FVector(-10.0f, 6.0f, 0.0f);
    TabletRotation = FRotator(270.0f, 80.0f, 0.0f);
    TabletScreenOffset = FVector(0, 0, 5);
    TabletScreenRotation = FRotator(90.0f, 0.0f, 0.0f);

    TabletScreenWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("TabletScreenWidget"));
    TabletScreenWidget->SetupAttachment(TabletMesh);
    TabletScreenWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 5.0f)); // Adjust as needed
    TabletScreenWidget->SetRelativeRotation(FRotator(0.0f, 0.0f, 180.0f)); // Face the widget towards the player
    TabletScreenWidget->SetDrawSize(FVector2D(150.0f, 230.0f)); // Set the size of the widget
    TabletScreenWidget->SetVisibility(false); // Start with the widget hidden

    bIsMovingObject = false;
    MoveObjectHoldTime = 2.0f;
    MaxMoveDistance = 500.0f;
    RotationAngle = 45.0f;

    // Initialize tablet mode variables
    bIsTabletMode = false;
    TabletCameraRotation = FRotator(-60.f, 0.f, 0.f); // Look down at 60 degree angle
    TabletCameraFOV = 50.f;
    bCameraRotationEnabled = true;
   ////UE_LOGLogTemp, Display, TEXT("Initial TabletCameraRotation set to: %s"), *TabletCameraRotation.ToString());

    // Initialize camera transition properties
    CameraTransitionTime = 0.5f;
    CameraTransitionDuration = 0.0f;
    CameraTransitionElapsedTime = 0.0f;
    bIsCameraTransitioning = false;

    PrimaryActorTick.bCanEverTick = true;

    /*
    BuildingArea = CreateDefaultSubobject<UBoxComponent>(TEXT("BuildingArea"));
    if (BuildingArea)
    {
        BuildingArea->SetupAttachment(RootComponent);
        BuildingArea->SetBoxExtent(FVector(1000.0f, 1000.0f, 100.0f));
        BuildingArea->SetCollisionProfileName(TEXT("OverlapAll"));
    }
    */
    // Initialize other member variables
    HighlightVolume = nullptr;
    RotationAngle = 45.0f;
    bIsMovingObject = false;
    SelectedObject = nullptr;
}

void ASupermarketCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsCameraTransitioning)
    {
        UpdateCameraTransition();
    }
    

    if (bIsBuildModeActive && BuildModeCamera)
    {
        LastBuildModeCameraTransform = BuildModeCamera->GetActorTransform();
    }

    UpdateProductBoxTransform();

    if (bIsBuildModeActive && bIsMovingObject && SelectedObject)
    {
        MoveSelectedObject();
    }
}

void ASupermarketCharacter::BeginPlay()
{
    // Call the base class  
    Super::BeginPlay();
    CreateMoneyDisplayWidget();
    
    OriginalCameraRotation = FirstPersonCameraComponent->GetRelativeRotation();
    OriginalCameraFOV = FirstPersonCameraComponent->FieldOfView;
    SetupTabletScreen();
    if (Controller)
    {
        OriginalControllerRotation = Controller->GetControlRotation();
    }
    
    SetupBuildModeInputs();
    SetupInputMappingContexts();
    UpdateMovementState(); // Ensure correct initial state
    UpdateTabletTransform();
    bIsBuildModeActive = false;

    APlayerController* PlayerController = Cast<APlayerController>(Controller);
    if (PlayerController)
    {
        UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
        if (Subsystem)
        {
            Subsystem->AddMappingContext(SupermarketMappingContext, 0);
        }
    }

    if (!HighlightVolume && GetWorld())
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;
        HighlightVolume = GetWorld()->SpawnActor<APostProcessVolume>(APostProcessVolume::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

        if (HighlightVolume)
        {
            HighlightVolume->bUnbound = true;
            HighlightVolume->Settings.bOverride_DynamicGlobalIlluminationMethod = true;
            HighlightVolume->Settings.DynamicGlobalIlluminationMethod = EDynamicGlobalIlluminationMethod::Lumen;
           ////UE_LOGLogTemp, Log, TEXT("HighlightVolume created successfully"));
        }
        else
        {
           ////UE_LOGLogTemp, Error, TEXT("Failed to create HighlightVolume"));
        }
    }
    else if (HighlightVolume)
    {
       ////UE_LOGLogTemp, Warning, TEXT("HighlightVolume already exists"));
    }
    else
    {
       ////UE_LOGLogTemp, Error, TEXT("Invalid World context"));
    }

    
    // Spawn BuildingArea in the world instead of attaching to the character
    if (!BuildingArea)
    {
        BuildingArea = NewObject<UBoxComponent>(this, TEXT("BuildingArea"));
        if (BuildingArea)
        {
            BuildingArea->RegisterComponent();
            BuildingArea->SetWorldLocation(FVector(1941, 2477, 95)); // Set to desired location
            BuildingArea->SetBoxExtent(FVector(1000.0f, 1000.0f, 100.0f));
            BuildingArea->SetCollisionProfileName(TEXT("NoCollision"));
            BuildingArea->SetHiddenInGame(false); // Make it visible for debugging
        }
    }
  
    // Initialize StoreManager reference
    CreateAndShowStoreStatusWidget();
    StoreManager = AStoreManager::GetInstance(GetWorld());
    if (StoreManager)
    {
        // Ensure the store starts closed
        StoreManager->SetStoreOpen(false);
    }

}

//////////////////////////////////////////////////////////////////////////// Input

void ASupermarketCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    // Set up action bindings
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        // Jumping
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

        // Moving
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASupermarketCharacter::Move);

        // Looking
        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASupermarketCharacter::Look);
        EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &ASupermarketCharacter::OnInteract);
        EnhancedInputComponent->BindAction(DropAction, ETriggerEvent::Triggered, this, &ASupermarketCharacter::DropProductBox);
        EnhancedInputComponent->BindAction(StockAction, ETriggerEvent::Started, this, &ASupermarketCharacter::StartStocking);
        EnhancedInputComponent->BindAction(StockAction, ETriggerEvent::Completed, this, &ASupermarketCharacter::StopStocking);
        EnhancedInputComponent->BindAction(TabletAction, ETriggerEvent::Started, this, &ASupermarketCharacter::OnTabletAction);

        EnhancedInputComponent->BindAction(TabletClickAction, ETriggerEvent::Started, this, &ASupermarketCharacter::OnTabletClickInput);


        EnhancedInputComponent->BindAction(BuildModeAction, ETriggerEvent::Started, this, &ASupermarketCharacter::ToggleBuildMode);
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASupermarketCharacter::MoveInBuildMode);
        EnhancedInputComponent->BindAction(BuildModeRotateAction, ETriggerEvent::Triggered, this, &ASupermarketCharacter::RotateInBuildMode);
        EnhancedInputComponent->BindAction(RightMouseButtonPressedAction, ETriggerEvent::Started, this, &ASupermarketCharacter::OnRightMouseButtonPressed);
        EnhancedInputComponent->BindAction(RightMouseButtonReleasedAction, ETriggerEvent::Completed, this, &ASupermarketCharacter::OnRightMouseButtonReleased);

        PlayerInputComponent->BindAction("ToggleObjectMovement", IE_Pressed, this, &ASupermarketCharacter::ToggleObjectMovement);
        PlayerInputComponent->BindAction("RotateObjectLeft", IE_Pressed, this, &ASupermarketCharacter::RotateObjectLeft);
        PlayerInputComponent->BindAction("RotateObjectRight", IE_Pressed, this, &ASupermarketCharacter::RotateObjectRight);
        PlayerInputComponent->BindAction("LeftMouseButton", IE_Pressed, this, &ASupermarketCharacter::OnLeftMouseButtonPressed);
        PlayerInputComponent->BindAction("LeftMouseButton", IE_Released, this, &ASupermarketCharacter::OnLeftMouseButtonReleased);

        EnhancedInputComponent->BindAction(ToggleStoreStatusAction, ETriggerEvent::Triggered, this, &ASupermarketCharacter::ToggleStoreStatusInput);
    }
    else
    {
       ////UE_LOGLogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
    }
}

void ASupermarketCharacter::OnTabletAction()
{
    if (!bIsBuildModeActive)
    {
        ToggleTabletMode();
    }
}

void ASupermarketCharacter::ToggleTabletMode()
{
    bIsTabletMode = !bIsTabletMode;
    SetupTabletMode(bIsTabletMode);

}


void ASupermarketCharacter::SetupTabletMode(bool bEnable)
{
    bIsTabletMode = bEnable;
    StartCameraTransition(bEnable);

    if (bEnable)
    {
        // Enable tablet mode
        if (TabletMesh && TabletCameraComponent)
        {
            // Attach the tablet to the tablet camera
            TabletMesh->AttachToComponent(TabletCameraComponent, FAttachmentTransformRules::KeepRelativeTransform);

            // Set the relative location and rotation of the tablet
            TabletMesh->SetRelativeLocation(FVector(13.0f, 0.0f, 0.0f)); // Adjust these values as needed
            TabletMesh->SetRelativeRotation(FRotator(-90.0f, 180.0f, 0.0f)); // Adjust these values as needed

            TabletMesh->SetVisibility(true);
        }
        if (TabletScreenWidget)
        {
            TabletScreenWidget->SetVisibility(true);
        }

        // Disable character movement
        if (GetCharacterMovement())
        {
            GetCharacterMovement()->DisableMovement();
        }

        // Show mouse cursor and enable input for UI
        APlayerController* PC = Cast<APlayerController>(GetController());
        if (PC)
        {
            PC->SetShowMouseCursor(true);
            PC->SetInputMode(FInputModeGameAndUI());
        }
    }
    else
    {
        // Disable tablet mode
        if (TabletMesh)
        {
            // Detach the tablet from the camera
            TabletMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

            // Reattach to the original socket or component if necessary
            // TabletMesh->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, "hand_r");

            TabletMesh->SetVisibility(false);
        }
        if (TabletScreenWidget)
        {
            TabletScreenWidget->SetVisibility(false);
        }

        // Enable character movement
        if (GetCharacterMovement())
        {
            GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        }

        // Hide mouse cursor and reset input mode
        APlayerController* PC = Cast<APlayerController>(GetController());
        if (PC)
        {
            PC->SetShowMouseCursor(false);
            PC->SetInputMode(FInputModeGameOnly());
        }
    }

    // Switch between first person and tablet cameras
    SwitchCamera(bEnable);
}

void ASupermarketCharacter::StartCameraTransition(bool bToTabletView)
{
    if (FirstPersonCameraComponent && TabletCameraComponent)
    {
        bIsCameraTransitioning = true;
        CameraTransitionElapsedTime = 0.0f;
        CameraTransitionDuration = CameraTransitionTime;

        // Get the current view rotation
        APlayerController* PC = Cast<APlayerController>(GetController());
        FRotator CurrentViewRotation = PC ? PC->GetControlRotation() : GetActorRotation();

        if (bToTabletView)
        {
            // Store the initial rotation when entering tablet mode
            InitialRotation = CurrentViewRotation;

            // Store the start rotation (current view) and target rotation
            StartCameraRotation = CurrentViewRotation;
            TargetCameraRotation = TabletCameraComponent->GetRelativeRotation() + GetActorRotation();

            // Set the TabletCameraComponent to start at the current view rotation
            TabletCameraComponent->SetWorldRotation(StartCameraRotation);
        }
        else
        {
            // When transitioning back, start from the current tablet rotation and target the initial rotation
            StartCameraRotation = TabletCameraComponent->GetComponentRotation();
            TargetCameraRotation = InitialRotation;
        }

        // Log for debugging
      // ////UE_LOGLogTemp, Log, TEXT("Starting camera transition. To Tablet: %s, Start Rotation: %s, Target Rotation: %s"),
        //    bToTabletView ? TEXT("True") : TEXT("False"),
        //    *StartCameraRotation.ToString(),
       //     *TargetCameraRotation.ToString());

        // Activate the appropriate camera immediately
        FirstPersonCameraComponent->SetActive(!bToTabletView);
        TabletCameraComponent->SetActive(bToTabletView);
    }
}

void ASupermarketCharacter::UpdateCameraTransition()
{
    if (FirstPersonCameraComponent && TabletCameraComponent)
    {
        CameraTransitionElapsedTime += GetWorld()->GetDeltaSeconds();
        float Alpha = FMath::Clamp(CameraTransitionElapsedTime / CameraTransitionDuration, 0.0f, 1.0f);

        // Use a smooth step function for easing
        float SmoothedAlpha = FMath::SmoothStep(0.0f, 1.0f, Alpha);

        // Interpolate the rotation
        FRotator NewRotation = FMath::Lerp(StartCameraRotation, TargetCameraRotation, SmoothedAlpha);

        // Apply the interpolated rotation to the active camera
        UCameraComponent* ActiveCamera = bIsTabletMode ? TabletCameraComponent : FirstPersonCameraComponent;
        ActiveCamera->SetWorldRotation(NewRotation);

        // Update the player's control rotation
        APlayerController* PC = Cast<APlayerController>(GetController());
        if (PC)
        {
            PC->SetControlRotation(NewRotation);
        }

        // Log for debugging
       //////UE_LOGLogTemp, Log, TEXT("Updating camera transition. Alpha: %f, New Rotation: %s"),
           // SmoothedAlpha, *NewRotation.ToString());

        if (Alpha >= 1.0f)
        {
            bIsCameraTransitioning = false;

            // Ensure final rotation is set
            ActiveCamera->SetWorldRotation(TargetCameraRotation);
            if (PC)
            {
                PC->SetControlRotation(TargetCameraRotation);
            }

            // Log for debugging
           ////UE_LOGLogTemp, Log, TEXT("Camera transition complete. Tablet mode: %s, Final Rotation: %s"),
            //    bIsTabletMode ? TEXT("True") : TEXT("False"), *TargetCameraRotation.ToString());
        }
    }
}



void ASupermarketCharacter::SwitchCamera(bool bUseTabletCamera)
{
    if (FirstPersonCameraComponent && TabletCameraComponent)
    {
        FirstPersonCameraComponent->SetActive(!bUseTabletCamera);
        TabletCameraComponent->SetActive(bUseTabletCamera);

        // Set the view target to the appropriate camera
        APlayerController* PC = Cast<APlayerController>(GetController());
        if (PC)
        {
            PC->SetViewTarget(this);
        }
    }
}

void ASupermarketCharacter::SetCameraRotationEnabled(bool bEnable)
{
    bCameraRotationEnabled = bEnable;

    APlayerController* PC = Cast<APlayerController>(Controller);
    if (PC)
    {
        if (bEnable)
        {
            PC->SetIgnoreLookInput(false);
        }
        else
        {
            PC->SetIgnoreLookInput(true);
        }
    }
}

void ASupermarketCharacter::StartStocking()
{
    if (HeldProductBox)  // Only start stocking if holding a product box
    {
        bIsStocking = true;
        GetWorldTimerManager().SetTimer(StockingTimerHandle, this, &ASupermarketCharacter::CheckShelfInView, 0.1f, true);
    }
}

void ASupermarketCharacter::StopStocking()
{
    bIsStocking = false;
    GetWorldTimerManager().ClearTimer(StockingTimerHandle);
    if (CurrentTargetShelf)
    {
        CurrentTargetShelf->StopStockingShelf();
        CurrentTargetShelf = nullptr;
    }

    // Check if the ProductBox is empty and destroy it if necessary
    if (HeldProductBox && HeldProductBox->IsEmpty())
    {
        HeldProductBox->Destroy();
        HeldProductBox = nullptr;
    }
}



void ASupermarketCharacter::CheckShelfInView()
{
    if (!bIsStocking || !HeldProductBox)
    {
        return;
    }

    FVector Start = FirstPersonCameraComponent->GetComponentLocation();
    FVector End = Start + FirstPersonCameraComponent->GetForwardVector() * 300.0f;

    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);

    if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, QueryParams))
    {
        AShelf* HitShelf = Cast<AShelf>(HitResult.GetActor());
        if (HitShelf)
        {
            if (HitShelf != CurrentTargetShelf)
            {
                if (CurrentTargetShelf)
                {
                    CurrentTargetShelf->StopStockingShelf();
                }
                CurrentTargetShelf = HitShelf;
                if (!HeldProductBox->IsEmpty())
                {
                    CurrentTargetShelf->SetProductBox(HeldProductBox);
                    CurrentTargetShelf->StartStockingShelf(HeldProductBox->GetProductClass());
                }
                else
                {
                    // ProductBox is empty, stop stocking
                    StopStocking();
                }
            }
        }
        else
        {
            if (CurrentTargetShelf)
            {
                CurrentTargetShelf->StopStockingShelf();
                CurrentTargetShelf = nullptr;
            }
        }
    }
    else
    {
        if (CurrentTargetShelf)
        {
            CurrentTargetShelf->StopStockingShelf();
            CurrentTargetShelf = nullptr;
        }
    }
}

void ASupermarketCharacter::Move(const FInputActionValue& Value)
{
    if (Controller == nullptr || bIsBuildModeActive)
        return;

    const FVector2D MovementVector = Value.Get<FVector2D>();

    const FRotator Rotation = Controller->GetControlRotation();
    const FRotator YawRotation(0, Rotation.Yaw, 0);

    const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

    AddMovementInput(ForwardDirection, MovementVector.Y);
    AddMovementInput(RightDirection, MovementVector.X);
}

void ASupermarketCharacter::Look(const FInputActionValue& Value)
{
    if (Controller == nullptr)
        return;

    const FVector2D LookAxisVector = Value.Get<FVector2D>();

    if (bIsBuildModeActive)
    {
        if (BuildModeCamera && bIsRightMouseButtonPressed)
        {
            BuildModeCamera->RotateCamera(LookAxisVector);
        }
    }
    else
    {
        AddControllerYawInput(LookAxisVector.X);
        AddControllerPitchInput(LookAxisVector.Y);
    }
}


void ASupermarketCharacter::OnInteract()
{
    if (HeldProductBox)
    {
        // If already holding a box, do nothing (dropping is handled by 'G' key)
        return;
    }

    // Try to pick up a box
    FVector Start = FirstPersonCameraComponent->GetComponentLocation();
    FVector End = Start + FirstPersonCameraComponent->GetForwardVector() * 300.0f;

    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);

    if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, QueryParams))
    {
        if (AProductBox* ProductBox = Cast<AProductBox>(HitResult.GetActor()))
        {
            PickUpProductBox(ProductBox);
        }
    }
}

bool ASupermarketCharacter::GetHasRifle()
{
    return bHasRifle;
}

void ASupermarketCharacter::SetHasRifle(bool bNewHasRifle)
{
    bHasRifle = bNewHasRifle;
}

void ASupermarketCharacter::PickUpProductBox(AProductBox* ProductBox)
{
    if (ProductBox && !HeldProductBox)
    {
        HeldProductBox = ProductBox;

        // Disable physics simulation
        UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(HeldProductBox->GetRootComponent());
        if (PrimitiveComponent)
        {
            PrimitiveComponent->SetSimulatePhysics(false);
        }

        // Attach the product box to the camera
        HeldProductBox->AttachToComponent(FirstPersonCameraComponent);

        // Apply custom offset and rotation
        UpdateProductBoxTransform();

        bIsInteracting = true;
    }
}


void ASupermarketCharacter::DropProductBox()
{
    if (HeldProductBox)
    {
        HeldProductBox->DetachFromCamera();

        // Re-enable physics simulation
        UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(HeldProductBox->GetRootComponent());
        if (PrimitiveComponent)
        {
            PrimitiveComponent->SetSimulatePhysics(true);
        }

        HeldProductBox = nullptr;
        bIsInteracting = false;
    }
}



void ASupermarketCharacter::InteractWithShelf(AShelf* Shelf)
{
    if (Shelf && HeldProductBox)
    {
        TSubclassOf<AProduct> BoxProductClass = HeldProductBox->GetProductClass();
        TSubclassOf<AProduct> ShelfProductClass = Shelf->GetCurrentProductClass();

        if (!ShelfProductClass || ShelfProductClass == BoxProductClass)
        {
            // Start stocking the shelf
            Shelf->StartStockingShelf(BoxProductClass);

            // Remove a product from the box
            AProduct* RemovedProduct = HeldProductBox->RemoveProduct();
            if (RemovedProduct)
            {
                RemovedProduct->Destroy(); // Destroy the product as it's now on the shelf
            }

            // If the box is empty, destroy it
            if (HeldProductBox->GetProductCount() == 0)
            {
                HeldProductBox->Destroy();
                HeldProductBox = nullptr;
            }
        }
        else
        {
            // Clear the shelf and start stocking with the new product
            while (Shelf->RemoveNextProduct())
            {
                // Remove all products from the shelf
            }
            Shelf->StartStockingShelf(BoxProductClass);

            // Remove a product from the box
            AProduct* RemovedProduct = HeldProductBox->RemoveProduct();
            if (RemovedProduct)
            {
                RemovedProduct->Destroy(); // Destroy the product as it's now on the shelf
            }

            // If the box is empty, destroy it
            if (HeldProductBox->GetProductCount() == 0)
            {
                HeldProductBox->Destroy();
                HeldProductBox = nullptr;
            }
        }
    }
}

void ASupermarketCharacter::SetupTabletScreen()
{
    if (TabletWidgetClass && TabletScreenWidget)
    {
        TabletScreenWidget->SetWidgetClass(TabletWidgetClass);

        // Set a high resolution for the widget
        TabletScreenWidget->SetDrawAtDesiredSize(false);
        TabletScreenWidget->SetDrawSize(FVector2D(1920, 1080)); // Full HD resolution

        // Set the widget to a smaller physical size
        TabletScreenWidget->SetRelativeScale3D(FVector(0.002500,0.073200,0.203000)); // Adjust this value as needed

        // Ensure the widget is set to be transparent
        TabletScreenWidget->SetBackgroundColor(FLinearColor::Transparent);

        // Initially hide the widget
        TabletScreenWidget->SetVisibility(false);
    }
}

void ASupermarketCharacter::SetTabletScreenContent(TSubclassOf<UUserWidget> NewWidgetClass)
{
    if (NewWidgetClass && TabletScreenWidget)
    {
        TabletScreenWidget->SetWidgetClass(NewWidgetClass);
        TabletScreenWidget->SetVisibility(true);
    }
}

UUserWidget* ASupermarketCharacter::GetTabletScreenWidget() const
{
    return TabletScreenWidget ? TabletScreenWidget->GetUserWidgetObject() : nullptr;
}

void ASupermarketCharacter::UpdateTabletTransform()
{
    if (TabletMesh)
    {
        TabletMesh->SetRelativeLocation(TabletOffset);
        TabletMesh->SetRelativeRotation(TabletRotation);
    }

    if (TabletScreenWidget)
    {
        TabletScreenWidget->SetRelativeLocation(TabletScreenOffset);
        TabletScreenWidget->SetRelativeRotation(TabletScreenRotation);
    }
}

void ASupermarketCharacter::OnTabletClickInput()
{
    if (bIsTabletMode)
    {
       ////UE_LOGLogTemp, Log, TEXT("Tablet click input detected"));

        APlayerController* PC = Cast<APlayerController>(GetController());
        if (PC)
        {
            FVector2D MousePosition;
            if (PC->GetMousePosition(MousePosition.X, MousePosition.Y))
            {
               ////UE_LOGLogTemp, Log, TEXT("Mouse Position: %s"), *MousePosition.ToString());
                OnTabletClicked(MousePosition);
            }
        }
    }
}
void ASupermarketCharacter::UpdateProductBoxTransform()
{
    if (HeldProductBox)
    {
        HeldProductBox->SetActorRelativeLocation(ProductBoxOffset);
        HeldProductBox->SetActorRelativeRotation(ProductBoxRotation);
    }
}


void ASupermarketCharacter::CreateMoneyDisplayWidget()
{
   ////UE_LOGLogTemp, Log, TEXT("CreateMoneyDisplayWidget function started"));

    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
       ////UE_LOGLogTemp, Log, TEXT("PlayerController found"));

        // Load the widget class
        TSubclassOf<UUserWidget> MoneyWidgetClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/WBP_MoneyDisplay.WBP_MoneyDisplay_C"));

        if (MoneyWidgetClass)
        {
           ////UE_LOGLogTemp, Log, TEXT("MoneyWidgetClass loaded successfully"));

            // Create the widget
            MoneyDisplayWidget = CreateWidget<UMoneyDisplayWidget>(PC, MoneyWidgetClass);

            if (MoneyDisplayWidget)
            {
               ////UE_LOGLogTemp, Log, TEXT("MoneyDisplayWidget created successfully"));

                // Add the widget to the viewport
                MoneyDisplayWidget->AddToViewport();
               ////UE_LOGLogTemp, Log, TEXT("MoneyDisplayWidget added to viewport"));
            }
            else
            {
               ////UE_LOGLogTemp, Error, TEXT("Failed to create MoneyDisplayWidget"));
            }
        }
        else
        {
           ////UE_LOGLogTemp, Error, TEXT("Failed to load MoneyWidgetClass"));
        }
    }
    else
    {
       ////UE_LOGLogTemp, Error, TEXT("PlayerController not found"));
    }
}



void ASupermarketCharacter::SetupBuildModeInputs()
{
    if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            // Assuming you have a separate Input Mapping Context for build mode
            Subsystem->AddMappingContext(BuildModeMappingContext, 1);
        }
    }
}

void ASupermarketCharacter::ToggleBuildMode()
{
    bIsBuildModeActive = !bIsBuildModeActive;

    APlayerController* PlayerController = Cast<APlayerController>(Controller);
    if (PlayerController)
    {
        UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
        if (Subsystem)
        {
            if (bIsBuildModeActive)
            {
                Subsystem->AddMappingContext(BuildModeMappingContext, 1);
                PlayerController->SetShowMouseCursor(true);
                PlayerController->SetInputMode(FInputModeGameAndUI());

                if (!BuildModeCamera)
                {
                    FActorSpawnParameters SpawnParams;
                    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

                    FVector SpawnLocation = GetActorLocation();
                    SpawnLocation.Z += 1000.0f; // Raise the camera 1000 units above the player

                    FRotator SpawnRotation(-60.0f, 0.0f, 0.0f); // Default downward angle

                    BuildModeCamera = GetWorld()->SpawnActor<ABuildModeCameraActor>(BuildModeCameraClass, SpawnLocation, SpawnRotation, SpawnParams);

                    if (BuildModeCamera)
                    {
                        BuildModeCamera->SetPivotLocation(GetActorLocation());
                    }
                }

                if (BuildModeCamera)
                {
                    PlayerController->SetViewTargetWithBlend(BuildModeCamera, 0.5f);
                }

                if (BuildModeWidgetClass && !BuildModeWidget)
                {
                    BuildModeWidget = CreateWidget<UUserWidget>(GetWorld(), BuildModeWidgetClass);
                    if (BuildModeWidget)
                    {
                        BuildModeWidget->AddToViewport();
                    }
                }
            }
            else
            {
                Subsystem->RemoveMappingContext(BuildModeMappingContext);
                PlayerController->SetShowMouseCursor(false);
                PlayerController->SetInputMode(FInputModeGameOnly());

                if (BuildModeCamera)
                {
                    BuildModeCamera->Destroy();
                    BuildModeCamera = nullptr;
                }

                PlayerController->SetViewTargetWithBlend(this, 0.5f);

                // If an object is still being moved, reset it to its original position
                if (bIsMovingObject && SelectedObject)
                {
                    ResetObjectToOriginalPosition();
                    bIsMovingObject = false;
                    SelectedObject = nullptr;
                }

                if (BuildModeWidget)
                {
                    BuildModeWidget->RemoveFromParent();
                    BuildModeWidget = nullptr;
                }
            }
        }
    }

    // Disable character movement in build mode
    GetCharacterMovement()->SetMovementMode(bIsBuildModeActive ? MOVE_None : MOVE_Walking);
}


void ASupermarketCharacter::MoveInBuildMode(const FInputActionValue& Value)
{
    if (bIsBuildModeActive && BuildModeCamera)
    {
        FVector2D MovementVector = Value.Get<FVector2D>();
        BuildModeCamera->MoveCamera(MovementVector);
    }
}

void ASupermarketCharacter::RotateInBuildMode(const FInputActionValue& Value)
{
    if (bIsBuildModeActive && BuildModeCamera && bIsRightMouseButtonPressed)
    {
        const FVector2D RotationVector = Value.Get<FVector2D>();
        BuildModeCamera->RotateCamera(RotationVector);
    }
}

void ASupermarketCharacter::UpdateMovementState()
{
    if (bIsBuildModeActive)
    {
        // Disable character movement
        GetCharacterMovement()->SetMovementMode(MOVE_None);
    }
    else
    {
        // Enable character movement
        GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    }
}

void ASupermarketCharacter::OnRightMouseButtonPressed()
{
    bIsRightMouseButtonPressed = true;
    if (bIsBuildModeActive)
    {
        APlayerController* PC = Cast<APlayerController>(Controller);
        if (PC)
        {
            PC->SetInputMode(FInputModeGameOnly());
            PC->SetShowMouseCursor(false);
        }
    }
}

void ASupermarketCharacter::OnRightMouseButtonReleased()
{
    bIsRightMouseButtonPressed = false;
    if (bIsBuildModeActive)
    {
        APlayerController* PC = Cast<APlayerController>(Controller);
        if (PC)
        {
            PC->SetInputMode(FInputModeGameAndUI());
            PC->SetShowMouseCursor(true);
        }
    }
}

void ASupermarketCharacter::SetupInputMappingContexts()
{
    APlayerController* PlayerController = Cast<APlayerController>(Controller);
    if (PlayerController)
    {
        UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
        if (Subsystem)
        {
            Subsystem->AddMappingContext(SupermarketMappingContext, 0);
            // BuildModeMappingContext will be added/removed in ToggleBuildMode
        }
    }
}

void ASupermarketCharacter::ToggleObjectMovement()
{
    if (!bIsBuildModeActive) return;

    if (!bIsMovingObject)
    {
        // Start moving an object
        SelectedObject = GetActorUnderCursor();
        if (SelectedObject)
        {
            bIsMovingObject = true;
            UE_LOG(LogTemp, Display, TEXT("Selected object: %s"), *SelectedObject->GetName());
            InitialObjectPosition = SelectedObject->GetActorLocation();
            OriginalObjectRotation = SelectedObject->GetActorRotation();

            // Store the original material
            UStaticMeshComponent* MeshComponent = SelectedObject->FindComponentByClass<UStaticMeshComponent>();
            if (MeshComponent)
            {
                OriginalMaterial = MeshComponent->GetMaterial(0);
            }

            // Calculate and store the click offset
            APlayerController* PC = Cast<APlayerController>(GetController());
            if (PC)
            {
                FVector WorldLocation, WorldDirection;
                if (PC->DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
                {
                    FVector IntersectionPoint = FMath::LinePlaneIntersection(
                        WorldLocation,
                        WorldLocation + WorldDirection * 10000.0f,
                        FPlane(InitialObjectPosition, FVector::UpVector)
                    );
                    ClickOffset = InitialObjectPosition - IntersectionPoint;
                }
            }

            // Start moving the object
            StartObjectMovement();
        }
    }
    else
    {
        // Attempt to stop moving the object
        if (bIsValidPlacement)
        {
            // If the placement is valid, stop moving the object
            UE_LOG(LogTemp, Display, TEXT("Placed object: %s"), *SelectedObject->GetName());
            ApplyMaterialToActor(SelectedObject, OriginalMaterial);
            bIsMovingObject = false;
            SelectedObject = nullptr;
        }
        else
        {
            // If the placement is invalid, keep moving the object
            UE_LOG(LogTemp, Warning, TEXT("Invalid placement. Object remains selected and moving."));
        }
    }
}

void ASupermarketCharacter::RotateObjectLeft()
{
    if (bIsBuildModeActive && SelectedObject)
    {
        SelectedObject->AddActorWorldRotation(FRotator(0, -RotationAngle, 0));
       ////UE_LOGLogTemp, Display, TEXT("Rotated object left: %s"), *SelectedObject->GetName());
    }
}

void ASupermarketCharacter::RotateObjectRight()
{
    if (bIsBuildModeActive && SelectedObject)
    {
        SelectedObject->AddActorWorldRotation(FRotator(0, RotationAngle, 0));
       ////UE_LOGLogTemp, Display, TEXT("Rotated object right: %s"), *SelectedObject->GetName());
    }
}

void ASupermarketCharacter::OnLeftMouseButtonPressed()
{
    if (bIsBuildModeActive)
    {
        StartObjectMovement();
    }
}


void ASupermarketCharacter::OnLeftMouseButtonReleased()
{
    if (bIsBuildModeActive && bIsMovingObject)
    {
        ToggleObjectMovement();
    }
}

void ASupermarketCharacter::MoveSelectedObject()
{
    if (!SelectedObject || !BuildModeCamera) return;

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

    FVector WorldLocation, WorldDirection;
    if (PC->DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
    {
        FVector IntersectionPoint = FMath::LinePlaneIntersection(
            WorldLocation,
            WorldLocation + WorldDirection * 10000.0f,
            FPlane(InitialObjectPosition, FVector::UpVector)
        );

        // Apply the click offset to maintain the relative position
        FVector NewLocation = IntersectionPoint + ClickOffset;
        NewLocation.Z = InitialObjectPosition.Z; // Maintain the original height

        // Clamp the position to the building area bounds
        FVector ClampedLocation = ClampLocationToBuildingArea(NewLocation);

        // Move the object to the new location
        SelectedObject->SetActorLocation(ClampedLocation);

        // Update the placement validity
        UpdateObjectPlacement();

        // Apply the appropriate material based on placement validity
        ApplyMaterialToActor(SelectedObject, bIsValidPlacement ? ValidPlacementMaterial : InvalidPlacementMaterial);

        // Debug visualization
        if (GetWorld() && GetWorld()->IsPlayInEditor())
        {
            FColor DebugColor = bIsValidPlacement ? FColor::Green : FColor::Red;
            DrawDebugBox(GetWorld(), ClampedLocation, FVector(100, 100, 100), DebugColor, false, -1.0f, 0, 2.0f);
        }

        UE_LOG(LogTemp, Verbose, TEXT("Moving object to: %s, Valid Placement: %s"),
            *ClampedLocation.ToString(), bIsValidPlacement ? TEXT("True") : TEXT("False"));
    }
}


void ASupermarketCharacter::HighlightHoveredObject(AActor* HoveredActor)
{
    static AActor* LastHighlightedActor = nullptr;

    // Clear previous highlight
    if (LastHighlightedActor && LastHighlightedActor != HoveredActor)
    {
        TArray<UStaticMeshComponent*> MeshComponents;
        LastHighlightedActor->GetComponents<UStaticMeshComponent>(MeshComponents);
        for (UStaticMeshComponent* MeshComponent : MeshComponents)
        {
            if (MeshComponent)
            {
                MeshComponent->SetRenderCustomDepth(false);
            }
        }
    }

    // Apply new highlight
    if (HoveredActor && HoveredActor != LastHighlightedActor)
    {
        TArray<UStaticMeshComponent*> MeshComponents;
        HoveredActor->GetComponents<UStaticMeshComponent>(MeshComponents);
        for (UStaticMeshComponent* MeshComponent : MeshComponents)
        {
            if (MeshComponent)
            {
                MeshComponent->SetRenderCustomDepth(true);
                MeshComponent->SetCustomDepthStencilValue(1); // Make sure this value matches your post-process material
            }
        }
       ////UE_LOGLogTemp, Verbose, TEXT("Highlighting object: %s"), *HoveredActor->GetName());
    }

    LastHighlightedActor = HoveredActor;
}

FVector ASupermarketCharacter::GetMouseWorldPosition()
{
    APlayerController* PlayerController = Cast<APlayerController>(GetController());
    if (PlayerController)
    {
        FVector2D MousePosition;
        if (PlayerController->GetMousePosition(MousePosition.X, MousePosition.Y))
        {
            FVector WorldLocation, WorldDirection;
            if (UGameplayStatics::DeprojectScreenToWorld(PlayerController, MousePosition, WorldLocation, WorldDirection))
            {
                FVector Start = BuildModeCamera ? BuildModeCamera->GetActorLocation() : WorldLocation;
                FVector End = Start + WorldDirection * 20000.0f;

                FHitResult HitResult;
                FCollisionQueryParams CollisionParams;
                CollisionParams.AddIgnoredActor(this);
                CollisionParams.AddIgnoredActor(BuildModeCamera);

                if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, CollisionParams))
                {
                    return HitResult.Location;
                }
            }
        }
    }
    return FVector::ZeroVector;
}

AActor* ASupermarketCharacter::GetActorUnderCursor()
{
    APlayerController* PlayerController = Cast<APlayerController>(GetController());
    if (!PlayerController)
    {
        return nullptr;
    }

    FHitResult HitResult;
    if (PlayerController->GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
    {
        AActor* HitActor = HitResult.GetActor();
        if (CanObjectBeMoved(HitActor))
        {
            return HitActor;
        }
    }

    return nullptr;
}

bool ASupermarketCharacter::IsActorInBuildingArea(AActor* Actor)
{
    if (Actor && BuildingArea)
    {
        FVector ActorLocation = Actor->GetActorLocation();
        FVector BoxLocation = BuildingArea->GetComponentLocation();
        FVector BoxExtent = BuildingArea->GetScaledBoxExtent();

        // Check if the actor's location is within the box bounds
        bool bXInBounds = FMath::Abs(ActorLocation.X - BoxLocation.X) <= BoxExtent.X;
        bool bYInBounds = FMath::Abs(ActorLocation.Y - BoxLocation.Y) <= BoxExtent.Y;
        bool bZInBounds = FMath::Abs(ActorLocation.Z - BoxLocation.Z) <= BoxExtent.Z;

        return bXInBounds && bYInBounds && bZInBounds;
    }
    return false;
}

void ASupermarketCharacter::SetBuildingAreaSize(const FVector& NewSize)
{
    if (BuildingArea)
    {
        BuildingArea->SetBoxExtent(NewSize);
    }
}

void ASupermarketCharacter::SetBuildingAreaLocation(const FVector& NewLocation)
{
    if (BuildingArea)
    {
        BuildingArea->SetWorldLocation(NewLocation);
    }
}

FVector ASupermarketCharacter::ClampLocationToBuildingArea(const FVector& Location)
{
    if (!BuildingArea) return Location;

    FVector BuildingAreaLocation = BuildingArea->GetComponentLocation();
    FVector BuildingAreaExtent = BuildingArea->GetScaledBoxExtent();

    FVector ClampedLocation;
    ClampedLocation.X = FMath::Clamp(Location.X, BuildingAreaLocation.X - BuildingAreaExtent.X, BuildingAreaLocation.X + BuildingAreaExtent.X);
    ClampedLocation.Y = FMath::Clamp(Location.Y, BuildingAreaLocation.Y - BuildingAreaExtent.Y, BuildingAreaLocation.Y + BuildingAreaExtent.Y);
    ClampedLocation.Z = Location.Z; // Keep the original Z value

    return ClampedLocation;
}

void ASupermarketCharacter::UpdateObjectPosition()
{
    if (!bIsMovingObject || !SelectedObject || !BuildModeCamera)
        return;

    APlayerController* PlayerController = Cast<APlayerController>(GetController());
    if (!PlayerController)
        return;

    // Get current mouse position in screen space
    FVector2D CurrentMousePosition;
    PlayerController->GetMousePosition(CurrentMousePosition.X, CurrentMousePosition.Y);

    // Calculate the world position of the current mouse cursor
    FVector WorldPosition, WorldDirection;
    PlayerController->DeprojectScreenPositionToWorld(CurrentMousePosition.X, CurrentMousePosition.Y, WorldPosition, WorldDirection);

    // Calculate the movement plane (parallel to the camera's view)
    FVector CameraLocation = BuildModeCamera->GetActorLocation();
    FVector CameraForwardVector = BuildModeCamera->GetActorForwardVector();
    FPlane ObjectMovementPlane(InitialHitPoint, CameraForwardVector);

    // Find the intersection point between the mouse ray and the movement plane
    FVector NewHitPoint = FMath::RayPlaneIntersection(WorldPosition, WorldDirection, ObjectMovementPlane);

    // Calculate the movement delta
    FVector MovementDelta = NewHitPoint - InitialHitPoint;

    // Apply the movement to the object
    FVector NewLocation = InitialObjectTransform.GetLocation() + MovementDelta;

    // Set the new location for the object
    SelectedObject->SetActorLocation(NewLocation);

    // Check if the new position is within the building area
    if (!IsActorInBuildingArea(SelectedObject))
    {
        // If outside, clamp the position to the building area bounds
        FVector ClampedLocation = ClampLocationToBuildingArea(NewLocation);
        SelectedObject->SetActorLocation(ClampedLocation);
    }
}


void ASupermarketCharacter::StopObjectMovement()
{
    if (!SelectedObject) return;

    if (bIsValidPlacement)
    {
        // Finalize the object's position only if it's a valid placement
        UE_LOG(LogTemp, Display, TEXT("Valid placement. Object placed at: %s"), *SelectedObject->GetActorLocation().ToString());

        // Apply the original material back to the object
        ApplyMaterialToActor(SelectedObject, OriginalMaterial);

        bIsMovingObject = false;
        SelectedObject = nullptr;
    }
    else
    {
        // If placement is invalid, keep the object attached to the mouse
        UE_LOG(LogTemp, Warning, TEXT("Invalid placement. Object remains attached to mouse."));
        // Do not reset bIsMovingObject or SelectedObject here
        // The object will continue to move with the mouse in the next frame
    }
}
void ASupermarketCharacter::StartObjectMovement()
{
    if (!bIsBuildModeActive || !BuildModeCamera)
        return;

    APlayerController* PlayerController = Cast<APlayerController>(GetController());
    if (!PlayerController)
        return;

    FHitResult HitResult;
    PlayerController->GetHitResultUnderCursor(ECC_Visibility, false, HitResult);

    SelectedObject = HitResult.GetActor();
    if (!SelectedObject || !CanObjectBeMoved(SelectedObject))
    {
        SelectedObject = nullptr;
        return;
    }

    bIsMovingObject = true;
    InitialObjectPosition = SelectedObject->GetActorLocation();
    OriginalObjectRotation = SelectedObject->GetActorRotation();

    // Calculate and store the click offset
    FVector WorldLocation, WorldDirection;
    if (PlayerController->DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
    {
        FVector IntersectionPoint = FMath::LinePlaneIntersection(
            WorldLocation,
            WorldLocation + WorldDirection * 10000.0f,
            FPlane(InitialObjectPosition, FVector::UpVector)
        );
        ClickOffset = InitialObjectPosition - IntersectionPoint;
    }

    // Store the original material
    UStaticMeshComponent* MeshComponent = SelectedObject->FindComponentByClass<UStaticMeshComponent>();
    if (MeshComponent)
    {
        OriginalMaterial = MeshComponent->GetMaterial(0);
    }

    UE_LOG(LogTemp, Display, TEXT("Started moving object: %s from position: %s"),
        *SelectedObject->GetName(), *InitialObjectPosition.ToString());
}

bool ASupermarketCharacter::CanObjectBeMoved(AActor* Actor)
{
    if (!Actor)
        return false;

    // Check if the actor has the "Moveable" tag
    if (!Actor->ActorHasTag(FName("Moveable")))
        return false;

    // Check if the root component is movable
    USceneComponent* ActorRootComponent = Actor->GetRootComponent();
    if (!ActorRootComponent || ActorRootComponent->Mobility != EComponentMobility::Movable)
        return false;

    // If it's a StaticMeshActor, check if its StaticMeshComponent is movable
    AStaticMeshActor* StaticMeshActor = Cast<AStaticMeshActor>(Actor);
    if (StaticMeshActor)
    {
        UStaticMeshComponent* MeshComponent = StaticMeshActor->GetStaticMeshComponent();
        if (!MeshComponent || MeshComponent->Mobility != EComponentMobility::Movable)
            return false;
    }

    return true;
}

void ASupermarketCharacter::ToggleStoreStatus()
{
    StoreManager = AStoreManager::GetInstance(GetWorld());
    if (StoreManager)
    {
        bool bCurrentStatus = StoreManager->IsStoreOpen();
        StoreManager->SetStoreOpen(!bCurrentStatus);
       ////UE_LOGLogTemp, Display, TEXT("Store status toggled. Now %s"), StoreManager->IsStoreOpen() ? TEXT("OPEN") : TEXT("CLOSED"));

        // Update the widget if it exists
        if (StoreStatusWidget)
        {
            StoreStatusWidget->SetStoreOpen(!bCurrentStatus);
        }
    }
    else
    {
       ////UE_LOGLogTemp, Warning, TEXT("StoreManager not found. Cannot toggle store status."));
    }
}

void ASupermarketCharacter::ToggleStoreStatusInput(const FInputActionValue& Value)
{
    ToggleStoreStatus();
}

void ASupermarketCharacter::CreateAndShowStoreStatusWidget()
{
    if (StoreStatusWidgetClass)
    {
        APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
        if (PlayerController)
        {
            StoreStatusWidget = CreateWidget<UStoreStatusWidget>(PlayerController, StoreStatusWidgetClass);
            if (StoreStatusWidget)
            {
                StoreStatusWidget->AddToViewport();
               ////UE_LOGLogTemp, Display, TEXT("Store Status Widget added to viewport"));
            }
            else
            {
               ////UE_LOGLogTemp, Error, TEXT("Failed to create Store Status Widget"));
            }
        }
        else
        {
           ////UE_LOGLogTemp, Error, TEXT("PlayerController not found"));
        }
    }
    else
    {
       ////UE_LOGLogTemp, Error, TEXT("StoreStatusWidgetClass is not set"));
    }
}

bool ASupermarketCharacter::IsHoldingProductBox(AProductBox* Box) const
{
    return HeldProductBox == Box;
}


void ASupermarketCharacter::ResetObjectToOriginalPosition()
{
    if (SelectedObject)
    {
        SelectedObject->SetActorLocation(OriginalObjectPosition);
        SelectedObject->SetActorRotation(OriginalObjectRotation);
        UE_LOG(LogTemp, Display, TEXT("Object reset to original position: %s"), *OriginalObjectPosition.ToString());

        // Ensure we update the object's appearance
        ApplyMaterialToActor(SelectedObject, OriginalMaterial);
    }
}

void ASupermarketCharacter::UpdateObjectPlacement()
{
    if (!SelectedObject) return;

    USceneBoxComponent* SceneBox = SelectedObject->FindComponentByClass<USceneBoxComponent>();
    if (SceneBox)
    {
        bool bOverlapping = SceneBox->CheckOverlap();
        bool bInBuildingArea = IsActorInBuildingArea(SelectedObject);
        bIsValidPlacement = !bOverlapping && bInBuildingArea;

        UE_LOG(LogTemp, Display, TEXT("UpdateObjectPlacement - Object: %s, Overlap: %s, In Building Area: %s, Valid Placement: %s"),
            *SelectedObject->GetName(),
            bOverlapping ? TEXT("True") : TEXT("False"),
            bInBuildingArea ? TEXT("True") : TEXT("False"),
            bIsValidPlacement ? TEXT("True") : TEXT("False"));

        
    }
    else
    {
        bIsValidPlacement = IsActorInBuildingArea(SelectedObject);
        UE_LOG(LogTemp, Warning, TEXT("SceneBox not found on selected object: %s"), *SelectedObject->GetName());
    }

    // Update the object's appearance based on placement validity
    ApplyMaterialToActor(SelectedObject, bIsValidPlacement ? ValidPlacementMaterial : InvalidPlacementMaterial);
}


void ASupermarketCharacter::ApplyMaterialToActor(AActor* Actor, UMaterialInterface* Material)
{
    if (!Actor || !Material) return;

    TArray<UStaticMeshComponent*> MeshComponents;
    Actor->GetComponents<UStaticMeshComponent>(MeshComponents);

    for (UStaticMeshComponent* MeshComponent : MeshComponents)
    {
        if (MeshComponent)
        {
            MeshComponent->SetMaterial(0, Material);
        }
    }
}

