// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "LeavesExporterPCH.h"
#include "LeavesExporter.h"
#include "../../../../../Source/Utility/SnowyStream/ResourceBase.h"

namespace PaintsNow {
	class Service;
	class Entity;
}

class SLeavesWidget : public SCompoundWidget, public ISceneExplorer
{
public:
	SLATE_BEGIN_ARGS(SLeavesWidget) {}
	SLATE_ARGUMENT(TSharedPtr<PaintsNow::Service>, service);
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);
	FReply OnConnectClicked();
	FReply OnSyncClicked();

private:
	TSharedPtr<SEditableText> tunnelIdentifier;

protected:
	virtual void WriteLog(LOG_LEVEL logLevel, const std::string& content);

	void CollectSceneEntities();
	void CollectSceneEntity(AActor& actor);

	template <class T>
	void ExportActorComponentsByClass(AActor& actor, void (SLeavesWidget::*exportProc)(T* component)) {
		TArray<UActorComponent*> components = actor.GetComponentsByClass(typename T::StaticClass());
		for (int32 i = 0; i < components.Num(); i++) {
			T* component = static_cast<T*>(components[i]);
			(this->*exportProc)(component);
		}
	}

	void OnExportMeshComponent(UMeshComponent* meshComponent);
	void OnExportLandscapeComponent(ULandscapeComponent* landspaceComponent);
	void OnExportSphereReflectionComponent(UReflectionCaptureComponent* sphereReflectionComponent);
	void OnExportExpressionInput(const UMaterialExpression* expression);
	template <class T>
	void OnExportTextureResource(UTexture* texture, T op);

	TMap<UObject*, PaintsNow::TShared<PaintsNow::ResourceBase> > collectedObjects;
	TMap<UStaticMesh*, uint32> mapModelComponent;
	uint32 componentCount;
	uint32 entityCount;
	uint32 entityGroupCount;
	TSharedPtr<PaintsNow::Service> service;
};