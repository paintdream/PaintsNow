// Repository.h
// PaintDream (paintdream@paintdream.com)
// 2019-11-1
//

#pragma once
#include "IWidget.h"
#include "../../../../Core/Interface/IArchive.h"
#include "../../../../Core/System/Kernel.h"
#include "../../../../Core/Template/TAllocator.h"

namespace PaintsNow {
	class ResourceBase;

	class Repository : public TReflected<Repository, WarpTiny>, public IWidget {
	public:
		Repository();
		void TickRender(LeavesFlute& leavesFlute) override;

		struct Item final : public TAllocatedTiny<Item, SharedTiny> {
			enum { ITEM_DIRECTORY = SharedTiny::TINY_CUSTOM_BEGIN };
			Item() : parent(nullptr) {}
			Item* parent;
			String name;
			TShared<ResourceBase> resource;
			std::vector<TShared<Item> > children;
		};

		void RenderItem(Item& item, uint32_t depth);
		void RefreshLocalFilesRecursive(IArchive& archive, const String& path, Item& item);
		void RefreshLocalFiles(IArchive& archive);

	protected:
		TShared<Item> rootItem;
		TShared<Item::Allocator> itemAllocator;
		String viewResourcePath;
		TShared<ResourceBase> viewResource;
		std::atomic<uint32_t> critical;
	};
}

