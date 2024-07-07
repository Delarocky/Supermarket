// CashierAI.cpp
#include "CashierAI.h"
#include "Checkout.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Kismet/KismetMathLibrary.h"
#include "Blueprint/AIBlueprintHelperLibrary.h" // Add this include

ACashierAI::ACashierAI()
{
    PrimaryActorTick.bCanEverTick = true;

    ProcessingDelay = 0.5f;
    InterpSpeed = 300.0f;
    bDebugMode = true;
    CurrentCheckout = nullptr;
    AIController = nullptr;

    TextBoxWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("TextBoxWidget"));
    TextBoxWidget->SetupAttachment(RootComponent);
    TextBoxWidget->SetWidgetSpace(EWidgetSpace::Screen);
    TextBoxWidget->SetDrawAtDesiredSize(true);
    TextBoxWidget->SetVisibility(false);

}

void ACashierAI::BeginPlay()
{
    Super::BeginPlay();
    InitializeAIController();
    DebugLog(TEXT("CashierAI initialized"));

    if (TextBoxWidgetClass)
    {
        TextBoxWidget->SetWidgetClass(TextBoxWidgetClass);
    }
}

void ACashierAI::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!CurrentCheckout)
    {
        FindAndMoveToCheckout();
    }

    if (bIsRotating)
    {
        UpdateRotation(DeltaTime);
    }
}

void ACashierAI::InitializeAIController()
{
    AIController = Cast<AAIController>(GetController());
    if (!AIController)
    {
        DebugLog(TEXT("Failed to initialize AIController"));
    }
    else
    {
        DebugLog(TEXT("AIController initialized successfully"));
    }
}

void ACashierAI::LeaveCheckout()
{
    if (CurrentCheckout)
    {
        CurrentCheckout->RemoveCashier();
        CurrentCheckout = nullptr;
        GetWorldTimerManager().ClearTimer(CheckPositionTimerHandle);
        DebugLog(TEXT("Left Checkout"));
    }
    else
    {
        DebugLog(TEXT("Cannot leave Checkout: Not assigned to any Checkout"));
    }
}

void ACashierAI::MoveToCheckout(ACheckout* Checkout)
{
    if (Checkout && AIController)
    {
        CurrentCheckout = Checkout;
        FVector CheckoutLocation = Checkout->GetCashierPosition();

        TArray<FVector> AccessPoints;
        AccessPoints.Add(CheckoutLocation);
        FVector TargetLocation = FindMostAccessiblePoint(AccessPoints);

        MoveTo(TargetLocation);
        GetWorldTimerManager().SetTimer(CheckPositionTimerHandle, this, &ACashierAI::CheckPosition, 0.5f, true);
        DebugLog(FString::Printf(TEXT("Moving to Checkout at location: %s"), *TargetLocation.ToString()));
    }
    else
    {
        DebugLog(TEXT("Cannot move to Checkout: Invalid Checkout or AIController"));
    }
}

void ACashierAI::CheckPosition()
{
    if (CurrentCheckout)
    {
        FVector CashierPosition = CurrentCheckout->GetCashierPosition();
        float DistanceToCheckout = FVector::Dist(GetActorLocation(), CashierPosition);
        if (DistanceToCheckout <= 100.0f)
        {
            GetWorldTimerManager().ClearTimer(CheckPositionTimerHandle);
            TurnToFaceCheckout();
        }
        else
        {
            DebugLog(FString::Printf(TEXT("Still moving to Checkout. Distance: %f"), DistanceToCheckout));
        }
    }
    else
    {
        GetWorldTimerManager().ClearTimer(CheckPositionTimerHandle);
        DebugLog(TEXT("CheckPosition: No assigned Checkout"));
    }
}

void ACashierAI::DebugLog(const FString& Message)
{
    if (bDebugMode)
    {
        UE_LOG(LogTemp, Display, TEXT("[CashierAI] %s"), *Message);
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, FString::Printf(TEXT("[CashierAI] %s"), *Message));
    }
}

bool ACashierAI::IsCheckoutAvailable(ACheckout* Checkout) const
{
    return Checkout && !Checkout->HasCashier() && !Checkout->IsBeingServiced();
}


void ACashierAI::FindAndMoveToCheckout()
{
    TArray<AActor*> FoundCheckouts;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACheckout::StaticClass(), FoundCheckouts);

    bool bFoundAvailableCheckout = false;

    for (AActor* Actor : FoundCheckouts)
    {
        ACheckout* Checkout = Cast<ACheckout>(Actor);
        if (Checkout && IsCheckoutAvailable(Checkout))
        {
            MoveToCheckout(Checkout);
            bFoundAvailableCheckout = true;
            break;
        }
    }

    if (!bFoundAvailableCheckout)
    {
        ShowFloatingText("I need an empty Register!", true);
        GetWorldTimerManager().SetTimer(FindCheckoutTimerHandle, this, &ACashierAI::FindAndMoveToCheckout, 5.0f, false);
    }
    else
    {
        ShowFloatingText("", false);
    }
}

void ACashierAI::ShowFloatingText(const FString& Message, bool bShow)
{
    if (TextBoxWidget)
    {
        TextBoxWidget->SetVisibility(bShow);
        if (bShow)
        {
            UUserWidget* Widget = TextBoxWidget->GetUserWidgetObject();
            if (Widget)
            {
                UTextBlock* TextBlock = Cast<UTextBlock>(Widget->GetWidgetFromName(FName("MessageText")));
                if (TextBlock)
                {
                    TextBlock->SetText(FText::FromString(Message));
                }
            }
        }
    }
}

void ACashierAI::HandleCheckoutArrival()
{
    if (CurrentCheckout)
    {
        if (IsCheckoutAvailable(CurrentCheckout))
        {
            CurrentCheckout->SetCashier(this);
            ShowFloatingText("", false);
            DebugLog(TEXT("Arrived at Checkout and it's available"));
        }
        else
        {
            ShowFloatingText("This Register is taken!", true);
            DebugLog(TEXT("Arrived at Checkout but it's already taken"));
            CurrentCheckout = nullptr;
            GetWorldTimerManager().SetTimer(FindCheckoutTimerHandle, this, &ACashierAI::FindAndMoveToCheckout, 5.0f, false);
        }
    }
    else
    {
        DebugLog(TEXT("HandleCheckoutArrival: No assigned Checkout"));
    }
}

void ACashierAI::UpdateRotation(float DeltaTime)
{
    ElapsedTime += DeltaTime;
    float Alpha = FMath::Clamp(ElapsedTime / RotationTime, 0.0f, 1.0f);

    FRotator CurrentRotation = GetActorRotation();
    FRotator NewRotation = FMath::Lerp(CurrentRotation, TargetRotation, Alpha);
    NewRotation.Pitch = 0.0f;
    NewRotation.Roll = 0.0f;

    SetActorRotation(NewRotation);

    if (Alpha >= 1.0f)
    {
        bIsRotating = false;
        SetActorRotation(TargetRotation);
        HandleCheckoutArrival();
    }
}

void ACashierAI::TurnToFaceCheckout()
{
    if (!CurrentCheckout)
    {
        DebugLog(TEXT("Cannot turn to face checkout, CurrentCheckout is null"));
        return;
    }

    FVector AILocation = GetActorLocation();
    FVector CheckoutLocation = CurrentCheckout->GetActorLocation();
    TargetRotation = UKismetMathLibrary::FindLookAtRotation(AILocation, CheckoutLocation);

    // Only consider yaw rotation to avoid twitching on X and Y axes
    TargetRotation.Pitch = 0.0f;
    TargetRotation.Roll = 0.0f;

    // Calculate the time needed for rotation (assuming 180 degrees per second)
    FRotator CurrentRotation = GetActorRotation();
    float DeltaYaw = FMath::FindDeltaAngleDegrees(CurrentRotation.Yaw, TargetRotation.Yaw);
    RotationTime = FMath::Abs(DeltaYaw) / 180.0f;
    ElapsedTime = 0.0f;

    bIsRotating = true;
}

FVector ACashierAI::FindMostAccessiblePoint(const TArray<FVector>& Points)
{
    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!NavSys)
    {
        DebugLog(TEXT("Navigation system not found in FindMostAccessiblePoint"));
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

void ACashierAI::MoveTo(const FVector& Location)
{
    if (AIController)
    {
        UAIBlueprintHelperLibrary::SimpleMoveToLocation(AIController, Location);
    }
    else
    {
        DebugLog(TEXT("AIController is null in ACashierAI::MoveTo"));
        InitializeAIController();
        if (AIController)
        {
            UAIBlueprintHelperLibrary::SimpleMoveToLocation(AIController, Location);
        }
    }
}