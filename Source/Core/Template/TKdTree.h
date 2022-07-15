#pragma once

#include "../PaintsNow.h"
#include "../Interface/IType.h"
#include "TTagged.h"
#include "TAlgorithm.h"
#include <algorithm>
#include <cassert>
#include <utility>

namespace PaintsNow {
#ifndef _MSC_VER
	template <class T, class R = typename T::first_type, size_t N = R::size * 2>
#else
	template <class T, class R = T::first_type, size_t N = R::size * 2>
#endif
	struct TOverlap {
		typedef typename R::type type;
		typedef T BoundType;
		enum { size = N };

		static inline type& Get(T& v, uint16_t index) {
			type* buffer = reinterpret_cast<type*>(&v);
			return buffer[index];
		}

		static inline type Get(const T& v, uint16_t index) {
			const type* buffer = reinterpret_cast<const type*>(&v);
			return buffer[index];
		}

		static inline bool Compare(const T& lhs, const T& rhs, uint16_t index) {
			return Get(rhs, index) < Get(lhs, index);
		}

		static inline bool OverlapLeft(const T& lhs, const T& rhs, uint16_t index) {
			return index < size / 2 || !(Get(lhs, index) < Get(rhs, index - size / 2));
		}

		static T Bound(const T& lhs) {
			return lhs;
		}

		static inline void Union(T& lhs, const T& rhs) {
			Math::Union(lhs, rhs.first);
			Math::Union(lhs, rhs.second);
		}

		static inline bool OverlapRight(const T& lhs, const T& rhs, uint16_t index) {
			return index >= size / 2 || !(Get(rhs, index + size / 2) < Get(lhs, index));
		}

		static inline size_t Encode(const T& box, const T& value) {
			uint32_t level = (uint32_t)sizeof(size_t) * 8 / size;
			TVector<uint32_t, size / 2> divCount;
			for (size_t i = 0; i < size / 2; i++) {
				divCount[i] = (1 << (level + 1)) - 1;
			}

			TVector<uint32_t, size / 2> d[2] = {
				Math::QuantizeVector(box, value.first, divCount),
				Math::QuantizeVector(box, value.second, divCount)
			};

			return Math::Interleave(TypeTrait<size_t>(), &d[0][0], N);
		}

		template <class I>
		static I NextIndex(I index) {
			return (index + 1) % N;
		}

		// hard to understand, do not waste time here.
		// split by given index
		template <class D>
		static type SplitPush(D rightSkew, T& value, const T& reference, uint16_t index) {
			if (index < size / 2 != getboolean<D>::value) {
				type& target = Get(value, index);
				type save = target;
				target = Get(reference, index);
				return save;
			} else {
				return Get(value, index);
			}
		}

		// recover split
		static void SplitPop(T& value, uint16_t index, type save) {
			Get(value, index) = save;
		}
	};

	// KdTree with custom spatial structure
	template <class K, class Base = Void, class P = TOverlap<K>, size_t KEY_BITS = 3>
	class TKdTree : public Base {
	public:
		typedef uint8_t KeyType;
		typedef TKdTree Type;
		typedef P Meta;

		TKdTree(const K& k = K(), const KeyType i = 0) : key(k), leftNode(nullptr), rightNode(nullptr), _parentNode(nullptr) {
			const size_t KEY_MASK = (1 << KEY_BITS) - 1;
			assert(P::size <= KEY_MASK + 1);
			assert(i <= KEY_MASK);
			assert(((size_t)this & KEY_MASK) == 0); // must be aligned
			SetIndex(i);
		}

		// Add a sub tree to this, preserving KD-Tree orders
		inline void Attach(TKdTree* tree) {
			assert(tree != nullptr && tree != this);
			assert(tree->leftNode == nullptr && tree->rightNode == nullptr && tree->GetParent() == nullptr);
			CheckCycle();
			Merge(tree);
			CheckCycle();
		}

		// Remove a sub tree from this
		// selector is to select candidate from left part or from right part
		template <class S>
		inline TKdTree* Detach(S& selector) {
			CheckCycle();
			TKdTree* newRoot = nullptr;
			if (LightDetach(newRoot)) {
				CheckCycle();
				return newRoot;
			}

			CheckCycle();
			assert(leftNode != nullptr && rightNode != nullptr);

			KeyType keyIndex = GetIndex();

			// Important! select minimal of right branch or maximal of left branch as candidate to preserve Kd-Tree orders!
			TKdTree* p = selector(leftNode, rightNode) ? rightNode->FindMinimal(keyIndex) : leftNode->FindMaximal(keyIndex);
			CheckCycle();
			assert(p != nullptr);
			newRoot = p;

			TKdTree* temp = nullptr;
			p->Detach(selector);
			assert(p->GetParent() == nullptr);
			assert(p->leftNode == nullptr && p->rightNode == nullptr);

			CheckCycle();
			if (GetParent() != nullptr) {
				TKdTree** pp = GetParent()->leftNode == this ? &GetParent()->leftNode : &GetParent()->rightNode;
				*pp = newRoot;
				newRoot = nullptr;
			}

			if (leftNode != nullptr) {
				leftNode->SetParent(p);
			}

			if (rightNode != nullptr) {
				rightNode->SetParent(p);
			}

			std::swap(links, p->links);
			p->CheckCycle();
			CheckCycle();

			return newRoot;
		}

		template <class Q, class D>
		inline bool Query(D rightSkew, const K& targetKey, Q& queryer) {
			// ranged queryer
			for (TKdTree* p = this; p != nullptr; p = (getboolean<D>::value ? p->rightNode : p->leftNode)) {
				if (!queryer(*p)) {
					return false;
				}

				if /* constexpr */ (getboolean<D>::value) {
					if (p->leftNode != nullptr && P::OverlapLeft(p->key, targetKey, p->GetIndex())
						&& !p->leftNode->Query(rightSkew, targetKey, queryer)) {
						return false;
					}
				} else {
					if (p->rightNode != nullptr && P::OverlapRight(p->key, targetKey, p->GetIndex())
						&& !p->rightNode->Query(rightSkew, targetKey, queryer)) {
						return false;
					}
				}
			}

			return true;
		}

		template <class D, class Q, class C>
		inline bool Query(D rightSkew, K& targetKey, Q& queryer, C& culler) {
			// ranged queryer
			for (TKdTree* p = this; p != nullptr; p = (getboolean<D>::value ? p->rightNode : p->leftNode)) {
				if (!culler(targetKey))
					break;

				if (culler(p->key) && !queryer(*p)) {
					return false;
				}

				// culling
				auto save = P::SplitPush(rightSkew, targetKey, p->key, p->GetIndex());

				if /* constexpr */ (rightSkew) {
					// cull right in left node
					if (p->leftNode != nullptr && !p->leftNode->Query(rightSkew, targetKey, queryer, culler)) {
						return false;
					}
				} else {
					// cull left in right node
					if (p->rightNode != nullptr && !p->rightNode->Query(rightSkew, targetKey, queryer, culler)) {
						return false;
					}
				}

				P::SplitPop(targetKey, p->GetIndex(), save);
			}

			return true;
		}

		inline const K& GetKey() const {
			return key;
		}

		inline K& GetKey() {
			return key;
		}

		inline KeyType GetIndex() const {
			return (KeyType)_parentNode.Tag();
		}

		inline void SetIndex(KeyType t) {
			_parentNode.Tag(t);
		}

		inline TKdTree* GetParent() const {
			return _parentNode();
		}

		inline void SetParent(TKdTree* tree) {
			_parentNode(tree);
		}

		inline void CheckCycle() {
			KeyType keyIndex = GetIndex();
			assert(leftNode != this && rightNode != this);
			if (leftNode != nullptr) {
				assert(leftNode->GetParent() == this);
				assert(!P::Compare(leftNode->key, key, keyIndex));
			}

			if (rightNode != nullptr) {
				assert(rightNode->GetParent() == this);
				assert(!P::Compare(key, rightNode->key, keyIndex));
			}

			if (GetParent() != nullptr) {
				assert(GetParent()->leftNode == this || GetParent()->rightNode == this);
			}
		}

		void Validate() {
			CheckCycle();
			if (leftNode != nullptr) {
				leftNode->CheckCycle();
			}

			if (rightNode != nullptr) {
				rightNode->CheckCycle();
			}
		}

		// For Kd-Tree optimizing with balanced tree depth!
		class TreeCode {
		public:
			TreeCode(TKdTree* t = nullptr, size_t c = 0) : code(c), tree(t) {}
			bool operator < (const TreeCode& rhs) const { return code < rhs.code; }
			size_t code;
			TKdTree* tree;
		};

		// Rearrange Kd-Tree node to make it more balancer!
		TKdTree* Optimize() {
			// Collect all nodes over the tree
			std::vector<TreeCode> allNodes;
			allNodes.emplace_back(TreeCode(this));
			size_t n = 0;
			typename P::BoundType box = P::Bound(key);

			while (n < allNodes.size()) {
				TKdTree* tree = allNodes[n++].tree;
				P::Union(box, tree->key);

				if (tree->leftNode != nullptr) {
					allNodes.emplace_back(TreeCode(tree->leftNode));
				}

				if (tree->rightNode != nullptr) {
					allNodes.emplace_back(TreeCode(tree->rightNode));
				}

				memset(&tree->links, 0, sizeof(tree->links));
			}

			for (size_t i = 0; i < n; i++) {
				TreeCode& treeCode = allNodes[i];
				treeCode.code = P::Encode(box, treeCode.tree->key);
			}

			std::sort(allNodes.begin(), allNodes.end());
			TreeCode* root = &allNodes[allNodes.size() / 2];
			root->tree->SetIndex(0);
			Build(root->tree, &allNodes[0], root, 1);
			Build(root->tree, root + 1, &allNodes[0] + allNodes.size(), 1);

			return root->tree;
		}

	protected:
		// Build Kd-Tree from given TreeCode range
		void Build(TKdTree* root, TreeCode* begin, TreeCode* end, KeyType index) {
			if (begin < end) {
				TreeCode* mid = begin + (end - begin) / 2;
				mid->tree->SetIndex(index);
				root->Attach(mid->tree);

				index = P::NextIndex(index);
				Build(root, begin, mid, index);
				Build(root, mid + 1, end, index);
			}
		}

		bool LightDetach(TKdTree*& newRoot) {
			newRoot = nullptr;
			CheckCycle();
			if (leftNode != nullptr) {
				if (rightNode != nullptr) {
					return false;
				} else {
					leftNode->SetParent(GetParent());
					newRoot = leftNode;
					leftNode = nullptr;
				}
			} else if (rightNode != nullptr) {
				rightNode->SetParent(GetParent());
				newRoot = rightNode;
				rightNode = nullptr;
			}

			if (GetParent() != nullptr) {
				CheckCycle();
				TKdTree** pp = GetParent()->leftNode == this ? &GetParent()->leftNode : &GetParent()->rightNode;
				*pp = newRoot;
				if (newRoot != nullptr) {
					newRoot->CheckCycle();
					newRoot = nullptr;
				}
				SetParent(nullptr);
			}
			CheckCycle();

			return true;
		}

		// Find minimal node with given index in self and subtrees of self
		TKdTree* FindMinimal(KeyType index) {
			TKdTree* p = this;
			if (leftNode != nullptr) {
				TKdTree* compare = leftNode->FindMinimal(index);
				if (P::Compare(p->key, compare->key, index)) {
					p = compare;
				}
			}

			KeyType keyIndex = GetIndex();
			if (index != keyIndex && rightNode != nullptr) {
				TKdTree* compare = rightNode->FindMinimal(index);
				if (P::Compare(p->key, compare->key, index)) {
					p = compare;
				}
			}

			return p;
		}

		// Find maximal node with given index in self and subtrees of self
		TKdTree* FindMaximal(KeyType index) {
			TKdTree* p = this;
			KeyType keyIndex = GetIndex();
			if (index != keyIndex && leftNode != nullptr) {
				TKdTree* compare = leftNode->FindMaximal(index);
				if (!P::Compare(p->key, compare->key, index)) {
					p = compare;
				}
			}

			if (rightNode != nullptr) {
				TKdTree* compare = rightNode->FindMaximal(index);
				if (!P::Compare(p->key, compare->key, index)) {
					p = compare;
				}
			}

			return p;
		}

		inline void Merge(TKdTree* tree) {
			CheckCycle();
			assert(tree->GetParent() == nullptr);
			KeyType keyIndex = GetIndex();
			bool left = P::Compare(key, tree->key, keyIndex);

			TKdTree** ptr = left ? &leftNode : &rightNode;
			if (*ptr == nullptr) {
				*ptr = tree;
				tree->SetParent(this);
			} else {
				(*ptr)->Merge(tree);
			}
		}

		K key;													// 0	: Float3Pair => 24 Bytes
		union {
			struct {
				TTagged<TKdTree*, KEY_BITS> _parentNode;		// 24	: Pointer => 4 Bytes (32bit mode), 8 Bytes (64bit mode)
				TKdTree* leftNode;								// 28(32)
				TKdTree* rightNode;								// 32(40)
			};
			struct {
				TKdTree* links[3];
			} links;
		};
	};
}
