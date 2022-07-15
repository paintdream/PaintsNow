// Graph.h
// PaintDream (paintdream@paintdream.com)
// 2018-7-30
//

#pragma once
#include "Tiny.h"
#include <set>
#include <map>
#include <sstream>

namespace PaintsNow {
	template <class N>
	class Graph;

	template <class P>
	class GraphPort : public TReflected<GraphPort<P>, P> {
	public:
		typedef TReflected<GraphPort<P>, P> BaseClass;
		GraphPort(Tiny* n = nullptr) : node(n) {}

		class LinkInfo {
		public:
			bool operator < (const LinkInfo& c) const {
				return port < c.port;
			}

			LinkInfo(GraphPort* p = nullptr, Tiny::FLAG f = 0) : port(p), flag(f) {}
			GraphPort* port;
			Tiny::FLAG flag;
		};

		void Link(GraphPort* p, Tiny::FLAG f = Tiny::TINY_PINNED, Tiny::FLAG t = 0) {
			BinaryInsert(links, LinkInfo(p, f));
			BinaryInsert(p->links, LinkInfo(this, t));
		}

		void UnLink(GraphPort* p) {
			BinaryErase(links, LinkInfo(p));
			BinaryErase(p->links, LinkInfo(this));
		}

		void Cleanup() {
			for (size_t i = 0; i < links.size(); i++) {
				BinaryErase(links[i].port->links, LinkInfo(this));
			}

			links.clear();
		}

		TObject<IReflect>& operator () (IReflect& reflect) override {
			BaseClass::operator () (reflect);
			return *this;
		}

		Tiny* GetNode() const {
			return node;
		}

		void SetNode(Tiny* n) {
			node = n;
		}

		const std::vector<LinkInfo>& GetLinks() const {
			return links;
		}

	protected:
		Tiny* node;
		std::vector<LinkInfo> links;
	};

	// T Must be derived from Tiny
	template <class T, class P>
	class GraphNode : public T {
	public:
		typedef P Port;
		class PortInfo {
		public:
			bool operator < (const PortInfo& c) const {
				return name < c.name;
			}

			PortInfo(const String& s = "", Port* p = nullptr) : name(s), port(p) {}
			String name;
			Port* port;
		};

	private:
		class PortsReflector : public IReflect {
		public:
			PortsReflector(GraphNode* t, std::vector<PortInfo>& ports, bool r) : IReflect(true, false), thisNode(t), nodePorts(ports), recursive(r) {}
			void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) override {
				if (!s.IsBasicObject()) {
					if (s.IsIterator()) {
						IIterator& it = static_cast<IIterator&>(s);
						Unique subType = it.GetElementUnique();
						Unique refSubType = it.GetElementReferenceUnique();
						if (subType != refSubType && refSubType->IsClass(UniqueType<Port>::Get())) {
							size_t i = 0;
							while (it.Next()) {
								std::stringstream ss;
								ss << name << "[" << i++ << "]";
								TShared<Port>& p = *reinterpret_cast<TShared<Port>*>(it.Get());
								assert(p);

								Property(*p(), subType, it.GetElementReferenceUnique(), ss.str().c_str(), ptr, &p, nullptr);
							}
						}
					} else {
						Port* p = s.QueryInterface(UniqueType<Port>());
						if (p != nullptr) {
							p->SetNode(thisNode);
							BinaryInsert(nodePorts, PortInfo(path + name, p));
						} else if (recursive) {
							String orgPath = path;
							path = path + name + ".";
							s(*this);
							path = orgPath;
						}
					}
				}
			}

			void Method(const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) override {}

		private:
			std::vector<PortInfo>& nodePorts;
			GraphNode* thisNode;
			String path;
			bool recursive;
		};

	public:
		void ReflectNodePorts(bool recursive = false) {
			// use reflected tech to get node ports
			std::vector<PortInfo> newNodePorts;
			PortsReflector reflector(this, newNodePorts, recursive);
			(*this)(reflector);
			std::swap(nodePorts, newNodePorts);
		}

		void Cleanup() {
			for (size_t i = 0; i < nodePorts.size(); i++) {
				nodePorts[i].port->Cleanup();
			}
		}

		Port* operator [] (const String& key) const {
			typename std::vector<PortInfo>::const_iterator it = BinaryFind(nodePorts.begin(), nodePorts.end(), PortInfo(key));
			return it == nodePorts.end() ? nullptr : it->port;
		}

		const std::vector<PortInfo>& GetPorts() const {
			return nodePorts;
		}

	protected:
		std::vector<PortInfo> nodePorts;
	};

	template <class N>
	class Graph {
	public:
		typedef N Node;
		typedef typename N::Port Port;

		void AddNode(Node* node) {
			node->ReferenceObject();
			BinaryInsert(allNodes, node);
		}

		void RemoveNode(Node* node) {
			node->Cleanup();
			BinaryErase(allNodes, node);
			node->ReleaseObject();
		}

		bool HasNode(Node* node) {
			return BinaryFind(allNodes.begin(), allNodes.end(), node) != allNodes.end();
		}

		~Graph() {
			for (size_t i = 0; i < allNodes.size(); i++) {
				Node* node = allNodes[i];
				node->Cleanup();
				node->ReleaseObject();
			}
		}

		// removes all nodes
		template <class F>
		void Optimize(F f) {
			std::vector<Node*> importantNodes;
			std::vector<Node*> finalNodes;

			for (size_t i = 0; i < allNodes.size(); i++) {
				Node* n = allNodes[i];
				if (f(n)) {
					importantNodes.push_back(n);
				}
			}

			while (!importantNodes.empty()) {
				std::vector<Node*> newNodes;
				for (size_t i = 0; i < importantNodes.size(); i++) {
					Node* node = importantNodes[i];
					for (size_t n = 0; n < node->GetPorts().size(); n++) {
						Port* port = node->GetPorts()[n].port;
						for (size_t m = 0; m < port->GetLinks().size(); m++) {
							Node* p = static_cast<Node*>(port->GetLinks()[m].port->GetNode());

							if (BinaryFind(finalNodes.begin(), finalNodes.end(), p) == finalNodes.end()) {
								BinaryInsert(finalNodes, p);
								newNodes.push_back(p);
							}
						}
					}
				}

				std::swap(newNodes, importantNodes);
			}

			// removed all unreferenced nodes, quickly!
			for (size_t j = 0; j < allNodes.size(); j++) {
				Node* node = allNodes[j];

				if (BinaryFind(finalNodes.begin(), finalNodes.end(), node) == finalNodes.end()) {
					node->Cleanup();
					node->ReleaseObject();
				}
			}

			std::swap(finalNodes, allNodes);
		}

		template <class F, class B>
		bool IterateTopological(F& t, B& b) const {
			std::vector<Node*> preNodes = allNodes;

			while (!preNodes.empty() && b()) {
				std::vector<Node*> lockedNodes;
				for (size_t i = 0; i < preNodes.size(); i++) {
					Node* node = preNodes[i];
					for (size_t n = 0; n < node->GetPorts().size(); n++) {
						Port* port = node->GetPorts()[n].port;
						for (size_t m = 0; m < port->GetLinks().size(); m++) {
							Port* q = static_cast<Port*>(port->GetLinks()[m].port);
							Tiny::FLAG flag = port->GetLinks()[m].flag;
							if (flag & Tiny::TINY_PINNED) {
								BinaryInsert(lockedNodes, static_cast<Node*>(q->GetNode()));
							}
						}
					}
				}

				// no more available nodes, got cycle. exit.
				if (preNodes.size() == lockedNodes.size()) {
					return false;
				}

				for (size_t j = 0; j < preNodes.size(); j++) {
					Node* node = preNodes[j];
					if (BinaryFind(lockedNodes.begin(), lockedNodes.end(), node) == lockedNodes.end()) {
						if (!t(node)) {
							return false;
						}
					}
				}

				std::swap(lockedNodes, preNodes);
			}

			return true;
		}

	protected:
		std::vector<Node*> allNodes;
	};
}

