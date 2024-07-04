// SupermarketGameState.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "SupermarketGameState.generated.h"

UCLASS()
class SUPERMARKET_API ASupermarketGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    ASupermarketGameState();

    UFUNCTION(BlueprintCallable, Category = "Money")
    float GetTotalMoney() const { return TotalMoney; }

    UFUNCTION(BlueprintCallable, Category = "Money")
    void AddMoney(float Amount);

    UPROPERTY(ReplicatedUsing = OnRep_TotalMoney)
    float TotalMoney;

    UFUNCTION()
    void OnRep_TotalMoney();

protected:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};