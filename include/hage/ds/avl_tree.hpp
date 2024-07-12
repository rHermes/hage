#pragma once

#include <cstdint>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace hage::ds {

// TODO(rHermes): Specify that the key must be moveable and default constructable. Same with keys.
// This is a flaw, but it made the design so much easier.
template<typename Key, typename Value>
class AVLTree
{
private:
  template<bool IsConst>
  class Iterator;

  class Node;

  using node_id_type = std::int32_t;

public:
  using value_type = Value;
  using size_type = std::size_t;
  using iterator = Iterator<false>;
  using const_iterator = Iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  AVLTree()
  {
    // This nodes parent is always the last node. It's value is always 0
    m_end = get_node();
  }

  template<typename K, typename... Args>
  constexpr std::pair<iterator, bool> try_emplace(K&& key, Args&&... args)
  {
    if (m_root == -1) {
      m_root = get_node(std::forward<K>(key), Value{ std::forward<Args>(args)... });
      m_begin = m_root;
      m_nodes[m_end].m_parent = m_root;

      m_size++;
      return { make_iterator(m_root), true };
    }

    const auto [id, inserted] = internal_try_emplace(m_root, std::forward<K>(key), std::forward<Args>(args)...);
    return { make_iterator(id), inserted };
  }

  [[nodiscard]] constexpr iterator find(const Key& key)
  {
    auto id = internal_find(m_root, key);
    if (id == -1) {
      return end();
    } else {
      return make_iterator(id);
    }
  }

  [[nodiscard]] constexpr const_iterator find(const Key& key) const
  {
    auto id = internal_find(m_root, key);
    if (id == -1) {
      return end();
    } else {
      return make_const_iterator(id);
    }
  }

  constexpr iterator erase(iterator pos)
  {
    auto id = internal_erase(pos.m_id);
    return make_iterator(id);
  }

  [[nodiscard]] constexpr bool contains(const Key& key) const { return internal_find(m_root, key) != -1; }

  [[nodiscard]] constexpr size_type size() const { return m_size; }
  [[nodiscard]] constexpr bool empty() const { return size() == 0; }

  constexpr void clear()
  {
    // Important to note here, that the end iterator, must be valid.
    // we have the understanding that m_last is always 0.
    // we are freeing from the back, so that the smallest nodes will be used first.
    for (std::size_t i = m_nodes.size() - 1; 0 < i; i--) {
      free_node(i);
    }
    m_size = 0;
    m_root = -1;
  }

  [[nodiscard]] constexpr iterator end() noexcept { return make_iterator(m_end); }
  [[nodiscard]] constexpr iterator begin() noexcept { return make_iterator(m_begin); }

  [[nodiscard]] constexpr const_iterator end() const noexcept { return make_const_iterator(m_end); }
  [[nodiscard]] constexpr const_iterator begin() const noexcept { return make_const_iterator(m_begin); }

  [[nodiscard]] constexpr const_iterator cend() const noexcept { return make_const_iterator(m_end); }
  [[nodiscard]] constexpr const_iterator cbegin() const noexcept { return make_const_iterator(m_begin); }

  [[nodiscard]] constexpr reverse_iterator rbegin() noexcept
  {
    return std::make_reverse_iterator(make_iterator(m_end));
  }
  [[nodiscard]] constexpr reverse_iterator rend() noexcept
  {
    return std::make_reverse_iterator(make_iterator(m_begin));
  }

  [[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept
  {
    return std::make_reverse_iterator(make_const_iterator(m_end));
  }
  [[nodiscard]] constexpr const_reverse_iterator rend() const noexcept
  {
    return std::make_reverse_iterator(make_const_iterator(m_begin));
  }

  [[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept
  {
    return std::make_reverse_iterator(make_const_iterator(m_end));
  }
  [[nodiscard]] constexpr const_reverse_iterator crend() const noexcept
  {
    return std::make_reverse_iterator(make_const_iterator(m_begin));
  }

  [[nodiscard]] std::string print_tree() const
  {
    std::ostringstream out;
    internal_print(out, m_root, 0);
    return out.str();
  }

  [[nodiscard]] std::string print_dot_tree() const
  {
    DotPrinter printer;
    printer.add_default_node_attrib("ordering", "out");
    printer.add_default_node_attrib("colorscheme", "piyg5");
    printer.add_default_node_attrib("style", "filled");

    internal_dot_print(printer, m_root, 0);
    return printer.print();
  }

private:
  class Node
  {
  private:
    friend class AVLTree;

    Key m_key{};
    Value m_value{};

    char m_balance{ 0 };
    node_id_type m_parent{ -1 };
    node_id_type m_left{ -1 };
    node_id_type m_right{ -1 };

  public:
    constexpr Node(){};

    template<typename K, typename V>
    constexpr Node(K&& k, V&& val) : m_key{ std::forward<K>(k) }
                                   , m_value{ std::forward<V>(val) }
    {
    }

    [[nodiscard]] constexpr const Key& key() const { return m_key; }
    [[nodiscard]] constexpr Value& value() { return m_value; }
    [[nodiscard]] constexpr const Value& value() const { return m_value; }
  };

  std::size_t m_size{ 0 };
  std::vector<Node> m_nodes;
  node_id_type m_freeList{ -1 };

  node_id_type m_root{ -1 };
  node_id_type m_begin{ -1 };
  node_id_type m_end{ -1 };

  template<bool IsConst>
  class Iterator
  {
  private:
    using tree_type = std::conditional_t<IsConst, const AVLTree, AVLTree>;
    using node_type = std::conditional_t<IsConst, const Node, Node>;

  public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = node_type;
    using difference_type = std::int32_t;
    using pointer = value_type*;
    using reference = value_type&;

    Iterator() = default;
    Iterator(tree_type* tree, node_id_type id) : m_tree{ tree }, m_id{ id } {}

    constexpr reference operator*() const { return m_tree->m_nodes[m_id]; }
    constexpr pointer operator->() const { return &m_tree->m_nodes[m_id]; }

    friend constexpr bool operator==(const Iterator& lhs, const Iterator& rhs)
    {
      return lhs.m_tree == rhs.m_tree && lhs.m_id == rhs.m_id;
    }
    friend constexpr bool operator!=(const Iterator& lhs, const Iterator& rhs) { return !(lhs == rhs); }

    constexpr Iterator operator++(int)
    {
      // BUG(rHermes): Should we return the value it had before iterating?
      auto it = *this;
      this->operator++();
      return it;
    }
    // Prefix increment
    constexpr Iterator& operator++()
    {
      m_id = m_tree->next_node(m_id);
      return *this;
    }

    constexpr Iterator operator--(int)
    {
      auto it = *this;
      this->operator--();
      return it;
    }

    constexpr Iterator& operator--()
    {
      m_id = m_tree->prev_node(m_id);
      return *this;
    }

  private:
    friend class AVLTree;

    tree_type* m_tree{ nullptr };
    node_id_type m_id{ -1 };

    // TODO(rHermes): create an "end" iterator, that doesn't take up a node. This could be done if we track the
    // iterators a bit differently.
  };

  [[nodiscard]] constexpr iterator make_iterator(node_id_type id) { return iterator{ this, id }; }

  [[nodiscard]] constexpr const_iterator make_const_iterator(node_id_type id) const
  {
    return const_iterator{ this, id };
  }

  template<typename K, typename V>
  [[nodiscard]] constexpr node_id_type get_node(K&& key, V&& value)
  {
    if (m_freeList != -1) {
      auto ret = m_freeList;
      m_freeList = m_nodes[ret].m_parent;

      auto& node = m_nodes[ret];
      node = { std::forward<K>(key), std::forward<V>(value) };

      return ret;
    }

    const auto ret = static_cast<node_id_type>(m_nodes.size());
    m_nodes.emplace_back(std::forward<K>(key), std::forward<V>(value));
    return ret;
  }

  [[nodiscard]] constexpr node_id_type get_node()
  {
    if (m_freeList != -1) {
      auto ret = m_freeList;
      m_freeList = m_nodes[ret].m_parent;

      auto& node = m_nodes[ret];
      node = {};
      return ret;
    }

    const auto ret = static_cast<node_id_type>(m_nodes.size());
    m_nodes.emplace_back();
    return ret;
  }

  constexpr void free_node(node_id_type node)
  {
    m_nodes[node] = {};
    m_nodes[node].m_parent = m_freeList;
    m_freeList = node;
  }

  [[nodiscard]] constexpr node_id_type internal_find(node_id_type root, const Key& key) const
  {
    while (root != -1) {
      const auto& node = m_nodes[root];
      if (node.m_key == key) {
        break;
      } else if (key < node.m_key) {
        root = node.m_left;
      } else {
        root = node.m_right;
      }
    }

    return root;
  }

  template<typename K, typename... Args>
  constexpr std::pair<node_id_type, bool> internal_try_emplace(node_id_type root, K&& key, Args&&... args)
  {
    auto cur = root;
    auto* node = &m_nodes[cur];

    node_id_type child = -1;
    while (true) {
      node = &m_nodes[cur];
      if (node->m_key == key) {
        return { cur, false };
      } else if (key < node->m_key) {
        if (node->m_left != -1) {
          cur = node->m_left;
          continue;
        }

        child = get_node(std::forward<K>(key), std::forward<Args>(args)...);
        m_nodes[child].m_parent = cur;

        // Update begin
        if (cur == m_begin)
          m_begin = child;

        node = &m_nodes[cur];
        node->m_left = child;
        break;

      } else {
        if (node->m_right != -1) {
          cur = node->m_right;
          continue;
        }

        child = get_node(std::forward<K>(key), std::forward<Args>(args)...);
        m_nodes[child].m_parent = cur;

        // ok, the only way this updates end is if the parent of m_end is the parent node.
        if (m_nodes[m_end].m_parent == cur)
          m_nodes[m_end].m_parent = child;

        node = &m_nodes[cur];
        node->m_right = child;
        break;
      }
    }

    // Ok, child is now the newly inserted item and cur is the parent.
    const auto newChild = child;

    for (; cur != -1; cur = m_nodes[child].m_parent) {

      node_id_type oldRoot = -1;
      node_id_type newRoot = -1;

      auto* parent = &m_nodes[cur];
      auto* ch = &m_nodes[child];

      if (child == parent->m_right) {
        if (parent->m_balance == 1) {
          // We are right heavy, we will need to rebalance.
          oldRoot = parent->m_parent;
          if (ch->m_balance == -1) {
            // Right left case
            newRoot = rotate_right_left(cur, child);
          } else {
            // right right case
            newRoot = rotate_left(cur, child);
          }
        } else {
          ++parent->m_balance;
          if (parent->m_balance == 0)
            break;

          child = cur;
          continue;
        }
      } else {
        // It's a left child.
        if (parent->m_balance == -1) {
          // Rebalancing will be required.

          oldRoot = parent->m_parent;
          if (ch->m_left == 1) {
            // left right
            newRoot = rotate_left_right(cur, child);
          } else {
            // left left case
            newRoot = rotate_right(cur, child);
          }

          // After rotation adapt parent link
        } else {
          --parent->m_balance;
          if (parent->m_balance == 0) {
            break;
          }

          child = cur; // The height of z increases
          continue;
        }
      }

      // ok, now we have rotated a parent link.
      // N is the new root of the rotated subtree.
      // The height did not change. Height(N) == old height(X)
      m_nodes[newRoot].m_parent = oldRoot;
      if (oldRoot != -1) {
        if (cur == m_nodes[oldRoot].m_left) {
          m_nodes[oldRoot].m_left = newRoot;
        } else {
          m_nodes[oldRoot].m_right = newRoot;
        }
      } else {
        m_root = newRoot;
      }

      break;
    }

    m_size++;
    return { newChild, true };
  }

  [[nodiscard]] constexpr node_id_type rotate_left(node_id_type parentId, node_id_type childId)
  {
    Node* parent = &m_nodes[parentId];
    Node* child = &m_nodes[childId];

    auto t23 = child->m_left;
    parent->m_right = t23;
    if (t23 != -1) {
      m_nodes[t23].m_parent = parentId;
    }

    child->m_left = parentId;
    parent->m_parent = childId;

    // This only happens on deletion
    if (child->m_balance == 0) {
      parent->m_balance = 1;
      child->m_balance = -1;
    } else {
      parent->m_balance = 0;
      child->m_balance = 0;
    }

    return childId;
  }

  [[nodiscard]] constexpr node_id_type rotate_right(node_id_type parentId, node_id_type childId)
  {
    Node* parent = &m_nodes[parentId];
    Node* child = &m_nodes[childId];

    auto t23 = child->m_right;
    parent->m_left = t23;
    if (t23 != -1) {
      m_nodes[t23].m_parent = parentId;
    }

    child->m_right = parentId;
    parent->m_parent = childId;

    // This only happens on deletion
    if (child->m_balance == 0) {
      parent->m_balance = -1;
      child->m_balance = 1;
    } else {
      parent->m_balance = 0;
      child->m_balance = 0;
    }

    return childId;
  }

  [[nodiscard]] constexpr node_id_type rotate_right_left(node_id_type parentId, node_id_type childId)
  {
    Node* parent = &m_nodes[parentId];
    Node* child = &m_nodes[childId];

    auto Y = child->m_left;
    Node* yc = &m_nodes[Y];

    // swap over the underlying node
    auto t3 = yc->m_right;
    child->m_left = t3;
    if (t3 != -1) {
      m_nodes[t3].m_parent = childId;
    }

    yc->m_right = childId;
    child->m_parent = Y;

    auto t2 = yc->m_left;
    parent->m_right = t2;
    if (t2 != -1) {
      m_nodes[t2].m_parent = parentId;
    }

    yc->m_left = parentId;
    parent->m_parent = Y;

    // This only happens on deletion
    // If the inner left child was balanced
    if (yc->m_balance == 0) {
      parent->m_balance = 0;
      child->m_balance = 0;
    } else if (yc->m_balance == 1) {
      // The inner child was right heavy.
      parent->m_balance = -1; // T1 is now higher
      child->m_balance = 0;
    } else {
      // T2 was higher
      parent->m_balance = 0;
      child->m_balance = +1;
    }

    yc->m_balance = 0;
    return Y;
  }

  [[nodiscard]] constexpr node_id_type rotate_left_right(node_id_type parentId, node_id_type childId)
  {
    Node* parent = &m_nodes[parentId];
    Node* child = &m_nodes[childId];

    auto Y = child->m_right;
    Node* yc = &m_nodes[Y];

    // swap over the underlying node
    auto t3 = yc->m_left;
    child->m_right = t3;
    if (t3 != -1) {
      m_nodes[t3].m_parent = childId;
    }

    yc->m_left = childId;
    child->m_parent = Y;

    auto t2 = yc->m_right;
    parent->m_left = t2;
    if (t2 != -1) {
      m_nodes[t2].m_parent = parentId;
    }

    yc->m_right = parentId;
    parent->m_parent = Y;

    // This only happens on deletion
    // If the inner left child was balanced
    if (yc->m_balance == 0) {
      parent->m_balance = 0;
      child->m_balance = 0;
    } else if (yc->m_balance == -1) {
      // The inner child was right heavy.
      parent->m_balance = 1; // T1 is now higher
      child->m_balance = 0;
    } else {
      // T2 was higher
      parent->m_balance = 0;
      child->m_balance = -1;
    }

    yc->m_balance = 0;
    return Y;
  }

  [[nodiscard]] constexpr node_id_type next_node(node_id_type cur) const
  {
    if (cur == m_end) {
      throw std::runtime_error("Invalid iterator usage, trying to increment beyond end");
    }

    if (cur == m_nodes[m_end].m_parent) {
      return m_end;
    }

    // ok, let's be a bit smart here. We want to go to the right. There are two ways this will go.
    // Either we are going up, or we are going down.
    if (m_nodes[cur].m_right == -1) {
      auto prev = cur;
      cur = m_nodes[cur].m_parent;
      while (m_nodes[cur].m_right == prev) {
        prev = cur;
        cur = m_nodes[cur].m_parent;
      }
    } else {
      cur = m_nodes[cur].m_right;
      while (m_nodes[cur].m_left != -1) {
        cur = m_nodes[cur].m_left;
      }
    }

    return cur;
  }

  [[nodiscard]] constexpr node_id_type prev_node(node_id_type cur) const
  {
    if (cur == m_begin) {
      throw std::runtime_error("We tried to decrement a begin iterator");
    }

    if (cur == m_end) {
      return m_nodes[cur].m_parent;
    }

    if (m_nodes[cur].m_left == -1) {
      // We need to go up again, until we are on the right side.
      auto prev = cur;
      cur = m_nodes[cur].m_parent;
      while (m_nodes[cur].m_left == prev) {
        prev = cur;
        cur = m_nodes[cur].m_parent;
      }
    } else {
      cur = m_nodes[cur].m_left;
      while (m_nodes[cur].m_right != -1) {
        cur = m_nodes[cur].m_right;
      }
    }

    return cur;
  }

  [[nodiscard]] constexpr node_id_type internal_erase(const node_id_type root)
  {

    const auto retValue = next_node(root);

    // If we are going to delete the end root, we need to update.
    if (root == m_nodes[m_end].m_parent)
      m_nodes[m_end].m_parent = m_nodes[root].m_parent;

    if (root == m_begin) {
      m_begin = retValue;
    }

    // ok, we are going to

    node_id_type newRoot = root;

    // Ok, but it's here that the sausage will get made.
    int firstNudge = 0;
    bool lateCleanup = false;
    auto& node = m_nodes[root];
    if (node.m_left == -1 && node.m_right == -1) {
      lateCleanup = true;
    } else if (node.m_left == -1) {
      newRoot = node.m_right;
      auto root_right = node.m_right;
      if (node.m_parent == -1) {
        m_root = root_right;
      } else {
        if (m_nodes[node.m_parent].m_left == root) {
          m_nodes[node.m_parent].m_left = root_right;
          firstNudge = -1;
        } else {
          m_nodes[node.m_parent].m_right = root_right;
          firstNudge = 1;
        }
      }

      m_nodes[newRoot].m_parent = std::exchange(node.m_parent, newRoot);
    } else if (node.m_right == -1) {
      newRoot = node.m_left;

      auto root_left = node.m_left;
      if (node.m_parent == -1) {
        m_root = root_left;
      } else {
        if (m_nodes[node.m_parent].m_left == root) {
          m_nodes[node.m_parent].m_left = root_left;
          firstNudge = -1;
        } else {
          m_nodes[node.m_parent].m_right = root_left;
          firstNudge = 1;
        }
      }

      m_nodes[newRoot].m_parent = std::exchange(node.m_parent, newRoot);
    } else {
      // This is where we need change also internal balancing nodes.
      auto next = retValue;
      auto& nextNode = m_nodes[next];

      // ok, we have two children, we will do a bit of a meme here.
      nextNode.m_left = std::exchange(node.m_left, -1);
      m_nodes[nextNode.m_left].m_parent = next;

      nextNode.m_balance = std::exchange(node.m_balance, 0);

      if (node.m_parent == -1) {
        m_root = next;
      } else {
        if (m_nodes[node.m_parent].m_left == root) {
          m_nodes[node.m_parent].m_left = next;
        } else {
          m_nodes[node.m_parent].m_right = next;
        }
      }

      if (node.m_right != next) {
        nextNode.m_right = std::exchange(node.m_right, -1);
        m_nodes[nextNode.m_right].m_parent = next;

        if (m_nodes[nextNode.m_parent].m_left == next) {
          m_nodes[nextNode.m_parent].m_left = -1;
        } else {
          m_nodes[nextNode.m_parent].m_right = -1;
        }
        nextNode.m_parent = std::exchange(node.m_parent, nextNode.m_parent);

      } else {
        firstNudge = 1;
        nextNode.m_parent = std::exchange(node.m_parent, next);
      }
    }

    // OK, time to rebalance the tree, let's hope I can do it. With wikipedia as my guide it can work.
    node_id_type G = -1;
    for (auto parentNode = m_nodes[newRoot].m_parent; parentNode != -1; parentNode = G) {
      G = m_nodes[parentNode].m_parent;
      int b = 0;

      // BF(X) is not yet updated.
      if (firstNudge == 1 || newRoot == m_nodes[parentNode].m_right) { // it's the right tree which decreases
        firstNudge = 0;
        // We are in the right subtree
        if (m_nodes[parentNode].m_balance < 0) {
          // parentNode is left heavy and we need to rebalance
          auto Z = m_nodes[parentNode].m_left;
          b = m_nodes[Z].m_balance;

          if (0 < b) {
            newRoot = rotate_left_right(parentNode, Z);
          } else {
            newRoot = rotate_right(parentNode, Z);
          }
        } else {
          auto pre = m_nodes[parentNode].m_balance--;
          if (pre == 0)
            break;

          newRoot = parentNode;
          continue;
        }
      } else {
        firstNudge = 0;

        if (0 < m_nodes[parentNode].m_balance) { // X is right heavy
          // THe temporary BF(X) == +2 amd we need to rebalance
          // we need to rebalance
          auto Z = m_nodes[parentNode].m_right;
          b = m_nodes[Z].m_balance;
          if (b < 0) {
            newRoot = rotate_right_left(parentNode, Z);
          } else {
            newRoot = rotate_left(parentNode, Z);
          }
        } else {
          auto pre = m_nodes[parentNode].m_balance++;
          if (pre == 0)
            break;

          newRoot = parentNode;
          continue;
        }
      }

      // After a rotation adapt parent link:
      // newRoot is the new root of the rotated subtree
      m_nodes[newRoot].m_parent = G;
      if (G != -1) {
        if (parentNode == m_nodes[G].m_left) {
          m_nodes[G].m_left = newRoot;
        } else {
          m_nodes[G].m_right = newRoot;
        }
      } else {
        m_root = newRoot;
      }

      if (b == 0)
        break;
    }

    if (lateCleanup && node.m_parent != -1) {
      if (m_nodes[node.m_parent].m_left == root) {
        m_nodes[node.m_parent].m_left = -1;
      } else {
        m_nodes[node.m_parent].m_right = -1;
      }
    }

    free_node(root);
    --m_size;
    return retValue;
  }

  constexpr void internal_print(std::ostringstream& out, const node_id_type cur, const int depth) const
  {
    std::string padding(depth * 2, '_');

    if (cur == -1) {
      out << padding << "[NO NODE]\n";
      return;
    }

    const auto& node = m_nodes[cur];

    out << padding << "[ .id=" << cur << ", .bal=" << static_cast<int>(node.m_balance);
    out << ", .key=" << node.m_key;
    // out << ", .val=" << node.m_value;
    out << "]";
    if (cur == m_root) {
      out << " (root)";
    }

    if (cur == m_begin) {
      out << " (begin)";
    }

    if (cur == m_nodes[m_end].m_parent) {
      out << " (end)";
    }
    out << "\n";

    internal_print(out, node.m_left, depth + 1);
    internal_print(out, node.m_right, depth + 1);
  }

  class DotPrinter
  {
    int m_nullNodes{ 0 };
    std::unordered_map<std::string, std::string> m_defaultNodeAttrib;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> m_nodes;
    std::vector<std::pair<std::string, std::string>> m_edges;

    std::string get_null_id()
    {
      std::stringstream ss;
      ss << "null";
      ss << m_nullNodes++;
      return ss.str();
    }

    void do_format_attributes(std::ostream& out, const std::unordered_map<std::string, std::string>& attribs) const
    {
      if (!attribs.empty()) {
        out << " [";
        bool first = true;
        for (const auto& [name, val] : attribs) {
          if (!first) {
            out << ",";
          } else {
            first = false;
          }
          out << name << "=" << val;
        }
        out << "]";
      }
    }

  public:
    void add_default_node_attrib(const std::string& key, const std::string& val) { m_defaultNodeAttrib[key] = val; }

    std::string get_node_id(int id)
    {
      std::stringstream ss;
      ss << "node";
      ss << id;
      return ss.str();
    }

    void add_node_attrib(const std::string& sid, const std::string& key, const std::string& val, bool isHtml)
    {
      using namespace std::string_literals;
      if (isHtml) {
        m_nodes[sid][key] = "<"s + val + ">"s;
      } else {
        m_nodes[sid][key] = "\""s + val + "\""s;
      }
    }

    void add_node_attrib(const std::string& sid, const std::string& key, const int val, bool isHtml)
    {
      std::ostringstream out;
      out << val;
      add_node_attrib(sid, key, out.str(), isHtml);
    }

    std::string add_node(int id, int balance, int key)
    {
      const auto sid = get_node_id(id);
      std::ostringstream label;
      // label << "id = " << id << ", bal = " << balance << ", key = " << key;
      label << key << "<SUP>" << id << "</SUP>";
      add_node_attrib(sid, "label", label.str(), true);

      label.str(std::string());
      // label << id << " (" << balance << ")";
      label << "(" << balance << ")";
      add_node_attrib(sid, "tooltip", label.str(), false);

      // add_node_attrib(sid, "style", "filled");
      add_node_attrib(sid, "fillcolor", 3 + balance, false);

      return sid;
    }

    std::string get_null_node()
    {
      const auto sid = get_null_id();
      add_node_attrib(sid, "shape", "point", false);
      return sid;
    }

    void add_edge(const std::string& src, const std::string& dst) { m_edges.emplace_back(src, dst); }

    [[nodiscard]] std::string print() const
    {
      std::ostringstream out;
      out << "digraph BST {\n";

      out << "\tnode";
      do_format_attributes(out, m_defaultNodeAttrib);
      out << ";\n";

      // Nodes
      for (const auto& [sid, attribs] : m_nodes) {
        out << "\t" << sid;
        do_format_attributes(out, attribs);
        out << ";\n";
      }

      // Edges
      for (const auto& [src, dst] : m_edges) {
        out << src << " -> " << dst << "\n";
      }

      out << "}\n";

      return out.str();
    }
  };

  constexpr void internal_dot_print(DotPrinter& printer, const node_id_type cur, const int depth) const
  {
    const auto& node = m_nodes[cur];

    const auto sid = printer.add_node(cur, node.m_balance, node.key());

    std::string xlabel;
    if (cur == m_root) {
      xlabel += " (root)";
    }

    if (cur == m_begin) {
      xlabel += " (begin)";
    }

    if (cur == m_nodes[m_end].m_parent) {
      xlabel += " (end)";
    }

    if (!xlabel.empty()) {
      printer.add_node_attrib(sid, "xlabel", xlabel, false);
    }

    if (node.m_left != -1) {
      const auto kid = printer.get_node_id(node.m_left);
      printer.add_edge(sid, kid);
      internal_dot_print(printer, node.m_left, depth + 1);
    } else {
      const auto kid = printer.get_null_node();
      printer.add_edge(sid, kid);
    }

    if (node.m_right != -1) {
      const auto kid = printer.get_node_id(node.m_right);
      printer.add_edge(sid, kid);
      internal_dot_print(printer, node.m_right, depth + 1);
    } else {
      const auto kid = printer.get_null_node();
      printer.add_edge(sid, kid);
    }
  }
};

} // namespace hage::ds