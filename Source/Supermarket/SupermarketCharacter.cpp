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
#include "GameFramework/CharacterMovementComponent.h"

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

    // Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
    Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
    Mesh1P->SetOnlyOwnerSee(true);
    Mesh1P->SetupAttachment(FirstPersonCameraComponent);
    Mesh1P->bCastDynamicShadow = false;
    Mesh1P->CastShadow = false;
    //Mesh1P->SetRelativeRotation(FRotator(0.9f, -19.19f, 5.2f));
    Mesh1P->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));
    HeldProductBox = nullptr;
    CurrentTargetShelf = nullptr;
    bIsInteracting = false;

    // Create tablet mesh
    TabletMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TabletMesh"));
    TabletMesh->SetupAttachment(GetMesh(), "hand_r"); // Attach to right hand
    TabletMesh->SetVisibility(false);

    // Initialize tablet mode variables
    bIsTabletMode = false;
    TabletCameraRotation = FRotator(-60.f, 0.f, 0.f); // Look down at 60 degree angle
    TabletCameraFOV = 50.f;
    bCameraRotationEnabled = true;
    UE_LOG(LogTemp, Display, TEXT("Initial TabletCameraRotation set to: %s"), *TabletCameraRotation.ToString());

    // Initialize camera transition properties
    CameraTransitionTime = 0.5f;
    CameraTransitionDuration = 0.0f;
    CameraTransitionElapsedTime = 0.0f;
    bIsCameraTransitioning = false;

    PrimaryActorTick.bCanEverTick = true;
}

void ASupermarketCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsCameraTransitioning)
    {
        UpdateCameraTransition();
    }
}

void ASupermarketCharacter::BeginPlay()
{
    // Call the base class  
    Super::BeginPlay();

    OriginalCameraRotation = FirstPersonCameraComponent->GetRelativeRotation();
    OriginalCameraFOV = FirstPersonCameraComponent->FieldOfView;

    if (Controller)
    {
        OriginalControllerRotation = Controller->GetControlRotation();
    }
    //Add Input Mapping Context
    if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
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
    }
    else
    {
        UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
    }
}

void ASupermarketCharacter::OnTabletAction()
{
    ToggleTabletMode();
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
        if (TabletMesh)
        {
            TabletMesh->SetVisibility(true);
        }

        // Show tablet UI
        if (TabletWidget && TabletWidget->IsValidLowLevel())
        {
            TabletWidget->AddToViewport();
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
            TabletMesh->SetVisibility(false);
        }

        // Hide tablet UI
        if (TabletWidget && TabletWidget->IsValidLowLevel())
        {
            TabletWidget->RemoveFromParent();
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
}

void ASupermarketCharacter::StartCameraTransition(bool bToTabletView)
{
    if (FirstPersonCameraComponent && TabletCameraComponent)
    {
        bIsCameraTransitioning = true;
        CameraTransitionElapsedTime = 0.0f;
        CameraTransitionDuration = CameraTransitionTime;

        // Store the start rotation and FOV
        StartCameraRotation = FirstPersonCameraComponent->GetRelativeRotation();
        StartCameraFOV = FirstPersonCameraComponent->FieldOfView;

        // Set target rotation and FOV
        TargetCameraRotation = bToTabletView ? TabletCameraComponent->GetRelativeRotation() : FRotator::ZeroRotator;
        TargetCameraFOV = bToTabletView ? TabletCameraComponent->FieldOfView : 90.0f; // Assuming 90 is the default FOV

        // Log for debugging
        UE_LOG(LogTemp, Log, TEXT("Starting camera transition. Start Rotation: %s, Target Rotation: %s"),
            *StartCameraRotation.ToString(), *TargetCameraRotation.ToString());
    }
}

void ASupermarketCharacter::UpdateCameraTransition()
{
    if (FirstPersonCameraComponent)
    {
        CameraTransitionElapsedTime += GetWorld()->GetDeltaSeconds();
        float Alpha = FMath::Clamp(CameraTransitionElapsedTime / CameraTransitionDuration, 0.0f, 1.0f);

        // Use a smooth step function for easing
        float SmoothedAlpha = FMath::SmoothStep(0.0f, 1.0f, Alpha);

        FRotator NewRotation = FMath::Lerp(StartCameraRotation, TargetCameraRotation, SmoothedAlpha);
        float NewFOV = FMath::Lerp(StartCameraFOV, TargetCameraFOV, SmoothedAlpha);

        FirstPersonCameraComponent->SetRelativeRotation(NewRotation);
        FirstPersonCameraComponent->SetFieldOfView(NewFOV);

        // Log for debugging
        UE_LOG(LogTemp, Log, TEXT("Updating camera transition. Alpha: %f, New Rotation: %s"),
            SmoothedAlpha, *NewRotation.ToString());

        if (Alpha >= 1.0f)
        {
            bIsCameraTransitioning = false;

            // Log for debugging
            UE_LOG(LogTemp, Log, TEXT("Camera transition complete. Final Rotation: %s"),
                *FirstPersonCameraComponent->GetRelativeRotation().ToString());
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
    // input is a Vector2D
    FVector2D MovementVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        // add movement 
        AddMovementInput(GetActorForwardVector(), MovementVector.Y);
        AddMovementInput(GetActorRightVector(), MovementVector.X);
    }
}

void ASupermarketCharacter::Look(const FInputActionValue& Value)
{
    if (!bIsTabletMode)
    {
        // input is a Vector2D
        FVector2D LookAxisVector = Value.Get<FVector2D>();

        if (Controller != nullptr)
        {
            // add yaw and pitch input to controller
            AddControllerYawInput(LookAxisVector.X);
            AddControllerPitchInput(LookAxisVector.Y);
        }
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

        HeldProductBox->AttachToCamera(FirstPersonCameraComponent);
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
