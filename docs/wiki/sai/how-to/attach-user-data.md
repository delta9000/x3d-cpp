---
title: Attach user data to SAI nodes and fields
summary: Store, read, replace and clear application-owned data on a node or field.
tags: [sai, how-to, user-data]
updated: 2026-11-14
related:
  - ../reference/user-data.md
---

# Attach user data to SAI nodes and fields

User data lets an application hang its own object on a node or a field, for
example a selection record or an editor widget handle. Exact rules are in
[User data](../reference/user-data.md).

The snippets use the kernel's handle methods. Through the portable service the
calls are `service.user_data(node)` and `service.set_user_data(node, value)`,
with the same overloads for a `field`.

## Store a value

```cpp
using namespace x3d::sai::experimental;

auto data = user_data_value::make<std::string>("selected item");
auto stored = node_handle.set_user_data(std::move(data));
if (!stored) { /* inspect stored.error() */ }
```

`make<T>` constructs the object in place, so `T` can be move-only.

## Read it back

```cpp
auto data = node_handle.user_data();
if (data) {
  auto text = data->as<std::string>();
  if (text && *text) {
    // *text is shared_ptr<const std::string>; **text is the stored string.
  }
}
```

Three outcomes to tell apart:

| Result | Meaning |
|---|---|
| `!data` | The service call failed |
| `text` holds a null pointer | Nothing is stored |
| `!text` with `type_mismatch` | Something else is stored |

## Replace or clear

```cpp
node_handle.set_user_data(user_data_value::make<int>(42));  // replace
node_handle.set_user_data({});                              // clear
```

## Attach to a field

The same two calls exist on field handles. The data belongs to the canonical
field, so `set_translation`, `translation` and `translation_changed` all reach
one association. Any access type works, including inputOnly.

## Avoid ownership cycles

If your payload holds an owning reference to the context, browser or service,
clear it before you drop your last handle. Nothing collects cycles for you.
Prefer a `weak_ptr` inside the payload when it only needs to observe.

## Things to expect

- Changes take effect immediately. They are not part of an edit, an update
  gate or an event batch, and they emit no notifications.
- Data stays attached until you clear or replace it, or the context is
  destroyed. Removing the node does not release it.
- The shared pointer you read stays valid after the data is replaced.
