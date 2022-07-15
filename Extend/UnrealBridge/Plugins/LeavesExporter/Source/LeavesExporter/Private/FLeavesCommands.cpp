#include "LeavesExporterPCH.h"

#include "FLeavesCommands.h"

#define LOCTEXT_NAMESPACE "FLeavesExporter"
 
PRAGMA_DISABLE_OPTIMIZATION
void FLeavesCommands::RegisterCommands()
{
	UI_COMMAND(button, "Leaves Exporter", "Export scene to PaintsNow!", EUserInterfaceActionType::Button, FInputGesture());
}
PRAGMA_ENABLE_OPTIMIZATION
