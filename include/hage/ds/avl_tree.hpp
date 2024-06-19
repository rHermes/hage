#pragma once

#include <cstdint>
#include <stdexcept>
#include <vector>

namespace hage::ds {

// TODO(rHermes): Specify that the key must be moveable and default constructable. Same with keys.
// This is a flaw, but it made the design so much easier.
template<typename Key, typename Value>
class AVLTree
{
private:
  class Iterator;
  class ConstIterator;
  class Node;

  using node_id_type = std::int32_t;

public:
  using value_type = Value;
  using size_type = std::size_t;
  using iterator = Iterator;
  using const_iterator = ConstIterator;

  AVLTree()
  {
    // This nodes parent is always the last node.
    m_end = get_node();
  }

  template<typename K, typename... Args>
  constexpr std::pair<Iterator, bool> try_emplace(K&& key, Args&&... args)
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

  [[nodiscard]] constexpr iterator end() noexcept { return make_iterator(m_end); }
  [[nodiscard]] constexpr iterator begin() noexcept { return make_iterator(m_begin); }

  [[nodiscard]] constexpr bool contains(const Key& key) const { return internal_find(m_root, key) != -1; }

  [[nodiscard]] constexpr size_type size() const { return m_size; }
  [[nodiscard]] constexpr bool empty() const { return size() == 0; }

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

    const Key& key() const { return m_key; }
    Value& value() { return m_value; }
  };

  std::size_t m_size{ 0 };
  std::vector<Node> m_nodes;
  node_id_type m_freeList{ -1 };

  node_id_type m_root{ -1 };
  node_id_type m_begin{ -1 };
  node_id_type m_end{ -1 };

  class Iterator
  {
  public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = Node;
    using difference_type = std::int32_t;
    using pointer = Node*;
    using reference = Node&;

    Iterator(AVLTree* tree, node_id_type id) : m_tree{ tree }, m_id{ id } {}

    constexpr reference operator*() const { return m_tree->m_nodes[m_id]; }
    constexpr pointer operator->() { return &m_tree->m_nodes[m_id]; }

    friend constexpr bool operator==(const Iterator& lhs, const Iterator& rhs)
    {
      return lhs.m_tree == rhs.m_tree && lhs.m_id == rhs.m_id;
    }
    friend constexpr bool operator!=(const Iterator& lhs, const Iterator& rhs) { return !(lhs == rhs); }

    constexpr Iterator operator++(int)
    {
      // BUG(rHermes): Should we return the value it had before iterating?
      auto it = *this;
      ++(*this);
      return it;
    }
    // Prefix increment
    constexpr Iterator& operator++()
    {
      const auto& nodes = m_tree->m_nodes;

      if (m_id == m_tree->m_end) {
        throw std::runtime_error("Invalid iterator usage, trying to increment beyond end");
      }

      if (m_id == nodes[m_tree->m_end].m_parent) {
        m_id = m_tree->m_end;
        return *this;
      }

      // ok, let's be a bit smart here. We want to go to the right. There are two ways this will go.
      // Either we are going up, or we are going down.
      if (nodes[m_id].m_right == -1) {
        while (nodes[nodes[m_id].m_parent].m_right == m_id) {
          m_id = nodes[m_id].m_parent;
        }
      } else {
        m_id = nodes[m_id].m_right;
        while (nodes[m_id].m_left != -1) {
          m_id = nodes[m_id].m_left;
        }
      }

      return *this;
    }

    constexpr Iterator operator--(int)
    {
      auto it = *this;
      --(*this);
      return it;
    }

    constexpr Iterator& operator--()
    {
      const auto& nodes = m_tree->m_nodes;

      if (m_id == m_tree->m_begin) {
        throw std::runtime_error("We tried to decrement a begin iterator");
      }

      if (m_id == m_tree->m_end) {
        m_id = nodes[m_id].m_parent;
      }

      if (nodes[m_id].m_left == -1) {

        // We need to go up again, until we are on the right side.
        while (nodes[nodes[m_id].m_parent].m_left == m_id) {
          m_id = nodes[m_id].m_parent;
        }
      } else {
        m_id = nodes[m_id].m_left;
        while (nodes[m_id].m_right != -1) {
          m_id = nodes[m_id].m_right;
        }
      }

      return *this;
    }

  private:
    friend class AVLTree;

    AVLTree* m_tree{ nullptr };
    node_id_type m_id{ -1 };

    // TODO(rHermes): create an "end" iterator, that doesn't take up a node. This could be done if we track the
    // iterators a bit differently.
  };

  [[nodiscard]] constexpr Iterator make_iterator(node_id_type id) { return Iterator{ this, id }; }

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
};

} // namespace hage::ds