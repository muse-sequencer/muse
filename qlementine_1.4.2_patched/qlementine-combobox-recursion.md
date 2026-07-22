# Infinite recursion / OOM crash in `ComboboxItemViewFilter`/`ComboboxFilter` on first `QComboBox::view()` call

## Summary

`ComboboxItemViewFilter::eventFilter()` and `ComboboxFilter::eventFilter()` both react to a
`QEvent::ChildAdded` on the combo box by checking `child == _comboBox->view()`. The first time a
`QComboBox`'s popup view is constructed, this check re-enters `QComboBox::view()` from *inside*
the `ChildAdded` event that view()'s own lazy construction fires — before
`QComboBoxPrivate::viewContainer()` has cached the container it's still constructing. The
reentrant call sees "no container yet" and starts building a second one, which fires its own
`ChildAdded`, calling `view()` again, recursing without bound. Each level allocates a fresh
`QComboBoxPrivateContainer`/`QListView`, so the process eventually dies of memory exhaustion
rather than a clean stack overflow.

## Root cause

```cpp
// ComboboxItemViewFilter::eventFilter(), case QEvent::Type::ChildAdded:
if (child == _comboBox->view()) {   // <-- reenters view()'s own construction
  ...
}
```

`QComboBoxPrivate::viewContainer()` only assigns its cached container pointer *after* the
container's constructor returns. That constructor calls `QWidget::setParent()`, which
synchronously sends the `ChildAdded` event this filter reacts to — so any call to
`_comboBox->view()` made from within this handler, on the very first popup construction, is
guaranteed to see the container as not-yet-built and start a new one.

## Fix

`child` (from `childEvent->child()`) is already the object we need to identify — there's no need
to call `view()` again at all. Checking its type directly breaks the recursion:

```cpp
if (qobject_cast<const QAbstractItemView*>(child)) {
  ...
}
```

Same fix applies in both `ComboboxItemViewFilter::eventFilter()` and
`ComboboxFilter::eventFilter()`, which have the identical pattern.

## Reproduction

Crash observed on first construction of a `QComboBox`'s popup view (backtrace shows the same
~15-frame cycle repeating: `QComboBox::view()` → `QComboBoxPrivate::viewContainer()` →
`QComboBoxPrivateContainer` ctor → `QWidget::setParent()` → `sendEvent` →
`ComboboxItemViewFilter::eventFilter()` → `QComboBox::view()` → ...), ending in an allocation
failure inside `QArrayData::reallocateUnaligned` (incidental crash site, not the actual bug).

- Qt: 6.x (Qt6Widgets, style sheet-based `QStyleSheetStyle::polish()` path)
- Qlementine: `v1.4.2` tag, and confirmed still present on `dev` as of the `isDefaultItemDelegate`
  commit (the delegate-overwrite fix landed nearby but doesn't touch this recursion)
- OS: Linux

## Environment / affected file

`lib/src/style/eventFilters/ComboboxItemViewFilter.hpp`
