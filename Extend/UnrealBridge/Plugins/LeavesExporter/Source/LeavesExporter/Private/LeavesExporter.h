#pragma once

namespace PaintsNow {
	class Service;
}

class FLeavesExporter : public ILeavesExporter
{
public:
	TSharedPtr<SViewport> Viewport;
	TSharedRef<SDockTab> SpawnTab(const FSpawnTabArgs& TabSpawnArgs);
	void OnButtonClicked();

protected:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	void AddToolbarExtension(FToolBarBuilder &);

	TSharedPtr<FUICommandList> leavesCommands;
	TSharedPtr<FExtensibilityManager> extensionManager;
	TSharedPtr<const FExtensionBase> toolbarExtension;
	TSharedPtr<FExtender> toolbarExtender;
	TSharedPtr<PaintsNow::Service> service;
};
