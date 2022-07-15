#pragma once

#include "SlateBasics.h"
#include "Commands.h"
#include "EditorStyle.h"

class FLeavesCommands : public TCommands<FLeavesCommands>
{
public:

	FLeavesCommands()
		: TCommands<FLeavesCommands>(TEXT("LeavesExporter"), NSLOCTEXT("Contexts", "Leaves", "Leaves Exporter"), NAME_None, FEditorStyle::GetStyleSetName())
	{
	}

	virtual void RegisterCommands() override;

	TSharedPtr<FUICommandInfo> button;

};