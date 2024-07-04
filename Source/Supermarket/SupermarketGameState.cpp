// SupermarketGameState.cpp
#include "SupermarketGameState.h"
#include "Net/UnrealNetwork.h"

ASupermarketGameState::ASupermarketGameState()
{
    TotalMoney = 0.0f;
}

void ASupermarketGameState::AddMoney(float Amount)
{
    TotalMoney += Amount;
    OnRep_TotalMoney();
}

void ASupermarketGameState::OnRep_TotalMoney()
{
    // This function will be called on clients when TotalMoney is updated
    // You can add any additional logic here if needed
}

void ASupermarketGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ASupermarketGameState, TotalMoney);
}