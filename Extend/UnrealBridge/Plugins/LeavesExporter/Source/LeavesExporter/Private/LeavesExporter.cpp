// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
#include "LeavesExporterPCH.h"
#include "ILeavesExporter.h"
#include "LeavesExporter.h"
#include "CoreUObject.h"
#include "SWindow.h"
#include "SDockTab.h"
#include "LeavesWidget.h"
#include "Service.h"

#define LOCTEXT_NAMESPACE "FLeavesExporter"

IMPLEMENT_MODULE(FLeavesExporter, LeavesExporter)

// Just follow the instructions here: https://answers.unrealengine.com/questions/25609/customizing-the-editors-toolbar-buttons-menu-via-c.html

// static PaintsNow::Service service;
static const FName LeavesTabName("LeavesTab");

void FLeavesExporter::StartupModule() {
	UE_LOG(LogStats, Log, TEXT("Start LeavesExporter "));
	TSharedRef<class FGlobalTabmanager> tm = FGlobalTabmanager::Get();

	FLeavesCommands::Register();

	leavesCommands = MakeShareable(new FUICommandList);

	leavesCommands->MapAction(
		FLeavesCommands::Get().button,
		FExecuteAction::CreateRaw(this, &FLeavesExporter::OnButtonClicked),
		FCanExecuteAction());

	toolbarExtender = MakeShareable(new FExtender);
	toolbarExtension = toolbarExtender->AddToolBarExtension("Settings", EExtensionHook::After, leavesCommands, FToolBarExtensionDelegate::CreateRaw(this, &FLeavesExporter::AddToolbarExtension));

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(toolbarExtender);

	extensionManager = LevelEditorModule.GetToolBarExtensibilityManager();

	// TSharedRef<FWorkspaceItem> cat = tm->AddLocalWorkspaceMenuCategory(FText::FromString(TEXT("LeavesExporter")));
	// auto WorkspaceMenuCategoryRef = cat.ToSharedRef();

	/*
	tm->RegisterTabSpawner(LeavesTabName, FOnSpawnTab::CreateRaw(this, &FLeavesExporter::SpawnTab))
		.SetDisplayName(FText::FromString(TEXT("Leaves Exporter")));*/

	tm->RegisterNomadTabSpawner(LeavesTabName, FOnSpawnTab::CreateRaw(this, &FLeavesExporter::SpawnTab))
		.SetDisplayName(FText::FromString(TEXT("Leaves Exporter")))
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.ViewOptions.Small"));

	service = MakeShared<PaintsNow::Service>();
}

void FLeavesExporter::OnButtonClicked() {
	FGlobalTabmanager::Get()->TryInvokeTab(LeavesTabName);
	// Trigger scene collection
	// CollectSceneEntities();
}


TSharedRef<SDockTab> FLeavesExporter::SpawnTab(const FSpawnTabArgs& TabSpawnArgs) {
	auto widget = SNew(SLeavesWidget).service(service);
	return SNew(SDockTab)
		.Icon(FEditorStyle::GetBrush("LevelEditor.ViewOptions.Small"))
		.TabRole(ETabRole::NomadTab)
		.Label(NSLOCTEXT("LeavesExporter", "Expoter", "Control Panel"))
		[
			widget
		];
}

void FLeavesExporter::AddToolbarExtension(FToolBarBuilder &builder) {
	UE_LOG(LogStats, Log, TEXT("Starting Extension logic"));

	FSlateIcon IconBrush = FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.ViewOptions", "LevelEditor.ViewOptions.Small");

	builder.AddToolBarButton(FLeavesCommands::Get().button, NAME_None, LOCTEXT("LeavesCommands", "Leaves Exporter"), LOCTEXT("LeavesCommandsOverride", "Click to toggle LeavesExporter"), IconBrush, NAME_None);
}

void FLeavesExporter::ShutdownModule() {
	if (extensionManager.IsValid()) {
		FLeavesCommands::Unregister();

		TSharedRef<class FGlobalTabmanager> tm = FGlobalTabmanager::Get();
		tm->UnregisterNomadTabSpawner(LeavesTabName);

		toolbarExtender->RemoveExtension(toolbarExtension.ToSharedRef());
		extensionManager->RemoveExtender(toolbarExtender);
	} else {
		extensionManager.Reset();
	}

	service->Uninitialize();
}
