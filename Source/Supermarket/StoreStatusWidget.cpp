// StoreStatusWidget.cpp
#include "StoreStatusWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "StoreManager.h"

void UStoreStatusWidget::NativeConstruct()
{
    Super::NativeConstruct();

   

    StoreManager = AStoreManager::GetInstance(GetWorld());
    if (StoreManager)
    {
        SetStoreOpen(StoreManager->IsStoreOpen());
    }
}

void UStoreStatusWidget::SetStoreOpen(bool bIsOpen)
{
    if (StatusText)
    {
        StatusText->SetText(FText::FromString(bIsOpen ? "OPEN" : "CLOSED"));
    }

    if (StoreManager)
    {
        StoreManager->SetStoreOpen(bIsOpen);
    }
}

bool UStoreStatusWidget::IsStoreOpen() const
{
    return StoreManager ? StoreManager->IsStoreOpen() : false;
}

void UStoreStatusWidget::ToggleStoreStatus()
{
    SetStoreOpen(!IsStoreOpen());
}

void UStoreStatusWidget::OnToggleButtonClicked()
{
    SetStoreOpen(!IsStoreOpen());
}