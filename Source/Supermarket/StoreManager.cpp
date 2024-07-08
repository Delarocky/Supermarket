// StoreManager.cpp
#include "StoreManager.h"

AStoreManager* AStoreManager::Instance = nullptr;

AStoreManager::AStoreManager()
{
    PrimaryActorTick.bCanEverTick = false;
    bStoreOpen = true;
}

void AStoreManager::SetStoreOpen(bool bIsOpen)
{
    if (bStoreOpen != bIsOpen)
    {
        bStoreOpen = bIsOpen;
        OnStoreStatusChanged.Broadcast(bStoreOpen);
    }
}

AStoreManager* AStoreManager::GetInstance(UWorld* World)
{
    if (!Instance && World)
    {
        Instance = World->SpawnActor<AStoreManager>(AStoreManager::StaticClass());
    }
    return Instance;
}

