// GLSLShaderGenerator.h
// PaintDream (paintdream@paintdream.com)
// 2020-8-19
//

#pragma once
#include "../../../Interface/IRender.h"
#include "../../../Interface/IShader.h"
#include "../../../Interface/IAsset.h"

namespace PaintsNow {
	class GLSLShaderGenerator : public IReflect {
	public:
		GLSLShaderGenerator(IRender::Resource::ShaderDescription::Stage s, uint32_t& pinputIndex, uint32_t& poutputIndex, uint32_t& pbindingIndex);

		static const String& GetFrameCode();
		static String FormatCode(const String& input);
		void Complete();
		void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) override;
		void Method(const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) override;

		IRender::Resource::ShaderDescription::Stage stage;
		bool forceLayout; // SPIRV requires force layout
		bool vulkanNDC; // Vulkan requires reverse-Y
		bool reserved[2];

		String declaration;
		String initialization;
		String finalization;

		uint32_t debugVertexBufferIndex;
		uint32_t propertyLevel;
		uint32_t& inputIndex;
		uint32_t& outputIndex;
		uint32_t& bindingIndex;

		template <class T>
		struct Binding {
			Binding() {}
			Binding(const T* p, uint32_t b, const String& n) : pointer(p), binding(b), name(n) {}

			const T* pointer;
			uint32_t binding;
			String name;
		};

		std::vector<Binding<IShader::BindBuffer> > bufferBindings;
		std::vector<Binding<IShader::BindTexture> > textureBindings;
		std::map<String, String> mapStructureDefinition;
		std::vector<String> structures;
		std::vector<IAsset::Material::Variable> constants;

	protected:
		std::map<const IShader::BindBuffer*, std::pair<String, String> > mapBufferDeclaration;
		std::map<const IShader::BindBuffer*, bool> mapBufferEnabled;
	};
}

