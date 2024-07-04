// CashierAI.cpp
#include "CashierAI.h"
#include "Checkout.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/AIBlueprintHelperLibrary.h" // Add this include

ACashierAI::ACashierAI()
{
    PrimaryActorTick.bCanEverTick = true;

    ProcessingDelay = 0.5f;
    InterpSpeed = 300.0f;
    bDebugMode = true;
    CurrentCheckout = nullptr;
    AIController = nullptr;
}

void ACashierAI::BeginPlay()
{
    Super::BeginPlay();
    InitializeAIController();
    DebugLog(TEXT("CashierAI initialized"));
}

void ACashierAI::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!CurrentCheckout)
    {
        FindAndMoveToCheckout();
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

        // Use AIBlueprintHelperLibrary to move the AI
        UAIBlueprintHelperLibrary::SimpleMoveToLocation(AIController, CheckoutLocation);

        GetWorldTimerManager().SetTimer(CheckPositionTimerHandle, this, &ACashierAI::CheckPosition, 0.5f, true);
        DebugLog(FString::Printf(TEXT("Moving to Checkout at location: %s"), *CheckoutLocation.ToString()));
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
            CurrentCheckout->SetCashier(this);
            DebugLog(TEXT("Reached Checkout position"));
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

    for (AActor* Actor : FoundCheckouts)
    {
        ACheckout* Checkout = Cast<ACheckout>(Actor);
        if (Checkout && IsCheckoutAvailable(Checkout))
        {
            MoveToCheckout(Checkout);
            break;
        }
    }
}