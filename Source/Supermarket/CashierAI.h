// CashierAI.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "EnvironmentQuery/EnvQueryInstanceBlueprintWrapper.h"
#include "CashierAI.generated.h"

class ACheckout;
class UWidgetComponent;
class UTextBlock;

UCLASS()
class SUPERMARKET_API ACashierAI : public ACharacter
{
    GENERATED_BODY()

public:
    ACashierAI();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;


    UFUNCTION(BlueprintCallable, Category = "Cashier")
    void LeaveCheckout();

    UFUNCTION(BlueprintCallable, Category = "Cashier")
    float GetProcessingDelay() const { return ProcessingDelay; }

    UFUNCTION(BlueprintCallable, Category = "Cashier")
    float GetInterpSpeed() const { return InterpSpeed; }
    UFUNCTION(BlueprintCallable, Category = "Cashier")
    void FindAndMoveToCheckout();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cashier")
    UWidgetComponent* TextBoxWidget;

    UPROPERTY(EditDefaultsOnly, Category = "Cashier")
    TSubclassOf<UUserWidget> TextBoxWidgetClass;
    static TArray<ACheckout*> OccupiedCheckouts;
    static FCriticalSection OccupiedCheckoutsLock;

    UPROPERTY(EditDefaultsOnly, Category = "AI")
    UEnvQuery* FindPathQuery;

    UFUNCTION()
    void OnFindPathQueryFinished(UEnvQueryInstanceBlueprintWrapper* QueryInstance, EEnvQueryStatus::Type QueryStatus);
protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cashier")
    float ProcessingDelay;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cashier")
    float InterpSpeed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cashier")
    bool bDebugMode;

private:
    UPROPERTY()
    ACheckout* AssignedCheckout;

    void MoveToCheckout(ACheckout* Checkout);

    FTimerHandle CheckPositionTimerHandle;
    void CheckPosition();

    void DebugLog(const FString& Message);

    UPROPERTY()
    class AAIController* AIController;

    void InitializeAIController();
    UPROPERTY()
    ACheckout* CurrentCheckout;
    bool IsCheckoutAvailable(ACheckout* Checkout) const;
    void ShowFloatingText(const FString& Message, bool bShow);
    FTimerHandle FindCheckoutTimerHandle;
    void HandleCheckoutArrival();
    void MoveTo(const FVector& Location);
    FVector FindMostAccessiblePoint(const TArray<FVector>& Points);
    void TurnToFaceCheckout();
    void UpdateRotation(float DeltaTime);
    FRotator TargetRotation;
    float RotationTime;
    float ElapsedTime;
    bool bIsRotating;
    

    static ACheckout* FindAvailableCheckout(UWorld* World);
    void ClaimCheckout(ACheckout* Checkout);
    void ReleaseCheckout(ACheckout* Checkout);
    UPROPERTY()
    FTimerHandle MovementCheckTimerHandle;
    void CheckMovement();
};