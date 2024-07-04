// MoneyDisplayWidget.cpp
#include "MoneyDisplayWidget.h"
#include "Components/TextBlock.h"
#include "SupermarketGameState.h"
#include "Kismet/GameplayStatics.h"

void UMoneyDisplayWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // Set up a timer to update the display periodically
    GetWorld()->GetTimerManager().SetTimer(UpdateTimerHandle, this, &UMoneyDisplayWidget::UpdateMoneyDisplayFromGameState, 0.1f, true);
}

void UMoneyDisplayWidget::UpdateMoneyDisplay(float NewAmount)
{
    if (MoneyText)
    {
        FString MoneyString = FString::Printf(TEXT("Money: $%.2f"), NewAmount);
        MoneyText->SetText(FText::FromString(MoneyString));
    }
}

void UMoneyDisplayWidget::UpdateMoneyDisplayFromGameState()
{
    if (ASupermarketGameState* GameState = GetWorld()->GetGameState<ASupermarketGameState>())
    {
        UpdateMoneyDisplay(GameState->GetTotalMoney());
    }
}