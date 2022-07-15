#include "Repository.h"
#include "../../../../Utility/LeavesFlute/LeavesFlute.h"
#include "Core/imgui.h"

using namespace PaintsNow;

Repository::Repository() {
	critical.store(0, std::memory_order_relaxed);
	itemAllocator = TShared<Item::Allocator>::From(new Item::Allocator());
	rootItem = TShared<Item>::From(itemAllocator->New());
}

void Repository::RefreshLocalFilesRecursive(IArchive& archive, const String& p, Item& item) {
	// TWrapper supports lambdas in synchronized context
	String path = p;
	if (!path.empty() && path[path.size() - 1] != '/') path += '/';

	archive.Query(path, WrapClosure([&](const String& name) {
		TShared<Item> subItem = TShared<Item>::From(itemAllocator->New());
		subItem->name = name;
		subItem->parent = &item;

		if (name[name.size() - 1] == '/') {
			RefreshLocalFilesRecursive(archive, path + name, *subItem);
			subItem->Flag().fetch_or(Item::ITEM_DIRECTORY, std::memory_order_relaxed);
		}

		item.children.emplace_back(std::move(subItem));
	}));
}

void Repository::RefreshLocalFiles(IArchive& archive) {
	TShared<Item> newRootItem = TShared<Item>::From(itemAllocator->New());
	newRootItem->Flag().fetch_or(Item::ITEM_DIRECTORY, std::memory_order_relaxed);
	RefreshLocalFilesRecursive(archive, "", *newRootItem);

	SpinLock(critical);
	std::swap(rootItem, newRootItem);
	SpinUnLock(critical);
}

void Repository::RenderItem(Item& item, uint32_t depth) {
	if (item.Flag().load(std::memory_order_acquire) & Item::ITEM_DIRECTORY) {
		if (ImGui::TreeNode(item.name.empty() ? "/" : item.name.c_str())) {
			for (size_t i = 0; i < item.children.size(); i++) {
				RenderItem(*item.children[i], depth + 1);
			}

			ImGui::TreePop();
		}
	} else {
		ImGui::Text("%s", item.name.c_str());
		if (ImGui::IsItemClicked()) {
			if (strncmp(item.name.c_str() + item.name.size() - 4, ".pod", 4) == 0) {
				// collect path
				String path = item.name.substr(0, item.name.size() - 4);
				for (Item* p = item.parent; p != nullptr; p = p->parent) {
					path = p->name + "/" + path;
				}

				viewResourcePath = path;
			}
		}
	}
}

void Repository::TickRender(LeavesFlute& leavesFlute) {
	ImGui::SetNextWindowPos(ImVec2(0, 480), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(730, 320), ImGuiCond_FirstUseEver);

	if (ImGui::Begin("Repository", &show)) {
		// Search from archive if needed
		if (ImGui::Button("Refresh Repository")) {
			leavesFlute.GetKernel().QueueRoutine(this, CreateTaskContextFree(Wrap(this, &Repository::RefreshLocalFiles), std::ref(leavesFlute.GetInterfaces().archive)));
		}

		TShared<Item> item;
		SpinLock(critical);
		std::swap(rootItem, item);
		SpinUnLock(critical);

		ImGui::BeginChild("Local Files:", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5f, 260));
		RenderItem(*item, 0);
		ImGui::EndChild();
		ImGui::SameLine();
		ImGui::BeginChild("Details:", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5f, 260));
		if (!viewResourcePath.empty()) {
			if (!viewResource || viewResource->GetLocation() != viewResourcePath) {
				viewResource = leavesFlute.snowyStream.CreateResource(viewResourcePath);
			}

			RenderObject(*viewResource);
		}
		ImGui::EndChild();

		SpinLock(critical);
		if (rootItem == nullptr) {
			std::swap(rootItem, item);
		}
		SpinUnLock(critical);
	}

	ImGui::End();
}