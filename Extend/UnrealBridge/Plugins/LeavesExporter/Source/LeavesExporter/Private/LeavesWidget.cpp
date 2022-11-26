// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "LeavesExporterPCH.h"
#include "LeavesWidget.h"
#include "Service.h"

// Resources
#include "../../../../../Source/Utility/MythForest/Entity.h"
#include "../../../../../Source/Utility/MythForest/Component/Form/FormComponent.h"
#include "../../../../../Source/Utility/SnowyStream/SnowyStream.h"
#include "../../../../../Source/Utility/SnowyStream/Resource/MeshResource.h"
#include "../../../../../Source/Utility/SnowyStream/Resource/MaterialResource.h"
#include "../../../../../Source/Utility/SnowyStream/Resource/TextureResource.h"
#include "../../../../../Source/Core/Driver/Filter/Pod/ZFilterPod.h"

#define LOCTEXT_NAMESPACE "SLeavesWidget"

void SLeavesWidget::Construct(const FArguments& InArgs) {
	UE_LOG(LogTemp, Log, TEXT("SLeavesWidget::Construct..."));

	service = InArgs._service;
	service->Initialize(this);
	tunnelIdentifier = SNew(SEditableText).Text(NSLOCTEXT("Tunnel", "127.0.0.1:16384", "127.0.0.1:16384"));

	auto MessagesTextBox = SNew(SMultiLineEditableTextBox)
		.Style(FEditorStyle::Get(), "Log.TextBox")
		.TextStyle(FEditorStyle::Get(), "Log.Normal")
		.ForegroundColor(FLinearColor::Gray)
		.IsReadOnly(true)
		.AlwaysShowScrollbars(true)
		// .OnVScrollBarUserScrolled(this, &SOutputLog::OnUserScrolled)
		// .ContextMenuExtender(this, &SOutputLog::ExtendTextBoxMenu)
		;

	ChildSlot
		[
			SNew(SBorder)
			.Padding(3)
		.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SVerticalBox)

			// Output Log Filter
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(0.0f, 0.0f, 0.0f, 4.0f))
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2, 5, 0, 0)
		[
			tunnelIdentifier->AsShared()
		]
	+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2, 0, 0, 0)
		[
			SNew(SButton)
			.ContentPadding(FMargin(6.0, 2.0))
		.Text(LOCTEXT("Connect to server", "Connect"))
		.OnClicked(this, &SLeavesWidget::OnConnectClicked)
		]
	+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2, 0, 0, 0)
		[
			SNew(SButton)
			.ContentPadding(FMargin(6.0, 2.0))
		.Text(LOCTEXT("Synchronize", "Synchronize"))
		.OnClicked(this, &SLeavesWidget::OnSyncClicked)
		]
	+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SComboButton)
			.ComboButtonStyle(FEditorStyle::Get(), "GenericFilters.ComboButtonStyle")
		.ForegroundColor(FLinearColor::White)
		.ContentPadding(0)
		.ToolTipText(LOCTEXT("AddFilterToolTip", "Add an output log filter."))
		// .OnGetMenuContent(this, &SOutputLog::MakeAddFilterMenu)
		.HasDownArrow(true)
		.ContentPadding(FMargin(1, 0))
		.ButtonContent()
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(STextBlock)
			.TextStyle(FEditorStyle::Get(), "GenericFilters.TextStyle")
		.Font(FEditorStyle::Get().GetFontStyle("FontAwesome.9"))
		.Text(FText::FromString(FString(TEXT("\xf0b0"))) /*fa-filter*/)
		]

	+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2, 0, 0, 0)
		[
			SNew(STextBlock)
			.TextStyle(FEditorStyle::Get(), "GenericFilters.TextStyle")
		.Text(LOCTEXT("Filters", "Filters"))
		]

		]
		]

	/*
				+SHorizontalBox::Slot()
				.Padding(4, 1, 0, 0)
				[
					SAssignNew(FilterTextBox, SSearchBox)
					.HintText(LOCTEXT("SearchLogHint", "Search Log"))
					.OnTextChanged(this, &SOutputLog::OnFilterTextChanged)
					.OnTextCommitted(this, &SOutputLog::OnFilterTextCommitted)
					.DelayChangeNotificationsWhileTyping(true)
				]*/
		]

	// Output log area
	+ SVerticalBox::Slot()
		.FillHeight(1)
		[
			MessagesTextBox
		]

	// The console input box
	/*
	+SVerticalBox::Slot()
	.AutoHeight()
	.Padding(FMargin(0.0f, 4.0f, 0.0f, 0.0f))
	[
		SNew(SBox)
		.MaxDesiredHeight(180.0f)
		[
			SNew(SConsoleInputBox)
			.OnConsoleCommandExecuted(this, &SOutputLog::OnConsoleCommandExecuted)

			// Always place suggestions above the input line for the output log widget
			.SuggestionListPlacement(MenuPlacement_AboveAnchor)
		]
	]*/
		]
		];
}

static void OnResourceComplete(PaintsNow::RemoteCall& remoteCall, PaintsNow::ProtoOutput&& outputPacket) {

}

static void OnComponentComplete(PaintsNow::RemoteCall& remoteCall, PaintsNow::ProtoOutput&& outputPacket) {

}

static void OnEntityComplete(PaintsNow::RemoteCall& remoteCall, PaintsNow::ProtoOutput&& outputPacket) {

}

static void OnAllComplete(PaintsNow::RemoteCall& remoteCall, PaintsNow::ProtoOutput&& outputPacket) {

}

static PaintsNow::String& ConvertPath(PaintsNow::String& path) {
	for (size_t i = 0; i < path.length(); i++) {
		if (path[i] == ':') {
			path[i] = '/';
		} else if (path[i] == '.') {
			path[i] = '_';
		}
	}

	if (path[0] == '/') {
		path = PaintsNow::String("/Weaver") + path;
	} else {
		path = PaintsNow::String("/Weaver/") + path;
	}

	return path;
}

static void PostResource(PaintsNow::Service& service, PaintsNow::ResourceBase& resource, const PaintsNow::String& location) {
	// PostResource
	using namespace PaintsNow;
	MemoryStream stream(409600, true);
	static ZFilterPod filter;
	IStreamBase* f = filter.CreateFilter(stream);
	*f << resource;
	f->Destroy();

	String extension = SnowyStream::GetReflectedExtension(resource.GetUnique());
	String data(reinterpret_cast<const char*>(stream.GetBuffer()), stream.GetTotalLength());

	ProtoInputPostResource inputPacket;
	inputPacket.location = location;
	inputPacket.extension = extension;
	inputPacket.resourceData = std::move(data);
	service.GetSession().Invoke("RpcPostResource", std::move(inputPacket), Wrap(OnResourceComplete));
}

static void CompleteSync(PaintsNow::Service& service) {
	using namespace PaintsNow;

	ProtoInputComplete inputPacket;
	service.GetSession().Invoke("RpcComplete", std::move(inputPacket), Wrap(OnAllComplete));
}

FReply SLeavesWidget::OnConnectClicked() {
	FString target = tunnelIdentifier->GetText().ToString();
	// CollectSceneEntities();
	service->Reconnect(TCHAR_TO_UTF8(*target));
	return FReply::Handled();
}

FReply SLeavesWidget::OnSyncClicked() {
	if (service->IsConnected()) {
		CollectSceneEntities();
	}

	return FReply::Handled();
}

void SLeavesWidget::CollectSceneEntities() {
	collectedObjects.Empty();
	componentCount = 0;
	entityCount = 0;
	entityGroupCount = 0;

	// Enumerate actors
	for (TObjectIterator<AActor> Itr; Itr; ++Itr) {
		AActor& actor = **Itr;
		CollectSceneEntity(actor);
	}

	CompleteSync(*service.Get());
}

// template <size_t n>
static inline PaintsNow::Float3 ConvertVector(const FVector& vec) {
	return PaintsNow::Float3(vec.X, vec.Y, vec.Z);
}

static inline PaintsNow::UChar4 EncodeNormal(const FVector4& vec) {
	FVector4 t = vec * 127.5f + FVector4(127.0f, 127.0f, 127.0f, 127.0f);
	return PaintsNow::UChar4(
		(uint8_t)FMath::Clamp(t.X, 0.0f, 255.0f),
		(uint8_t)FMath::Clamp(t.Y, 0.0f, 255.0f),
		(uint8_t)FMath::Clamp(t.Z, 0.0f, 255.0f),
		(uint8_t)FMath::Clamp(t.W, 0.0f, 255.0f));
}

void SLeavesWidget::CollectSceneEntity(AActor& actor) {
	using namespace PaintsNow;
	// Add necessary component(s)
	String actorName = TCHAR_TO_UTF8(*actor.GetName());
	/*
	FormComponent formComponent(actorName);
	entity.AddComponent(&formComponent);*/
	uint32 entityID = entityCount;
	const FTransform& transform = actor.GetTransform();

	{
		ProtoInputPostEntity inputPacket;
		inputPacket.entityID = entityID;
		inputPacket.groupID = 0;
		inputPacket.entityName = actorName;
		service->GetSession().Invoke("RpcPostEntity", std::move(inputPacket), Wrap(OnEntityComplete));
	}

	{
		ProtoInputPostTransformComponent inputPacket;
		inputPacket.componentID = componentCount;
		inputPacket.position = ConvertVector(transform.GetLocation() * FVector(-1, 1, 1) / 100.0f);
		inputPacket.scale = ConvertVector(transform.GetScale3D());
		inputPacket.rotation = ConvertVector(transform.GetRotation().Euler() / 180.0f * PI);
		service->GetSession().Invoke("RpcPostTransformComponent", std::move(inputPacket), Wrap(OnComponentComplete));
	}

	{
		ProtoInputPostEntityComponent inputPacket;
		inputPacket.entityID = entityID;
		inputPacket.componentID = componentCount;
		service->GetSession().Invoke("RpcPostEntityComponent", std::move(inputPacket), Wrap(OnComponentComplete));
	}

	service->GetSession().Flush();
	componentCount++;

	// Enumerate all components and export them
	ExportActorComponentsByClass(actor, &SLeavesWidget::OnExportMeshComponent);
	ExportActorComponentsByClass(actor, &SLeavesWidget::OnExportLandscapeComponent);
	ExportActorComponentsByClass(actor, &SLeavesWidget::OnExportSphereReflectionComponent);

	// Export entity attributes ...

	// Resolve grouping information ...

	// Resolve prefab information ...
	// CompleteSync(*service.Get());
	entityCount++;
}

static inline FVector ToVector3(const FVector4& vec) {
	return FVector(vec.X, vec.Y, vec.Z);
}

static void DefaultPixel(uint8_t* pixel) {}
static void SRMHToMixture(uint8_t* pixel) {
	std::swap(pixel[0], pixel[3]);
}

static uint32_t ConvertAddress(TextureAddress address) {
	switch (address) {
	case TextureAddress::TA_Wrap:
		return PaintsNow::IRender::Resource::TextureDescription::REPEAT;
	case TextureAddress::TA_Mirror:
		return PaintsNow::IRender::Resource::TextureDescription::MIRROR_REPEAT;
	case TextureAddress::TA_Clamp:
		return PaintsNow::IRender::Resource::TextureDescription::CLAMP;
	}

	return PaintsNow::IRender::Resource::TextureDescription::REPEAT;
}

template <class T>
void SLeavesWidget::OnExportTextureResource(UTexture* t, T op) {
	using namespace PaintsNow;

	if (!t->IsA(UTexture2D::StaticClass())) {
		return;
	}

	UTexture2D* texture = static_cast<UTexture2D*>(t);
	if (!collectedObjects.Contains(texture)) {
		FTextureSource& res = texture->Source;
		String textureName = TCHAR_TO_UTF8(*texture->GetPathName());
		ConvertPath(textureName);
		int32 bpp = res.GetBytesPerPixel();
		int32 groupCount = res.GetNumSlices();
		int32 mipCount = res.GetNumMips();
		auto& resourceManager = service->GetResourceManager();
		TShared<TextureResource> textureResource = TShared<TextureResource>::From(new TextureResource(resourceManager, textureName));
		IRender::Resource::TextureDescription::State& state = textureResource->description.state;
		collectedObjects.Add(texture, textureResource);

		state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
		switch (res.GetFormat()) {
		case TSF_G8:
			state.layout = IRender::Resource::TextureDescription::R;
			state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
			break;
		case TSF_BGRA8:
		case TSF_BGRE8:
			state.layout = IRender::Resource::TextureDescription::RGBA;
			state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
			break;
		case TSF_RGBA16:
			state.layout = IRender::Resource::TextureDescription::RGBA;
			state.format = IRender::Resource::TextureDescription::UNSIGNED_SHORT;
			break;
		case TSF_RGBA16F:
			state.layout = IRender::Resource::TextureDescription::RGBA;
			state.format = IRender::Resource::TextureDescription::HALF;
			break;
		}

		state.addressU = ConvertAddress(texture->AddressX);
		state.addressV = ConvertAddress(texture->AddressY);
		state.addressW = ConvertAddress(texture->AddressX);

		switch (texture->Filter) {
		case TextureFilter::TF_Nearest:
			state.sample = IRender::Resource::TextureDescription::POINT;
			break;
		case TextureFilter::TF_Default:
		case TextureFilter::TF_Bilinear:
			state.sample = IRender::Resource::TextureDescription::LINEAR;
			break;
		case TextureFilter::TF_Trilinear:
			state.sample = IRender::Resource::TextureDescription::TRILINEAR;
			break;
		}

		switch (texture->MipGenSettings) {
		case TMGS_NoMipmaps:
			state.mip = IRender::Resource::TextureDescription::NOMIP;
			break;
		case TMGS_LeaveExistingMips:
			state.mip = IRender::Resource::TextureDescription::SPECMIP;
			break;
		default:
			state.mip = IRender::Resource::TextureDescription::AUTOMIP;
			break;
		}

		state.compress = 0;

		textureResource->description.dimension.x() = res.GetSizeX();
		textureResource->description.dimension.y() = res.GetSizeY();
		textureResource->description.dimension.z() = groupCount;

		// Build buffers ...
		TArray64<uint8> buffer;

		for (int32 i = 0; i < mipCount; i++) {
			TArray64<uint8> data;
			if (res.GetMipData(data, i)) {
				// Convert to RGBA8
				if (res.GetFormat() == TSF_BGRA8) {
					for (int32 n = 0; n < data.Num(); n += 4) {
						std::swap(data[n], data[n + 2]);
						op(&data[n]);
					}
				}
				buffer.Append(data);
			} else {
				return; // Error
			}
		}

		textureResource->description.data.Assign(buffer.GetData(), (uint32_t)buffer.Num());
		PostResource(*service.Get(), *textureResource, textureName);
	}
}

void SLeavesWidget::OnExportExpressionInput(const UMaterialExpression* expression) {
	if (expression->IsA(UMaterialExpressionTextureBase::StaticClass())) {
		auto textureExpression = static_cast<const UMaterialExpressionTextureBase*>(expression);
		auto samplerType = textureExpression->SamplerType;
		auto texture = textureExpression->Texture;
		OnExportTextureResource(texture, DefaultPixel);
	}
}

inline bool EndsWith(const PaintsNow::String& s, const PaintsNow::String& p) {
	return s.size() < p.size() ? false : s.compare(s.size() - p.size(), p.size(), p) == 0;
}

void SLeavesWidget::OnExportMeshComponent(UMeshComponent* meshComponent) {
	using namespace PaintsNow;

	if (meshComponent->IsA(UStaticMeshComponent::StaticClass())) {
		auto staticMeshComponent = static_cast<UStaticMeshComponent*>(meshComponent);

		String modelName = TCHAR_TO_UTF8(*staticMeshComponent->GetPathName());
		ConvertPath(modelName);
		auto& resourceManager = service->GetResourceManager();
		uint32 modelComponentID = componentCount++;

		std::vector<TShared<MaterialResource> > materialResources;

		auto materials = staticMeshComponent->GetMaterials();
		for (int32 i = 0; i < materials.Num(); i++) {
			auto materialInterface = materials[i];
			if (materialInterface == nullptr) continue;

			String materialName = TCHAR_TO_UTF8(*materialInterface->GetPathName());
			ConvertPath(materialName);
			TShared<MaterialResource> materialResource = TShared<MaterialResource>::From(new MaterialResource(std::ref(resourceManager), materialName));
			if (!collectedObjects.Contains(materialInterface)) {
				collectedObjects.Add(materialInterface, materialResource);

				TMap<UTexture*, int32> mapTextureToID;
				auto AddTexture = [&](const String& name, UTexture* texture, int32 j) {
					// export default textures by postfix
					auto converter = DefaultPixel;
					String newName = name;
					if (EndsWith(name, "Normal")) {
						newName = "normalTexture";
					} else if (EndsWith(name, "Metallic")) {
						newName = "mixtureTexture";
					} else if (EndsWith(name, "BaseColor")) {
						newName = "baseColorTexture";
					} else if (EndsWith(name, "Diffuse")) {
						newName = "baseColorTexture";
					} else if (EndsWith(name, "SRMH")) {
						newName = "mixtureTexture";
						converter = SRMHToMixture;
					}

					OnExportTextureResource(texture, converter);
					TShared<TextureResource> res = TShared<TextureResource>::StaticCast(collectedObjects[texture]);
					materialResource->textureResources.emplace_back(res);
					mapTextureToID.Add(texture, j);
					materialResource->materialParams.variables.emplace_back(newName, IAsset::TextureIndex(j));
				};

				TArray<FMaterialParameterInfo> textureInfos;
				TArray<FGuid> paramIds;
				materialInterface->GetAllTextureParameterInfo(textureInfos, paramIds);

				// Fill material

				for (int32 n = 0, j = 0; n < textureInfos.Num(); n++) {
					auto& info = textureInfos[n];
					UTexture* texture = nullptr;
					if (materialInterface->GetTextureParameterValue(info, texture) && texture != nullptr) {
						String name = TCHAR_TO_UTF8(*info.Name.ToString());
						AddTexture(name, texture, j++);
					}
				}

				// Not an instanced material, try to parse texture from names
				if (textureInfos.Num() == 0) {
					TArray<UTexture*> usedTextures;
					// EMaterialQualityLevel::Type QualityLevel, bool bAllQualityLevels, ERHIFeatureLevel::Type FeatureLevel, bool bAllFeatureLevels) const override;
					materialInterface->GetUsedTextures(usedTextures, EMaterialQualityLevel::Type::Num, true, ERHIFeatureLevel::Type::ES3_1, true);

					for (int32 j = 0; j < usedTextures.Num(); j++) {
						UTexture* texture = usedTextures[j];
						String name = TCHAR_TO_UTF8(*texture->GetName());
						AddTexture(name, texture, j);
					}
				}

				TArray<FMaterialParameterInfo> vectorInfos;
				materialInterface->GetAllVectorParameterInfo(vectorInfos, paramIds);

				for (int32 k = 0; k < vectorInfos.Num(); k++) {
					auto& info = vectorInfos[k];
					IAsset::Material::Variable variable;
					FLinearColor value;
					if (materialInterface->GetVectorParameterValue(info, value)) {
						materialResource->materialParams.variables.emplace_back(TCHAR_TO_UTF8(*info.Name.GetPlainNameString()), Float4(value.R, value.G, value.B, value.A));
					}
				}

				TArray<FMaterialParameterInfo> scalarInfos;
				materialInterface->GetAllScalarParameterInfo(scalarInfos, paramIds);

				for (int32 k = 0; k < scalarInfos.Num(); k++) {
					auto& info = scalarInfos[k];
					IAsset::Material::Variable variable;
					float value;
					if (materialInterface->GetScalarParameterValue(info, value)) {
						materialResource->materialParams.variables.emplace_back(TCHAR_TO_UTF8(*info.Name.GetPlainNameString()), value);
					}
				}

				TArray<FMaterialParameterInfo> switchInfos;
				materialInterface->GetAllStaticSwitchParameterInfo(switchInfos, paramIds);
				for (int32 k = 0; k < switchInfos.Num(); k++) {
					auto& info = switchInfos[k];
					IAsset::Material::Variable variable;
					bool value;
					FGuid expression;
					if (materialInterface->GetStaticSwitchParameterValue(info, value, expression)) {
						materialResource->materialParams.variables.emplace_back(TCHAR_TO_UTF8(*info.Name.GetPlainNameString()), value);
					}
				}


				/*
				for (int32 n = 0; n < material->MaterialParameterCollectionInfos.Num(); n++) {
					auto param = material->MaterialParameterCollectionInfos[n].ParameterCollection;
				}*/

				// params in parent resource ..
				/*
				TArray<const UMaterialExpressionTextureBase*> textures;
				material->GetAllExpressionsOfType(textures);
				for (int32 k = 0; k < textures.Num(); k++) {
					OnExportExpressionInput(textures[k]);
				}*/

				// TODO: get source texture if exists
				TShared<ShaderResource> shaderResource = TShared<ShaderResource>::From(new ShaderResource(std::ref(resourceManager), ShaderResource::GetShaderPathPrefix() + "StandardPass"));

				materialResource->originalShaderResource = shaderResource;
				PostResource(*service.Get(), *materialResource, materialResource->GetLocation());
			}

			materialResources.emplace_back(materialResource);
		}

		// Post models
		auto staticMesh = staticMeshComponent->GetStaticMesh();
		if (staticMesh != nullptr) {
			if (!collectedObjects.Contains(staticMesh)) {
				collectedObjects.Add(staticMesh, nullptr);

				UE_LOG(LogStats, Display, TEXT("Collecting Mesh: %s\n"), *staticMesh->GetName());
				auto& lodResources = staticMesh->GetRenderData()->LODResources;

				// Create ModelComponent
				for (int32 i = 0; i < lodResources.Num(); i++) {
					uint32 modelComponentID = componentCount++;
					mapModelComponent.Add(staticMesh, modelComponentID);

					auto& mesh = lodResources[i];
					String meshName = TCHAR_TO_UTF8(*staticMesh->GetPathName());
					meshName += "_LOD_" + std::to_string(i);
					ConvertPath(meshName);

					TShared<MeshResource> meshResource = TShared<MeshResource>::From(new MeshResource(std::ref(resourceManager), meshName));
					auto meshCollection = meshResource->Inspect(UniqueType<IAsset::MeshCollection>());
					assert(meshCollection != nullptr);
					// Copy position buffer
					auto& vertexBuffers = mesh.VertexBuffers;
					auto& positionBuffer = vertexBuffers.PositionVertexBuffer;
					auto& targetPositionBuffer = meshCollection->vertices;
					targetPositionBuffer.resize(positionBuffer.GetNumVertices());
					for (uint32 j = 0; j < positionBuffer.GetNumVertices(); j++) {
						auto position = positionBuffer.VertexPosition(j);
						targetPositionBuffer[j] = Float3(-position.X, position.Y, position.Z) / 100.0f;
					}

					// Copy vertex attributes
					auto& staticMeshBuffer = vertexBuffers.StaticMeshVertexBuffer;
					auto& targetUVBuffer = meshCollection->texCoords;
					auto& targetNormalBuffer = meshCollection->normals;
					auto& targetTangentBuffer = meshCollection->tangents;

					targetNormalBuffer.resize(positionBuffer.GetNumVertices());
					targetTangentBuffer.resize(positionBuffer.GetNumVertices());
					targetUVBuffer.resize((staticMeshBuffer.GetNumTexCoords() + 1) >> 1);
					for (uint32 i = 0; i < targetUVBuffer.size(); i++) {
						targetUVBuffer[i].coords.resize(positionBuffer.GetNumVertices());
					}

					for (uint32 j = 0; j < positionBuffer.GetNumVertices(); j++) {
						FVector4 normal = staticMeshBuffer.VertexTangentZ(j);
						FVector4 tangent = staticMeshBuffer.VertexTangentX(j);
						FVector4 binormal = staticMeshBuffer.VertexTangentY(j);

						normal.X = -normal.X;
						tangent.X = -tangent.X;
						binormal.X = -binormal.X;

						// Custom ID
						tangent.W = FVector::DotProduct(ToVector3(normal),
							FVector::CrossProduct(ToVector3(binormal), ToVector3(tangent))) >= 0 ? 1.0f : 0.0f;

						binormal = FVector::CrossProduct(tangent, normal).GetSafeNormal();
						binormal.W = 0.0f;
						targetNormalBuffer[j] = EncodeNormal(binormal);
						targetTangentBuffer[j] = EncodeNormal(tangent);

						for (uint32 k = 0; k < staticMeshBuffer.GetNumTexCoords(); k++) {
							auto uv = staticMeshBuffer.GetVertexUV(j, k);
							auto& target = targetUVBuffer[k >> 1].coords[j];
							if ((k & 1) == 0) {
								target.x() = uv.X;
								target.y() = uv.Y;
							} else {
								target.z() = uv.X;
								target.w() = uv.Y;
							}
						}
					}

					// Copy indices
					auto indexBuffer = mesh.IndexBuffer.GetArrayView();
					auto& targetIndexBuffer = meshCollection->indices;
					targetIndexBuffer.resize(indexBuffer.Num() / 3);
					for (int32 j = 0; j < indexBuffer.Num() / 3; j++) {
						for (int32 m = 0; m < 3; m++) {
							auto index = indexBuffer[j * 3 + m];
							targetIndexBuffer[j][m] = index;
						}
					}

					// Copy vertex colors
					auto& colorBuffer = vertexBuffers.ColorVertexBuffer;
					auto& targetColorBuffer = meshCollection->colors;
					targetColorBuffer.resize(colorBuffer.GetNumVertices());
					for (uint32 j = 0; j < colorBuffer.GetNumVertices(); j++) {
						auto color = colorBuffer.VertexColor(j);
						targetColorBuffer[j] = UChar4(color.R, color.G, color.B, color.A);
					}


					// Groups
					auto& groupBuffer = meshCollection->groups;
					for (int32 k = 0; k < mesh.Sections.Num(); k++) {
						auto& section = mesh.Sections[k];
						// Redirect material
						uint32 mat = section.MaterialIndex;
						if (mat < materialResources.size()) {
							IAsset::MeshGroup group;
							assert(section.FirstIndex % 3 == 0);
							group.primitiveOffset = section.FirstIndex / 3;
							group.primitiveCount = section.NumTriangles;
							groupBuffer.emplace_back(std::move(group));
							auto materialResource = materialResources[mat];

							ProtoInputPostModelComponentMaterial inputPacket;
							inputPacket.componentID = modelComponentID;
							inputPacket.meshGroupID = k;
							inputPacket.materialResource = materialResource->GetLocation();
							service->GetSession().Invoke("RpcPostModelComponentMaterial", std::move(inputPacket), Wrap(OnComponentComplete));
						}
					}

					PostResource(*service, *meshResource, meshName);

					ProtoInputPostModelComponent inputPacket;
					inputPacket.componentID = modelComponentID;
					inputPacket.meshResource = meshResource->GetLocation();
					service->GetSession().Invoke("RpcPostModelComponent", std::move(inputPacket), Wrap(OnComponentComplete));

					UE_LOG(LogStats, Display, TEXT("LOD %d Vertices: %d, Primitives: %d, VertexColor: %s, UV Channels: %d\n"), i, positionBuffer.GetNumVertices(), indexBuffer.Num() / 3, colorBuffer.GetNumVertices() == 0 ? TEXT("False") : TEXT("True"), staticMeshBuffer.GetNumTexCoords());
				}
			}

			ProtoInputPostEntityComponent inputPacket;
			inputPacket.entityID = entityCount;
			inputPacket.componentID = mapModelComponent[staticMesh];
			service->GetSession().Invoke("RpcPostEntityComponent", std::move(inputPacket), Wrap(OnComponentComplete));
			service->GetSession().Flush();
		}
	}
}

void SLeavesWidget::OnExportLandscapeComponent(ULandscapeComponent* landspaceComponent) {}
void SLeavesWidget::OnExportSphereReflectionComponent(UReflectionCaptureComponent* sphereReflectionComponent) {
	using namespace PaintsNow;
	String textureName = TCHAR_TO_UTF8(*sphereReflectionComponent->GetName());
	ConvertPath(textureName);

	if (!collectedObjects.Contains(sphereReflectionComponent)) {
		collectedObjects.Add(sphereReflectionComponent);
		auto mapBuildData = sphereReflectionComponent->GetMapBuildData();
		if (mapBuildData == nullptr) {
			return;
		}
		auto& captureData = mapBuildData->FullHDRCapturedData;
		auto cubemapSize = mapBuildData->CubemapSize;
		auto& resourceManager = service->GetResourceManager();
		// Export Texture Data as TextureResource
		TShared<TextureResource> textureResource = TShared<TextureResource>::From(new TextureResource(resourceManager, textureName));
		IRender::Resource::TextureDescription::State& state = textureResource->description.state;
		state.compress = 0;
		state.format = IRender::Resource::TextureDescription::HALF;
		state.layout = IRender::Resource::TextureDescription::RGBA;
		state.mip = IRender::Resource::TextureDescription::SPECMIP;
		state.sample = IRender::Resource::TextureDescription::LINEAR;
		state.type = IRender::Resource::TextureDescription::TEXTURE_2D_CUBE;

		textureResource->description.dimension = UShort3(cubemapSize, cubemapSize, 1);

		Bytes& data = textureResource->description.data;
		data.Resize(captureData.Num());
		UE_LOG(LogStats, Display, TEXT("Collecting CubeMap: %d, size %p\n"), (int)cubemapSize, (void*)data.GetSize());
		// memcpy(const_cast<char*>(data.data()), captureData.GetData(), data.size());
		// UE4 stores cube data from mips to direction, while we need direction to mips ..
		assert(CubeFace_MAX == 6);
		// uint32 mipCount = FMath::CeilLogTwo(cubemapSize) + 1;
		uint8* target = data.GetData();
		uint32 pageSize = 0;
		for (uint32 i = cubemapSize; i > 0; i >>= 1) pageSize += i * i;
		pageSize *= sizeof(FFloat16Color);

		uint8* p = captureData.GetData();
		for (uint32 size = cubemapSize, offset = 0; size > 0; size >>= 1) {
			uint32 groupSize = size * size * sizeof(FFloat16Color);
			for (uint32 i = 0; i < CubeFace_MAX; i++) {
				uint32 k = i;
				memcpy(target + k * pageSize + offset, p, groupSize);
				p += groupSize;
			}

			offset += groupSize;
		}

		assert(p == captureData.GetData() + data.size());
		PostResource(*service.Get(), *textureResource, textureName);
	}

	{
		ProtoInputPostEnvCubeComponent inputPacket;
		inputPacket.componentID = componentCount;
		inputPacket.texturePath = textureName;
		service->GetSession().Invoke("RpcPostEnvCubeComponent", std::move(inputPacket), Wrap(OnComponentComplete));
	}

	{
		ProtoInputPostEntityComponent inputPacket;
		inputPacket.componentID = componentCount++;
		inputPacket.entityID = entityCount;
		service->GetSession().Invoke("RpcPostEntityComponent", std::move(inputPacket), Wrap(OnComponentComplete));
	}

	service->GetSession().Flush();
}

void SLeavesWidget::WriteLog(LOG_LEVEL logLevel, const std::string& content) {
	FString uniContent = UTF8_TO_TCHAR(content.c_str());
	switch (logLevel) {
	case LOG_TEXT:
		UE_LOG(LogStats, Display, TEXT("%s"), *uniContent);
		break;
	case LOG_WARNING:
		UE_LOG(LogStats, Warning, TEXT("%s"), *uniContent);
		break;
	case LOG_ERROR:
	default:
		UE_LOG(LogStats, Error, TEXT("%s"), *uniContent);
		break;
	}
}


#undef LOCTEXT_NAMESPACE