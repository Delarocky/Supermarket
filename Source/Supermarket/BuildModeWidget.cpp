#include "BuildModeWidget.h"
#include "Components/TextBlock.h"

void UBuildModeWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (KeybindsText)
    {
        KeybindsText->SetText(FText::FromString(
            "Build Mode Keybinds:\n"
            "WASD - Move Camera\n"
            "Right Mouse Button + Mouse Move - Rotate Camera\n"
            "B - Exit Build Mode"
        ));
    }
}