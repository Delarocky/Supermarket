// CashierAI.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CashierAI.generated.h"

class ACheckout;

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
};