#include "../../../Core/Template/TKdTree.h"
#include "../../../Utility/MythForest/Unit.h"
#include "../../../Core/Interface/IMemory.h"
#include "Spatial.h"
#include <vector>

using namespace PaintsNow;

inline Float3Pair BuildBoundingFromVertex(const Float3& first, const Float3& second) {
	Float3Pair box(first, first);
	Math::Union(box, second);
	return box;
}

class_aligned(8) Tree : public TKdTree<Float3Pair> {
public:
	typedef TKdTree<Float3Pair> Base;
	Tree() : TKdTree<Float3Pair>(Float3Pair(Float3(0, 0, 0), Float3(0, 0, 0)), 0) {}
	Tree(const Float3Pair& b, uint8_t k) : TKdTree<Float3Pair>(b, k) {}
};

inline Float3Pair BuildBoundingFloat3PairRandomly() {
	return BuildBoundingFromVertex(Float3((float)rand(), (float)rand(), (float)rand()), Float3((float)rand(), (float)rand(), (float)rand()));
}

class Queryer {
public:
	Queryer(const Float3Pair& b) : box(b) {}
	const Float3Pair& box;
	size_t count;
	bool operator () (const Tree::Base& tree) {
		if (Math::Overlap(box, tree.GetKey()))
			count++;
		return true;
	}
};

inline size_t FastQuery(Tree*& root, const Float3Pair& box) {
	Queryer q(box);
	q.count = 0;
	assert(root->GetParent() == nullptr);
	root->Query(std::true_type(), box, q);

	return q.count;
}

struct RandomSelect {
	inline bool operator ()(Tree::Base* left, Tree::Base* right) {
		return rand() & 1;
	}
};

inline size_t LinearSearch(Tree* root, std::vector<Tree>& nodes, const Float3Pair& box) {
	size_t count = 0;
	for (size_t i = 0; i < nodes.size(); i++) {
		if (nodes[i].GetParent() != nullptr || &nodes[i] == root)
			count += Math::Overlap(nodes[i].GetKey(), box);
	}

	return count;
}

bool RandomQuery::Initialize() {
	return true;
}

bool RandomQuery::Run(int randomSeed, int length) {
	std::vector<Tree> nodes(length * 4096);
	srand(randomSeed);

	// initialize data
	for (size_t i = 0; i < nodes.size(); i++) {
		nodes[i] = Tree(BuildBoundingFloat3PairRandomly(), rand() % 6);
	}

	// link data
	// select root
	Tree* root = &nodes[rand() % nodes.size()];

	for (size_t j = 0; j < nodes.size(); j++) {
		if (root != &nodes[j]) {
			root->Attach(&nodes[j]);
		}
	}

	// random detach data
	RandomSelect randomSelect;
	for (size_t k = 0; k < nodes.size() / 10; k++) {
		size_t index = rand() % nodes.size();
		Tree* toDetach = &nodes[index];
		Tree* newRoot = (Tree*)toDetach->Detach(randomSelect);
		if (newRoot != nullptr) {
			root = newRoot;
			if (k & 1) {
				root->Attach(toDetach);
			}
		}
	}
	/*
	size_t detachedCount = 0;
	for (size_t t = 0; t < nodes.size(); t++) {
		if (nodes[t].GetParent() == nullptr && &nodes[t] != root) {
			printf("DETEACED ITEM: %d", (int)t);
		}
		detachedCount += nodes[t].GetParent() == nullptr;
	}

	printf("DEtached: %d\n", (int)detachedCount - 1);

	for (size_t m = 0; m < nodes.size(); m++) {
		nodes[m].CheckCycle();
	}*/

	// random select
	for (size_t n = 0; n < (size_t)(10 * length); n++) {
		Float3Pair box = BuildBoundingFloat3PairRandomly();
		size_t searchCount = LinearSearch(root, nodes, box);
		size_t queryCount = FastQuery(root, box);

		if (queryCount != searchCount) {
			printf("Unmatched result, %d got, %d expected.\n", (int)queryCount, (int)searchCount);
			return false;
		}
	}

	return true;
}

void RandomQuery::Summary() {
}

TObject<IReflect>& RandomQuery::operator ()(IReflect& reflect) {
	BaseClass::operator () (reflect);

	return *this;
}