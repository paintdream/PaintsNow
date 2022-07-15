// RenderFlowGraph.h
// PaintDream (paintdream@paintdream.com)
// 2019-11-1
//

#pragma once
#include "IWidget.h"
#include "../../../../Core/Interface/IArchive.h"
#include "../../../../Core/System/Kernel.h"
#include "../../../../Core/Template/TAllocator.h"
#include "../../../../Utility/SnowyStream/Resource/TextureResource.h"
#include "Core/ImGuiNodeEditor/imgui_node_editor.h"
#include "Core/ImGuiNodeEditor/BluePrint/Math2D.h"
#include "Core/ImGuiNodeEditor/BluePrint/Builders.h"
#include "Core/ImGuiNodeEditor/BluePrint/Widgets.h"

namespace PaintsNow {
	namespace ed = ax::NodeEditor;
	namespace util = ax::NodeEditor::Utilities;

	class RenderFlowGraph : public TReflected<RenderFlowGraph, WarpTiny>, public IWidget {
	public:
		RenderFlowGraph();
		~RenderFlowGraph() override;
		void TickRender(LeavesFlute& leavesFlute) override;
		void LeaveMainLoop() override;

	protected:
		TShared<TextureResource> textureHeaderBackground;
		TShared<TextureResource> textureSaveIcon;
		TShared<TextureResource> textureRestoreIcon;

		enum class PinType {
			Flow,
			Bool,
			Int,
			Float,
			String,
			Object,
			Function,
			Delegate,
		};

		enum class PinKind {
			Output,
			Input
		};

		enum class NodeType {
			Blueprint,
			Simple,
			Tree,
			Comment
		};

		struct GraphNode;
		struct Pin {
			ed::PinId ID;
			GraphNode* Node;
			std::string Name;
			PinType Type;
			PinKind Kind;

			Pin(int id, const char* name, PinType type) :
				ID(id), Node(nullptr), Name(name), Type(type), Kind(PinKind::Input) {}
		};

		struct GraphNode {
			ed::NodeId ID;
			std::string Name;
			std::vector<Pin> Inputs;
			std::vector<Pin> Outputs;
			ImColor Color;
			NodeType Type;
			ImVec2 Size;

			std::string State;
			std::string SavedState;

			GraphNode(int id, const char* name, ImColor color = ImColor(255, 255, 255)) :
				ID(id), Name(name), Color(color), Type(NodeType::Blueprint), Size(0, 0) {}
		};

		struct Link {
			ed::LinkId ID;
			ed::PinId StartPinID;
			ed::PinId EndPinID;

			ImColor Color;

			Link(ed::LinkId id, ed::PinId startPinId, ed::PinId endPinId) :
				ID(id), StartPinID(startPinId), EndPinID(endPinId), Color(255, 255, 255) {}
		};

		GraphNode* FindNode(ed::NodeId id);
		Link* FindLink(ed::LinkId id);
		Pin* FindPin(ed::PinId id);
		bool IsPinLinked(ed::PinId id);
		bool CanCreateLink(Pin* a, Pin* b);
		void BuildNode(GraphNode* node);
		GraphNode* SpawnInputActionNode();
		GraphNode* SpawnBranchNode();
		GraphNode* SpawnDoNNode();
		GraphNode* SpawnOutputActionNode();
		GraphNode* SpawnPrintStringNode();
		GraphNode* SpawnMessageNode();
		GraphNode* SpawnSetTimerNode();
		GraphNode* SpawnLessNode();
		GraphNode* SpawnWeirdNode();
		GraphNode* SpawnTraceByChannelNode();
		GraphNode* SpawnTreeSequenceNode();
		GraphNode* SpawnTreeTaskNode();
		GraphNode* SpawnTreeTask2Node();
		GraphNode* SpawnComment();
		void BuildNodes();
		void InitializeGraph();
		ImColor GetIconColor(PinType type);
		void DrawPinIcon(const Pin& pin, bool connected, int alpha);
		void ShowStyleEditor(bool* show = nullptr);
		void ShowLeftPane(float paneWidth);
		void ShowFrame();

		ed::EditorContext* editor;
		int	pinIconSize;
		std::vector<GraphNode> nodes;
		std::vector<Link> links;
	};
}

