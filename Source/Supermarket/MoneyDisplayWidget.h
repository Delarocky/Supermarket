// MoneyDisplayWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MoneyDisplayWidget.generated.h"

UCLASS()
class SUPERMARKET_API UMoneyDisplayWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Money Display")
    void UpdateMoneyDisplay(float NewAmount);

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* MoneyText;

private:
    void UpdateMoneyDisplayFromGameState();
    FTimerHandle UpdateTimerHandle;
};